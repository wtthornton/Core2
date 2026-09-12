#include "protocol.h"

namespace {

AppState g_state;
constexpr size_t kLineMax = 128;
char g_line[kLineMax];
size_t g_line_len = 0;

}  // namespace

void protocol_copy_trunc(char* dst, size_t dst_len, const char* src) {
  if (dst_len == 0) {
    return;
  }
  if (src == nullptr) {
    dst[0] = 0;
    return;
  }
  strncpy(dst, src, dst_len - 1);
  dst[dst_len - 1] = 0;
}

AppState& app_state() { return g_state; }

void protocol_begin() {
  g_line_len = 0;
  protocol_copy_trunc(g_state.net.ssid, sizeof(g_state.net.ssid), CORE2_WIFI_SSID);
  protocol_copy_trunc(g_state.net.pass, sizeof(g_state.net.pass), CORE2_WIFI_PASS);
  protocol_copy_trunc(g_state.net.af_url, sizeof(g_state.net.af_url), AF_BASE_URL);
  protocol_copy_trunc(g_state.net.api_key, sizeof(g_state.net.api_key), AF_API_KEY);
  protocol_copy_trunc(g_state.net.project_slug, sizeof(g_state.net.project_slug), AF_PROJECT_SLUG);
  Serial.setRxBufferSize(512);
  Serial.println(F("core2 af-direct boot"));
}

void protocol_poll_serial() {
  // USB is debug-only (VISION.md). Drain RX; ignore host NDJSON.
  while (Serial.available() > 0) {
    const int ch = Serial.read();
    if (ch < 0) {
      break;
    }
    if (ch == '\n' || ch == '\r') {
      g_line_len = 0;
      continue;
    }
    if (g_line_len + 1 < kLineMax) {
      g_line[g_line_len++] = static_cast<char>(ch);
    } else {
      g_line_len = 0;
    }
  }
}

void protocol_set_alert(const char* id, const char* sev, const char* src, const char* agent,
                        const char* msg) {
  if (id != nullptr && id[0] != 0 &&
      strncmp(g_state.alert.id, id, sizeof(g_state.alert.id) - 1) == 0) {
    return;
  }
  AlertState& a = g_state.alert;
  protocol_copy_trunc(a.id, sizeof(a.id), id);
  protocol_copy_trunc(a.sev, sizeof(a.sev), sev != nullptr ? sev : "error");
  protocol_copy_trunc(a.src, sizeof(a.src), src);
  protocol_copy_trunc(a.agent, sizeof(a.agent), agent);
  protocol_copy_trunc(a.msg, sizeof(a.msg), msg);
  a.have = true;
  a.acked = false;
  a.fire_haptic = true;
  g_state.dirty = true;
}

bool protocol_muted() {
  if (g_state.mute_until_ms == 0) {
    return false;
  }
  if (static_cast<int32_t>(millis() - g_state.mute_until_ms) >= 0) {
    g_state.mute_until_ms = 0;
    return false;
  }
  return true;
}

void protocol_ack() {
  g_state.alert.acked = true;
  g_state.alert.fire_haptic = false;
  g_state.dirty = true;
}

void protocol_mute_15m() {
  g_state.mute_until_ms = millis() + 15UL * 60UL * 1000UL;
  g_state.dirty = true;
}

void protocol_next_screen() {
  const uint8_t n = static_cast<uint8_t>(g_state.screen) + 1;
  g_state.screen = static_cast<Screen>(n % static_cast<uint8_t>(Screen::Count));
  g_state.dirty = true;
}

void protocol_prev_screen() {
  const int n = static_cast<int>(g_state.screen) - 1;
  g_state.screen =
      static_cast<Screen>((n < 0) ? static_cast<int>(Screen::Count) - 1 : n);
  g_state.dirty = true;
}

void protocol_set_screen(Screen screen) {
  if (static_cast<uint8_t>(screen) >= static_cast<uint8_t>(Screen::Count)) {
    return;
  }
  g_state.screen = screen;
  g_state.dirty = true;
}
