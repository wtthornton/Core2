#include "voice.h"

#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>
#include <cstring>
#include <esp_heap_caps.h>
#include <esp_random.h>

#include "protocol.h"
#include "voice_client.h"

extern const uint8_t kCueBlockedApi[] asm("_binary_assets_cues_blocked_api_wav_start");
extern const uint8_t kCueBlockedApiEnd[] asm("_binary_assets_cues_blocked_api_wav_end");
extern const uint8_t kCueSpokenBlocked[] asm("_binary_assets_cues_spoken_blocked_wav_start");
extern const uint8_t kCueSpokenBlockedEnd[] asm("_binary_assets_cues_spoken_blocked_wav_end");
extern const uint8_t kCueMicMissing[] asm("_binary_assets_cues_mic_missing_wav_start");
extern const uint8_t kCueMicMissingEnd[] asm("_binary_assets_cues_mic_missing_wav_end");
extern const uint8_t kCueNeedKey[] asm("_binary_assets_cues_need_key_wav_start");
extern const uint8_t kCueNeedKeyEnd[] asm("_binary_assets_cues_need_key_wav_end");
extern const uint8_t kCueTooShort[] asm("_binary_assets_cues_too_short_wav_start");
extern const uint8_t kCueTooShortEnd[] asm("_binary_assets_cues_too_short_wav_end");

namespace {

constexpr uint32_t kSampleRate = 16000;
constexpr uint32_t kMaxMs = 6000;
constexpr size_t kMaxSamples = (kSampleRate * kMaxMs) / 1000;
constexpr size_t kChunk = 1024;
constexpr size_t kWavHdr = 44;
constexpr uint8_t kSdCs = 4;
constexpr char kSdPath[] = "/talk.wav";

uint8_t* g_blob = nullptr;
int16_t* g_pcm = nullptr;
size_t g_samples = 0;
size_t g_pending = 0;
int g_holds = 0;
bool g_recording = false;
bool g_mic_ok = false;
bool g_use_sd = false;
bool g_sd_ready = false;
uint32_t g_rec_start_ms = 0;
volatile uint8_t g_cue = 0;

const uint8_t* cue_wav(VoiceCue cue, size_t* bytes_out) {
  const uint8_t* start = nullptr;
  const uint8_t* end = nullptr;
  switch (cue) {
    case VoiceCue::BlockedApi:
      start = kCueBlockedApi;
      end = kCueBlockedApiEnd;
      break;
    case VoiceCue::SpokenBlocked:
      start = kCueSpokenBlocked;
      end = kCueSpokenBlockedEnd;
      break;
    case VoiceCue::MicMissing:
      start = kCueMicMissing;
      end = kCueMicMissingEnd;
      break;
    case VoiceCue::NeedKey:
      start = kCueNeedKey;
      end = kCueNeedKeyEnd;
      break;
    case VoiceCue::TooShort:
      start = kCueTooShort;
      end = kCueTooShortEnd;
      break;
    default:
      break;
  }
  if (bytes_out != nullptr) {
    *bytes_out = (start != nullptr && end != nullptr) ? static_cast<size_t>(end - start) : 0;
  }
  return start;
}

void speaker_ready() {
  if (M5.Mic.isRunning()) {
    M5.Mic.end();
  }
  if (!M5.Speaker.isRunning()) {
    M5.Speaker.begin();
    M5.Speaker.setVolume(72);
  }
}

void play_status_wav(const uint8_t* wav, size_t bytes) {
  if (wav == nullptr || bytes < kWavHdr || protocol_muted() || g_recording) {
    return;
  }
  speaker_ready();
  M5.Speaker.stop();
  M5.Speaker.playWav(wav, bytes, 1, -1, true);
}

void drain_cue() {
  if (protocol_muted()) {
    g_cue = 0;
    if (!g_recording && M5.Speaker.isPlaying()) {
      M5.Speaker.stop();
    }
    return;
  }
  const uint8_t raw = g_cue;
  if (raw == 0 || g_recording) {
    return;
  }
  g_cue = 0;
  size_t n = 0;
  const uint8_t* wav = cue_wav(static_cast<VoiceCue>(raw), &n);
  play_status_wav(wav, n);
}

void write_u16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v);
  p[1] = static_cast<uint8_t>(v >> 8);
}

