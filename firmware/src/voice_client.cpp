#include "voice_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SD.h>
#include <WiFi.h>
#include <cstring>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "protocol.h"
#include "voice.h"

namespace {

constexpr uint32_t kPostTimeoutMs = 25000;
constexpr uint32_t kPollTimeoutMs = 8000;
constexpr uint32_t kAudioTimeoutMs = 15000;
constexpr uint32_t kPollEveryMs = 1500;
constexpr int kPollMax = 30;
constexpr size_t kJsonMax = 8192;
constexpr size_t kAudioMax = 96 * 1024;

TaskHandle_t g_task = nullptr;
volatile bool g_submit = false;
volatile bool g_busy = false;

uint8_t* g_play = nullptr;
size_t g_play_len = 0;
uint32_t g_play_rate = 16000;
bool g_play_wav = false;
volatile bool g_play_ready = false;

String join_url(const char* base, const char* path) {
  String url = base;
  while (url.endsWith("/")) {
    url.remove(url.length() - 1);
  }
  if (path[0] != '/') {
    url += '/';
  }
  url += path;
  return url;
}

void set_detail(const char* msg) {
  protocol_copy_trunc(app_state().voice.detail, sizeof(app_state().voice.detail), msg);
  app_state().dirty = true;
}

void set_phase(VoicePhase phase) {
  app_state().voice.phase = phase;
  app_state().dirty = true;
}

void mark_blocked() {
  VoiceState& v = app_state().voice;
  v.tts_present = false;
  v.stream_blocked = true;
  v.transcript[0] = 0;
  v.reply[0] = 0;
  set_phase(VoicePhase::Blocked);
  set_detail("blocked: no AF voice API (TAP-7550)");
  voice_cue(VoiceCue::BlockedApi);
}

void mark_error(const char* msg) {
  set_phase(VoicePhase::Error);
  set_detail(msg);
}

int status_class(int code) {
  return code / 100;
}

bool is_blocked_code(int code) {
  return code == 404 || code == 405 || code == 501;
}

bool is_terminal_status(const char* status) {
  if (status == nullptr || status[0] == 0) {
    return false;
  }
  return strcmp(status, "complete") == 0 || strcmp(status, "completed") == 0 ||
         strcmp(status, "done") == 0 || strcmp(status, "ready") == 0 ||
         strcmp(status, "error") == 0 || strcmp(status, "failed") == 0 ||
         strcmp(status, "blocked") == 0;
}

int b64_val(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A';
  }
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 26;
  }
  if (c >= '0' && c <= '9') {
    return c - '0' + 52;
  }
  if (c == '+') {
    return 62;
  }
  if (c == '/') {
    return 63;
  }
  return -1;
}

size_t b64_decode(const char* src, uint8_t* dst, size_t dst_max) {
  size_t n = 0;
  int val = 0;
  int valb = -8;
  for (const char* p = src; *p && n < dst_max; ++p) {
    if (*p == '=' || *p == '\n' || *p == '\r') {
      continue;
    }
    const int d = b64_val(*p);
    if (d < 0) {
      break;
    }
    val = (val << 6) + d;
    valb += 6;
    if (valb >= 0) {
      dst[n++] = static_cast<uint8_t>((val >> valb) & 0xFF);
      valb -= 8;
    }
  }
  return n;
}

void apply_session(JsonDocument& doc) {
  const char* sid = doc["session_id"] | "";
  if (sid[0] != 0) {
    protocol_copy_trunc(app_state().voice.session_id, sizeof(app_state().voice.session_id), sid);
  }
}

void apply_text(JsonDocument& doc) {
  VoiceState& v = app_state().voice;
  protocol_copy_trunc(v.transcript, sizeof(v.transcript), doc["transcript"] | "");
  protocol_copy_trunc(v.reply, sizeof(v.reply),
                      doc["reply_text"] | doc["reply"] | doc["text"] | "");
  const char* tid = doc["turn_id"] | doc["id"] | "";
  if (tid[0] != 0) {
    protocol_copy_trunc(v.turn_id, sizeof(v.turn_id), tid);
  }
  apply_session(doc);
  v.stream_blocked = true;
  app_state().dirty = true;
}

bool looks_wav(const uint8_t* p, size_t n) {
  return n >= 12 && memcmp(p, "RIFF", 4) == 0 && memcmp(p + 8, "WAVE", 4) == 0;
}

void queue_play(uint8_t* buf, size_t n, uint32_t rate, bool wav) {
  if (g_play != nullptr && g_play != buf) {
    heap_caps_free(g_play);
  }
  g_play = buf;
  g_play_len = n;
  g_play_rate = rate == 0 ? 16000 : rate;
  g_play_wav = wav;
  g_play_ready = true;
}

