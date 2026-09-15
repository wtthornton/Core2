#include "af_sse.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "protocol.h"

namespace {

TaskHandle_t g_task = nullptr;
volatile bool g_reset = false;
char g_last_id[48] = "";

struct PendingAlert {
  bool have = false;
  char id[40] = "";
  char sev[12] = "";
  char src[28] = "";
  char agent[24] = "";
  char msg[80] = "";
};

PendingAlert g_pending;

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

bool is_failure_event(const char* type, JsonObject data) {
  if (type == nullptr || type[0] == 0) {
    return false;
  }
  const bool terminal = strstr(type, "invocation.terminal") != nullptr;
  const bool run_done = strstr(type, "run.completed") != nullptr;
  if (!terminal && !run_done) {
    return false;
  }
  if (data["is_error"] | false) {
    return true;
  }
  const char* status = data["status"] | "";
  return strcmp(status, "error") == 0 || strcmp(status, "failed") == 0;
}

void queue_alert(const char* id, const char* type, JsonObject data) {
  PendingAlert a;
  a.have = true;
  protocol_copy_trunc(a.id, sizeof(a.id), id);
  protocol_copy_trunc(a.sev, sizeof(a.sev), "error");
  protocol_copy_trunc(a.src, sizeof(a.src), type);
  protocol_copy_trunc(a.agent, sizeof(a.agent), data["agent"] | data["name"] | "core2");
  const char* status = data["status"] | "failed";
  protocol_copy_trunc(a.msg, sizeof(a.msg), status);
  g_pending = a;
}

void handle_data_line(const String& json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) {
    return;
  }
  const char* type = doc["type"] | "";
  JsonObject data = doc["data"].as<JsonObject>();
  if (!is_failure_event(type, data)) {
    return;
  }
  const char* eid = doc["id"] | data["invocation_id"] | "sse";
  queue_alert(eid, type, data);
}

void consume_block(String& block) {
  int start = 0;
  while (start < static_cast<int>(block.length())) {
    const int nl = block.indexOf('\n', start);
    String line = nl < 0 ? block.substring(start) : block.substring(start, nl);
    start = nl < 0 ? static_cast<int>(block.length()) : nl + 1;
    line.trim();
    if (line.startsWith("id:")) {
      String id = line.substring(3);
      id.trim();
      protocol_copy_trunc(g_last_id, sizeof(g_last_id), id.c_str());
    } else if (line.startsWith("data:")) {
      String payload = line.substring(5);
      payload.trim();
      handle_data_line(payload);
    }
  }
}

void sse_loop() {
  const NetCfg& n = app_state().net;
  if (n.af_url[0] == 0 || n.project_slug[0] == 0 || n.api_key[0] == 0) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    return;
  }

  char path[96];
  snprintf(path, sizeof(path), "/projects/%s/events?consumer_id=core2-desk", n.project_slug);
  // Ops alerts only (TAP-7503 / project events). Talk tokens wait on TAP-7555;
  // do not parse this stream as Jarvis transcript.
  HTTPClient http;
  http.setTimeout(65000);
  if (!http.begin(join_url(n.af_url, path))) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    return;
  }
  http.addHeader("Authorization", String("Bearer ") + n.api_key);
  http.addHeader("Accept", "text/event-stream");
  http.addHeader("Cache-Control", "no-cache");
  if (g_last_id[0]) {
    http.addHeader("Last-Event-ID", g_last_id);
  }
  const int code = http.GET();
  if (code != 200) {
    http.end();
    vTaskDelay(pdMS_TO_TICKS(4000));
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  String buf;
  buf.reserve(512);
  uint32_t last_byte_ms = millis();
  while (http.connected() && !g_reset) {
    while (stream != nullptr && stream->available() > 0) {
      const int ch = stream->read();
      if (ch < 0) {
        break;
      }
      last_byte_ms = millis();
      buf += static_cast<char>(ch);
      if (buf.length() > 2048) {
        buf.remove(0, buf.length() - 256);
      }
      if (buf.endsWith("\n\n") || buf.endsWith("\r\n\r\n")) {
        consume_block(buf);
        buf = "";
      }
    }
    if (millis() - last_byte_ms > 90000UL) {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(40));
  }
  http.end();
  vTaskDelay(pdMS_TO_TICKS(2500));
}

void sse_task(void*) {
  for (;;) {
    if (g_reset) {
      g_reset = false;
    }
    sse_loop();
  }
}

}  // namespace

void af_sse_begin() {
  if (g_task != nullptr) {
    return;
  }
  xTaskCreatePinnedToCore(sse_task, "af_sse", 6144, nullptr, 1, &g_task, 1);
}

void af_sse_poll() {
  if (!g_pending.have) {
    return;
  }
  PendingAlert a = g_pending;
  g_pending.have = false;
  protocol_set_alert(a.id, a.sev, a.src, a.agent, a.msg);
}

void af_sse_reset() {
  g_last_id[0] = 0;
  g_reset = true;
}