void write_u32(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v);
  p[1] = static_cast<uint8_t>(v >> 8);
  p[2] = static_cast<uint8_t>(v >> 16);
  p[3] = static_cast<uint8_t>(v >> 24);
}

void write_wav_header(uint8_t* h, uint32_t data_bytes, uint32_t rate) {
  memcpy(h, "RIFF", 4);
  write_u32(h + 4, 36 + data_bytes);
  memcpy(h + 8, "WAVEfmt ", 8);
  write_u32(h + 16, 16);
  write_u16(h + 20, 1);
  write_u16(h + 22, 1);
  write_u32(h + 24, rate);
  write_u32(h + 28, rate * 2);
  write_u16(h + 32, 2);
  write_u16(h + 34, 16);
  memcpy(h + 36, "data", 4);
  write_u32(h + 40, data_bytes);
}

void set_detail(const char* msg) {
  protocol_copy_trunc(app_state().voice.detail, sizeof(app_state().voice.detail), msg);
  app_state().dirty = true;
}

void set_phase(VoicePhase phase) {
  app_state().voice.phase = phase;
  app_state().dirty = true;
}

bool sd_begin() {
  if (g_sd_ready) {
    return true;
  }
  g_sd_ready = SD.begin(kSdCs, SPI, 25000000);
  return g_sd_ready;
}

void ensure_session() {
  VoiceState& v = app_state().voice;
  if (v.session_id[0] != 0) {
    return;
  }
  snprintf(v.session_id, sizeof(v.session_id), "c2-%08lx", static_cast<unsigned long>(esp_random()));
}

