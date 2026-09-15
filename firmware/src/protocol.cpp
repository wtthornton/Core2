#include "protocol.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <time.h>

#include "nvs_cfg.h"
#include "ota.h"

namespace {

AppState g_state;
constexpr size_t kLineMax = 192;
char g_line[kLineMax];
size_t g_line_len = 0;


void cfg_show() {
  const NetCfg& n = g_state.net;
  char key_m[24];
  protocol_mask_key(key_m, sizeof(key_m), n.api_key);
  Serial.printf("cfg fw=%s ssid=%s url=%s slug=%s key=%s src=%s\n", CORE2_FW_VERSION, n.ssid,
                n.af_url, n.project_slug, key_m, n.from_nvs ? "nvs" : "flash");
  if (n.quiet_en) {
    Serial.printf("cfg quiet %04u-%04u\n", static_cast<unsigned>(n.quiet_start),
                  static_cast<unsigned>(n.quiet_end));
  } else {
    Serial.println(F("cfg quiet off"));
  }
}

void cfg_time() {
  const time_t now = time(nullptr);
  struct tm tmv = {};
  localtime_r(&now, &tmv);
  Serial.printf("time %04d-%02d-%02d %02d:%02d tz=%s ok=%d\n", tmv.tm_year + 1900, tmv.tm_mon + 1,
                tmv.tm_mday, tmv.tm_hour, tmv.tm_min, CORE2_TZ, protocol_time_ok() ? 1 : 0);
}

bool parse_hm(const char* s, uint16_t* out) {
  if (s == nullptr || out == nullptr || s[0] == 0) {
    return false;
  }
  unsigned h = 0;
  unsigned m = 0;
  unsigned v = 0;
  if (sscanf(s, "%u:%u", &h, &m) == 2) {
    if (h > 23 || m > 59) {
      return false;
    }
    *out = static_cast<uint16_t>(h * 100 + m);
    return true;
  }
  if (sscanf(s, "%u", &v) != 1) {
    return false;
  }
  if (v > 2359 || (v % 100) > 59 || (v / 100) > 23) {
    return false;
  }
  *out = static_cast<uint16_t>(v);
  return true;
}

void cfg_quiet(char* rest) {
  NetCfg& n = g_state.net;
  while (*rest == ' ') {
    ++rest;
  }
  if (*rest == 0 || strcmp(rest, "show") == 0) {
    if (n.quiet_en) {
      Serial.printf("cfg quiet %04u-%04u\n", static_cast<unsigned>(n.quiet_start),
                    static_cast<unsigned>(n.quiet_end));
    } else {
      Serial.println(F("cfg quiet off"));
    }
    return;
  }
  if (strcmp(rest, "off") == 0) {
    n.quiet_en = false;
    nvs_cfg_save_quiet(n);
    g_state.dirty = true;
    Serial.println(F("cfg quiet off"));
    return;
  }
  char* sp = strchr(rest, ' ');
  if (sp == nullptr) {
    Serial.println(F("cfg quiet 2200 0700 | cfg quiet off"));
    return;
  }
  *sp = 0;
  const char* end_s = sp + 1;
  while (*end_s == ' ') {
    ++end_s;
  }
  uint16_t start = 0;
  uint16_t end = 0;
  if (!parse_hm(rest, &start) || !parse_hm(end_s, &end) || start == end) {
    Serial.println(F("cfg quiet need two distinct HHMM times"));
    return;
  }
  n.quiet_start = start;
  n.quiet_end = end;
  n.quiet_en = true;
  nvs_cfg_save_quiet(n);
  g_state.dirty = true;
  Serial.printf("cfg quiet %04u-%04u\n", static_cast<unsigned>(n.quiet_start),
                static_cast<unsigned>(n.quiet_end));
}

bool cfg_set(const char* field, const char* value) {
  NetCfg& n = g_state.net;
  if (strcmp(field, "ssid") == 0) {
    protocol_copy_trunc(n.ssid, sizeof(n.ssid), value);
  } else if (strcmp(field, "pass") == 0) {
    protocol_copy_trunc(n.pass, sizeof(n.pass), value);
  } else if (strcmp(field, "url") == 0) {
    protocol_copy_trunc(n.af_url, sizeof(n.af_url), value);
  } else if (strcmp(field, "slug") == 0) {
    protocol_copy_trunc(n.project_slug, sizeof(n.project_slug), value);
  } else if (strcmp(field, "key") == 0) {
    protocol_copy_trunc(n.api_key, sizeof(n.api_key), value);
  } else {
    return false;
  }
  n.dirty = true;
  g_state.dirty = true;
  return true;
}

void handle_cfg_line(char* line) {
  // USB debug: cfg ssid|pass|url|slug|key VALUE  /  cfg save|wipe|show
  if (strncmp(line, "cfg ", 4) != 0 && strcmp(line, "cfg") != 0) {
    return;
  }
  char* rest = line + (strncmp(line, "cfg ", 4) == 0 ? 4 : 3);
  while (*rest == ' ') {
    rest++;
  }
  if (*rest == 0 || strcmp(rest, "show") == 0) {
    cfg_show();
    return;
  }
  if (strcmp(rest, "save") == 0) {
    g_state.net.from_nvs = nvs_cfg_save(g_state.net);
    g_state.net.dirty = true;
    Serial.println(g_state.net.from_nvs ? F("cfg saved") : F("cfg save failed"));
    return;
  }
  if (strcmp(rest, "wipe") == 0) {
    nvs_cfg_wipe(g_state.net);
    Serial.println(F("cfg wiped"));
    return;
  }
  if (strcmp(rest, "time") == 0) {
    cfg_time();
    return;
  }
  if (strncmp(rest, "quiet", 5) == 0) {
    cfg_quiet(rest + 5);
    return;
  }
  if (strncmp(rest, "ota ", 4) == 0) {
    ota_run(rest + 4);
    return;
  }
  char* sp = strchr(rest, ' ');
  if (sp == nullptr) {
    Serial.println(F("cfg ssid|pass|url|slug|key VALUE | quiet | ota | time"));
    return;
  }
  *sp = 0;
  const char* value = sp + 1;
  while (*value == ' ') {
    value++;
  }
  if (!cfg_set(rest, value)) {
    Serial.println(F("cfg unknown field"));
    return;
  }
  Serial.println(F("cfg set"));
}

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
  nvs_cfg_load(g_state.net);
  Serial.println(F("core2 af-direct boot"));
  cfg_show();
}