bool fetch_audio_url(const char* url) {
  if (url == nullptr || url[0] == 0) {
    return false;
  }
  const NetCfg& n = app_state().net;
  HTTPClient http;
  http.setTimeout(kAudioTimeoutMs);
  String full = url;
  if (url[0] == '/') {
    full = join_url(n.af_url, url);
  }
  if (!http.begin(full)) {
    return false;
  }
  if (n.api_key[0] != 0) {
    http.addHeader("Authorization", String("Bearer ") + n.api_key);
  }
  const int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }
  const int len = http.getSize();
  if (len > static_cast<int>(kAudioMax)) {
    http.end();
    return false;
  }
  WiFiClient* stream = http.getStreamPtr();
  uint8_t* buf = static_cast<uint8_t*>(heap_caps_malloc(kAudioMax, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (buf == nullptr) {
    http.end();
    return false;
  }
  size_t got = 0;
  const uint32_t start = millis();
  while (http.connected() && got < kAudioMax && millis() - start < kAudioTimeoutMs) {
    const int avail = stream != nullptr ? stream->available() : 0;
    if (avail <= 0) {
      delay(10);
      if (!http.connected()) {
        break;
      }
      continue;
    }
    const int n = stream->readBytes(buf + got, kAudioMax - got);
    if (n <= 0) {
      break;
    }
    got += static_cast<size_t>(n);
  }
  http.end();
  if (got < 44) {
    heap_caps_free(buf);
    return false;
  }
  queue_play(buf, got, 16000, looks_wav(buf, got));
  return true;
}

bool take_audio_field(JsonDocument& doc) {
  const char* audio_url = doc["audio_url"] | "";
  if (audio_url[0] == 0) {
    JsonObject audio_obj = doc["audio"].as<JsonObject>();
    if (!audio_obj.isNull()) {
      audio_url = audio_obj["url"] | "";
    }
  }
  if (audio_url[0] != 0) {
    return fetch_audio_url(audio_url);
  }
  const char* audio = doc["audio"] | "";
  if (audio[0] == 0) {
    return false;
  }
  if (strncmp(audio, "http://", 7) == 0 || strncmp(audio, "https://", 8) == 0 || audio[0] == '/') {
    return fetch_audio_url(audio);
  }
  uint8_t* buf = static_cast<uint8_t*>(heap_caps_malloc(kAudioMax, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (buf == nullptr) {
    return false;
  }
  const size_t n = b64_decode(audio, buf, kAudioMax);
  if (n < 44) {
    heap_caps_free(buf);
    return false;
  }
  queue_play(buf, n, 16000, looks_wav(buf, n));
  return true;
}

void finish_ready(JsonDocument& doc) {
  apply_text(doc);
  VoiceState& v = app_state().voice;
  const char* status = doc["status"] | "";
  if (strcmp(status, "error") == 0 || strcmp(status, "failed") == 0) {
    mark_error("voice turn failed");
    return;
  }
  const bool played = take_audio_field(doc);
  v.tts_present = played;
  v.stream_blocked = true;
  if (!played) {
    set_phase(VoicePhase::Ready);
    set_detail("spoken blocked (TAP-7554)");
    voice_cue(VoiceCue::SpokenBlocked);
  }
}

int http_json(const char* method, const char* path, const uint8_t* body, size_t body_len,
              const char* content_type, String& out, uint32_t timeout_ms) {
  out = "";
  const NetCfg& n = app_state().net;
  if (n.af_url[0] == 0 || WiFi.status() != WL_CONNECTED) {
    return 0;
  }
  HTTPClient http;
  http.setTimeout(timeout_ms);
  if (!http.begin(join_url(n.af_url, path))) {
    return 0;
  }
  if (n.api_key[0] != 0) {
    http.addHeader("Authorization", String("Bearer ") + n.api_key);
  }
  http.addHeader("Accept", "application/json");
  int code = 0;
  if (strcmp(method, "GET") == 0) {
    code = http.GET();
  } else {
    if (content_type != nullptr) {
      http.addHeader("Content-Type", content_type);
    }
    code = http.POST(const_cast<uint8_t*>(body), body_len);
  }
  if (code > 0) {
    out = http.getString();
    if (out.length() > kJsonMax) {
      out = out.substring(0, kJsonMax);
    }
  }
  http.end();
  return code;
}

int post_file(const char* path, const char* file_path, String& out) {
  out = "";
  const NetCfg& n = app_state().net;
  File f = SD.open(file_path, FILE_READ);
  if (!f) {
    return 0;
  }
  HTTPClient http;
  http.setTimeout(kPostTimeoutMs);
  if (!http.begin(join_url(n.af_url, path))) {
    f.close();
    return 0;
  }
  if (n.api_key[0] != 0) {
    http.addHeader("Authorization", String("Bearer ") + n.api_key);
  }
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "audio/wav");
  const int code = http.sendRequest("POST", &f, f.size());
  if (code > 0) {
    out = http.getString();
    if (out.length() > kJsonMax) {
      out = out.substring(0, kJsonMax);
    }
  }
  http.end();
  f.close();
  return code;
}

void run_turn() {
  VoiceState& v = app_state().voice;
  v.stream_blocked = true;
  if (WiFi.status() != WL_CONNECTED) {
    mark_error("Wi-Fi");
    return;
  }
  const size_t samples = voice_sample_count();
  if (samples < 16000 / 4) {
    set_phase(v.mic_ok ? VoicePhase::Idle : VoicePhase::MicMissing);
    set_detail("too short");
    voice_cue(VoiceCue::TooShort);
    return;
  }

  set_phase(VoicePhase::Thinking);
  set_detail("thinking");

  const NetCfg& n = app_state().net;
  char path[160];
  snprintf(path, sizeof(path), "/projects/%s/voice/turns?session_id=%s&sample_rate=16000&channels=1",
           n.project_slug[0] ? n.project_slug : "core2",
           v.session_id[0] ? v.session_id : "c2-00000000");

  String body;
  int code = 0;
  size_t wav_len = 0;
  const uint8_t* wav = voice_wav_blob(&wav_len);
  const char* sd_path = voice_sd_path();
  if (wav != nullptr && wav_len > 0) {
    code = http_json("POST", path, wav, wav_len, "audio/wav", body, kPostTimeoutMs);
  } else if (sd_path != nullptr) {
    code = post_file(path, sd_path, body);
  } else {
    mark_error("need PSRAM");
    return;
  }

  if (code == 0) {
    mark_error("no AF");
    return;
  }
  if (is_blocked_code(code)) {
    mark_blocked();
    return;
  }
  if (code == 401 || code == 403) {
    mark_error("need afp_ key");
    voice_cue(VoiceCue::NeedKey);
    return;
  }
  if (code == 413) {
    mark_error("audio too large");
    return;
  }
  if (code == 422) {
    mark_error("unreadable audio");
    return;
  }
  if (status_class(code) != 2) {
    mark_error("voice HTTP error");
    return;
  }

  JsonDocument doc;
  if (!body.isEmpty() && deserializeJson(doc, body)) {
    mark_error("bad voice JSON");
    return;
  }
  apply_session(doc);
  const char* tid = doc["turn_id"] | doc["id"] | "";
  if (tid[0] != 0) {
    protocol_copy_trunc(v.turn_id, sizeof(v.turn_id), tid);
  }

  const char* status = doc["status"] | "";
  const bool have_text = (doc["transcript"] | "")[0] != 0 || (doc["reply_text"] | "")[0] != 0;
  if (code == 200 || is_terminal_status(status) || (have_text && code != 202)) {
    finish_ready(doc);
    return;
  }

  if (v.turn_id[0] == 0) {
    mark_error("no turn_id");
    return;
  }

  for (int i = 0; i < kPollMax; ++i) {
    vTaskDelay(pdMS_TO_TICKS(kPollEveryMs));
    char poll[160];
    snprintf(poll, sizeof(poll), "/projects/%s/voice/turns/%s",
             n.project_slug[0] ? n.project_slug : "core2", v.turn_id);
    String pbody;
    const int pc = http_json("GET", poll, nullptr, 0, nullptr, pbody, kPollTimeoutMs);
    if (is_blocked_code(pc)) {
      mark_blocked();
      return;
    }
    if (pc == 401 || pc == 403) {
      mark_error("need afp_ key");
      voice_cue(VoiceCue::NeedKey);
      return;
    }
    if (pc != 200) {
      continue;
    }
    JsonDocument pdoc;
    if (deserializeJson(pdoc, pbody)) {
      continue;
    }
    const char* st = pdoc["status"] | "";
    apply_text(pdoc);
    if (is_terminal_status(st) || (pdoc["reply_text"] | "")[0] != 0) {
      finish_ready(pdoc);
      return;
    }
  }
  mark_error("voice poll timeout");
}

void voice_task(void*) {
  for (;;) {
    if (!g_submit) {
      vTaskDelay(pdMS_TO_TICKS(40));
      continue;
    }
    g_submit = false;
    g_busy = true;
    run_turn();
    g_busy = false;
  }
}

}  // namespace

void voice_client_begin() {
  if (g_task != nullptr) {
    return;
  }
  xTaskCreatePinnedToCore(voice_task, "af_voice", 8192, nullptr, 1, &g_task, 1);
}

void voice_client_submit() {
  if (g_busy || g_submit) {
    return;
  }
  if (voice_is_recording()) {
    return;
  }
  g_submit = true;
}

void voice_client_poll() {
  if (!g_play_ready) {
    return;
  }
  g_play_ready = false;
  if (protocol_muted()) {
    return;
  }
  app_state().voice.tts_present = true;
  if (g_play_wav) {
    voice_play_wav(g_play, g_play_len);
  } else {
    voice_play_pcm(reinterpret_cast<const int16_t*>(g_play), g_play_len / 2, g_play_rate);
  }
}

bool voice_client_busy() { return g_busy || g_submit; }