bool alloc_psram() {
  if (g_blob != nullptr) {
    return true;
  }
  g_blob = static_cast<uint8_t*>(
      heap_caps_malloc(kWavHdr + kMaxSamples * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (g_blob == nullptr) {
    return false;
  }
  g_pcm = reinterpret_cast<int16_t*>(g_blob + kWavHdr);
  return true;
}

bool open_sd_wav() {
  if (!sd_begin()) {
    return false;
  }
  SD.remove(kSdPath);
  File f = SD.open(kSdPath, FILE_WRITE);
  if (!f) {
    return false;
  }
  uint8_t hdr[kWavHdr] = {};
  write_wav_header(hdr, 0, kSampleRate);
  f.write(hdr, kWavHdr);
  f.close();
  g_use_sd = true;
  app_state().voice.spilled_sd = true;
  return true;
}

bool append_sd(const int16_t* pcm, size_t n) {
  File f = SD.open(kSdPath, FILE_APPEND);
  if (!f) {
    return false;
  }
  const size_t wrote = f.write(reinterpret_cast<const uint8_t*>(pcm), n * sizeof(int16_t));
  f.close();
  return wrote == n * sizeof(int16_t);
}

bool finalize_sd_header() {
  File f = SD.open(kSdPath, "r+");
  if (!f) {
    return false;
  }
  uint8_t hdr[kWavHdr];
  write_wav_header(hdr, static_cast<uint32_t>(g_samples * sizeof(int16_t)), kSampleRate);
  f.seek(0);
  f.write(hdr, kWavHdr);
  f.close();
  return true;
}

bool mic_start() {
  Serial.println(F("mic on"));
  M5.Speaker.end();
  if (!M5.Mic.isEnabled()) {
    return false;
  }
  if (!M5.Mic.begin()) {
    M5.Speaker.begin();
    M5.Speaker.setVolume(72);
    Serial.println(F("mic begin fail"));
    return false;
  }
  Serial.println(F("mic ok"));
  return true;
}

void mic_stop_restore() {
  if (M5.Mic.isEnabled() && M5.Mic.isRunning()) {
    uint32_t wait_ms = millis();
    while (M5.Mic.isRecording() != 0 && millis() - wait_ms < 400) {
      delay(2);
    }
    M5.Mic.end();
  }
  if (!M5.Speaker.isRunning()) {
    M5.Speaker.begin();
    M5.Speaker.setVolume(72);
  }
}

void queue_chunk() {
  if (g_samples >= kMaxSamples || M5.Mic.isRecording() != 0 || g_pcm == nullptr) {
    return;
  }
  const size_t remain = kMaxSamples - g_samples;
  const size_t n = remain < kChunk ? remain : kChunk;
  if (!M5.Mic.record(g_pcm + g_samples, n, kSampleRate, false)) {
    return;
  }
  g_pending = n;
}

int16_t g_sd_scratch[kChunk];

void queue_chunk_sd() {
  if (g_samples >= kMaxSamples || M5.Mic.isRecording() != 0) {
    return;
  }
  const size_t remain = kMaxSamples - g_samples;
  const size_t n = remain < kChunk ? remain : kChunk;
  if (!M5.Mic.record(g_sd_scratch, n, kSampleRate, false)) {
    return;
  }
  g_pending = n;
}

void commit_pending_sd() {
  if (g_pending == 0) {
    return;
  }
  append_sd(g_sd_scratch, g_pending);
  g_samples += g_pending;
  g_pending = 0;
  app_state().voice.samples = static_cast<uint32_t>(g_samples);
  app_state().dirty = true;
}

void finish_record(bool keep) {
  if (!g_recording) {
    g_holds = 0;
    return;
  }
  uint32_t wait_ms = millis();
  while (M5.Mic.isRecording() != 0 && millis() - wait_ms < 400) {
    delay(2);
  }
  if (g_use_sd) {
    commit_pending_sd();
    finalize_sd_header();
  } else {
    if (g_pending) {
      g_samples += g_pending;
      g_pending = 0;
    }
    if (g_blob != nullptr) {
      write_wav_header(g_blob, static_cast<uint32_t>(g_samples * sizeof(int16_t)), kSampleRate);
    }
  }
  g_recording = false;
  g_holds = 0;
  mic_stop_restore();
  VoiceState& v = app_state().voice;
  v.samples = static_cast<uint32_t>(g_samples);
  v.clipped = g_samples >= kMaxSamples;
  if (!keep) {
    g_samples = 0;
    v.samples = 0;
    v.clipped = false;
    set_phase(v.mic_ok ? VoicePhase::Idle : VoicePhase::MicMissing);
  }
  app_state().dirty = true;
}

}  // namespace

void voice_begin() {
  VoiceState& v = app_state().voice;
  v.sample_rate = kSampleRate;
  v.stream_blocked = true;
  ensure_session();

  // Keep SD CS idle so a card cannot steal the display SPI bus (CS G4).
  pinMode(kSdCs, OUTPUT);
  digitalWrite(kSdCs, HIGH);

  // Do not Speaker.end()/Mic.begin() at boot — I2S on G0 can hang before
  // the first frame (clear_display leaves a black panel). Probe on PTT.
  g_mic_ok = M5.Mic.isEnabled();
  v.mic_ok = g_mic_ok;
  if (!g_mic_ok) {
    set_phase(VoicePhase::MicMissing);
    set_detail("mic missing (rear board)");
  } else {
    set_phase(VoicePhase::Idle);
    set_detail("hold Talk to speak");
  }
  Serial.println(g_mic_ok ? F("voice ready") : F("voice no mic pin"));
}

void voice_poll() {
  AppState& st = app_state();
  if (st.screen != Screen::Talk && g_recording) {
    finish_record(false);
  }
  if (st.screen == Screen::Talk &&
      (g_recording || st.voice.phase == VoicePhase::Thinking ||
       st.voice.phase == VoicePhase::Playing)) {
    st.dirty = true;
  }
  drain_cue();

  if (g_recording) {
    if (g_use_sd) {
      if (M5.Mic.isRecording() == 0) {
        commit_pending_sd();
        if (g_samples >= kMaxSamples || millis() - g_rec_start_ms >= kMaxMs) {
          st.voice.clipped = g_samples >= kMaxSamples;
          finish_record(true);
          voice_client_submit();
          return;
        }
        queue_chunk_sd();
      }
    } else if (M5.Mic.isRecording() == 0) {
      if (g_pending) {
        g_samples += g_pending;
        g_pending = 0;
        st.voice.samples = static_cast<uint32_t>(g_samples);
        st.dirty = true;
      }
      if (g_samples >= kMaxSamples || millis() - g_rec_start_ms >= kMaxMs) {
        st.voice.clipped = g_samples >= kMaxSamples;
        finish_record(true);
        voice_client_submit();
        return;
      }
      queue_chunk();
    }
  }

  if (st.voice.phase == VoicePhase::Playing && !M5.Speaker.isPlaying()) {
    set_phase(VoicePhase::Ready);
  }
}

bool voice_ptt_start() {
  AppState& st = app_state();
  VoiceState& v = st.voice;
  if (st.screen != Screen::Talk) {
    return false;
  }
  if (v.phase == VoicePhase::Thinking || v.phase == VoicePhase::Playing) {
    return false;
  }
  g_cue = 0;
  g_holds++;
  if (g_recording) {
    return true;
  }
  if (!g_mic_ok) {
    set_phase(VoicePhase::MicMissing);
    set_detail("mic missing (rear board)");
    g_holds = 0;
    voice_cue(VoiceCue::MicMissing);
    return false;
  }

  g_samples = 0;
  g_pending = 0;
  g_use_sd = false;
  v.spilled_sd = false;
  v.clipped = false;
  v.tts_present = false;
  v.transcript[0] = 0;
  v.reply[0] = 0;
  v.turn_id[0] = 0;
  v.samples = 0;

  if (!alloc_psram()) {
    if (!open_sd_wav()) {
      set_phase(VoicePhase::Error);
      set_detail("need PSRAM");
      g_holds = 0;
      return false;
    }
  }

  if (!mic_start()) {
    set_phase(VoicePhase::MicMissing);
    set_detail("mic missing (rear board)");
    g_holds = 0;
    v.mic_ok = false;
    g_mic_ok = false;
    voice_cue(VoiceCue::MicMissing);
    return false;
  }

  g_recording = true;
  g_rec_start_ms = millis();
  set_phase(VoicePhase::Listening);
  set_detail("listening");
  if (g_use_sd) {
    queue_chunk_sd();
  } else {
    queue_chunk();
  }
  return true;
}

bool voice_ptt_stop() {
  if (g_holds > 0) {
    g_holds--;
  }
  if (g_holds > 0 || !g_recording) {
    return false;
  }
  finish_record(true);
  return true;
}

void voice_ptt_cancel() {
  g_holds = 0;
  finish_record(false);
}

bool voice_is_recording() { return g_recording; }

const int16_t* voice_pcm(size_t* samples_out) {
  if (samples_out != nullptr) {
    *samples_out = g_use_sd ? 0 : g_samples;
  }
  return g_use_sd ? nullptr : g_pcm;
}

uint32_t voice_sample_rate() { return kSampleRate; }

const uint8_t* voice_wav_blob(size_t* bytes_out) {
  if (g_use_sd || g_blob == nullptr || g_samples == 0) {
    if (bytes_out != nullptr) {
      *bytes_out = 0;
    }
    return nullptr;
  }
  write_wav_header(g_blob, static_cast<uint32_t>(g_samples * sizeof(int16_t)), kSampleRate);
  if (bytes_out != nullptr) {
    *bytes_out = kWavHdr + g_samples * sizeof(int16_t);
  }
  return g_blob;
}

const char* voice_sd_path() { return g_use_sd ? kSdPath : nullptr; }

size_t voice_sample_count() { return g_samples; }

void voice_play_pcm(const int16_t* pcm, size_t samples, uint32_t rate) {
  if (pcm == nullptr || samples == 0 || protocol_muted()) {
    return;
  }
  speaker_ready();
  M5.Speaker.stop();
  M5.Speaker.playRaw(pcm, samples, rate == 0 ? kSampleRate : rate, false, 1, -1, true);
  set_phase(VoicePhase::Playing);
}

void voice_play_wav(const uint8_t* wav, size_t bytes) {
  if (wav == nullptr || bytes < kWavHdr || protocol_muted()) {
    return;
  }
  speaker_ready();
  M5.Speaker.stop();
  M5.Speaker.playWav(wav, bytes, 1, -1, true);
  set_phase(VoicePhase::Playing);
}

void voice_speaker_resume() { mic_stop_restore(); }

void voice_cue(VoiceCue cue) {
  if (cue == VoiceCue::None || protocol_muted()) {
    return;
  }
  g_cue = static_cast<uint8_t>(cue);
}

void voice_silence() {
  g_cue = 0;
  if (!g_recording && M5.Speaker.isPlaying()) {
    M5.Speaker.stop();
  }
}