void protocol_mask_key(char* dst, size_t dst_len, const char* key) {
  if (dst == nullptr || dst_len == 0) {
    return;
  }
  dst[0] = 0;
  if (key == nullptr || key[0] == 0) {
    protocol_copy_trunc(dst, dst_len, "(empty)");
    return;
  }
  char buf[24];
  snprintf(buf, sizeof(buf), "afp_...%u", static_cast<unsigned>(strlen(key)));
  protocol_copy_trunc(dst, dst_len, buf);
}

void protocol_poll_serial() {
  // USB is debug-only (VISION.md). `cfg …` writes NVS; other RX is drained.
  while (Serial.available() > 0) {
    const int ch = Serial.read();
    if (ch < 0) {
      break;
    }
    if (ch == '\n' || ch == '\r') {
      if (g_line_len > 0) {
        g_line[g_line_len] = 0;
        handle_cfg_line(g_line);
      }
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

bool protocol_time_ok() { return time(nullptr) > 1700000000; }

bool protocol_quiet_hours() {
  const NetCfg& n = g_state.net;
  if (!n.quiet_en || n.quiet_start == n.quiet_end || !protocol_time_ok()) {
    return false;
  }
  const time_t now = time(nullptr);
  struct tm tmv = {};
  localtime_r(&now, &tmv);
  const int hm = tmv.tm_hour * 100 + tmv.tm_min;
  const int start = static_cast<int>(n.quiet_start);
  const int end = static_cast<int>(n.quiet_end);
  if (start < end) {
    return hm >= start && hm < end;
  }
  return hm >= start || hm < end;
}

bool protocol_muted() {
  if (g_state.mute_until_ms != 0) {
    if (static_cast<int32_t>(millis() - g_state.mute_until_ms) >= 0) {
      g_state.mute_until_ms = 0;
    } else {
      return true;
    }
  }
  return protocol_quiet_hours();
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
