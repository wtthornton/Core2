#include "af_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <cstdio>
#include <ctime>
#include <time.h>

#include "protocol.h"

namespace {

constexpr uint32_t kHttpTimeoutMs = 4000;
constexpr size_t kBodyMax = 4096;
int32_t g_prev_n24 = -1;

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

int http_get(const char* path, String& body_out) {
  body_out = "";
  const NetCfg& n = app_state().net;
  if (n.af_url[0] == 0 || WiFi.status() != WL_CONNECTED) {
    return 0;
  }
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  http.begin(join_url(n.af_url, path));
  if (n.api_key[0] != 0) {
    http.addHeader("Authorization", String("Bearer ") + n.api_key);
  }
  http.addHeader("Accept", "application/json");
  const int code = http.GET();
  if (code > 0) {
    body_out = http.getString();
    if (body_out.length() > kBodyMax) {
      body_out = body_out.substring(0, kBodyMax);
    }
  }
  http.end();
  return code;
}

void ago_label(const char* ts, char* out, size_t out_len) {
  if (out_len == 0) {
    return;
  }
  out[0] = '?';
  out[1] = 0;
  if (ts == nullptr || ts[0] == 0) {
    return;
  }
  // Expect ISO-8601; compute rough age from ESP time if set, else "?".
  struct tm tmv = {};
  if (sscanf(ts, "%d-%d-%dT%d:%d:%d", &tmv.tm_year, &tmv.tm_mon, &tmv.tm_mday, &tmv.tm_hour,
             &tmv.tm_min, &tmv.tm_sec) < 6) {
    return;
  }
  tmv.tm_year -= 1900;
  tmv.tm_mon -= 1;
  const time_t when = mktime(&tmv);
  time_t now = time(nullptr);
  if (when <= 0 || now <= 0) {
    protocol_copy_trunc(out, out_len, "?");
    return;
  }
  long sec = static_cast<long>(difftime(now, when));
  if (sec < 0) {
    sec = 0;
  }
  if (sec < 60) {
    snprintf(out, out_len, "%lds", sec);
  } else if (sec < 3600) {
    snprintf(out, out_len, "%ldm", sec / 60);
  } else if (sec < 86400) {
    snprintf(out, out_len, "%ldh", sec / 3600);
  } else {
    snprintf(out, out_len, "%ldd", sec / 86400);
  }
}

void apply_health_ready(const String& health_body, int health_code, int ready_code) {
  AppState& st = app_state();
  SnapState& s = st.snap;
  s.have = true;
  s.link[0] = 0;
  protocol_copy_trunc(s.link, sizeof(s.link), "wifi");
  s.reachable = health_code == 200;
  s.ready = s.reachable && ready_code == 200;
  s.age_s = s.reachable ? 0 : -1;
  if (!s.reachable) {
    s.degraded = false;
    s.auth_ok = true;
    s.ver[0] = 0;
    st.dirty = true;
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, health_body)) {
    st.dirty = true;
    return;
  }
  protocol_copy_trunc(s.ver, sizeof(s.ver), doc["version"] | "");
  const char* status = doc["status"] | "";
  s.degraded = (strcmp(status, "degraded") == 0);
  st.dirty = true;
}

void apply_summary(const String& body, int code) {
  AppState& st = app_state();
  SnapState& s = st.snap;
  if (!s.reachable) {
    return;
  }
  if (code == 401 || code == 403) {
    s.auth_ok = false;
    protocol_copy_trunc(s.agent, sizeof(s.agent), "need afp_ key");
    st.dirty = true;
    return;
  }
  s.auth_ok = true;
  if (code != 200) {
    st.dirty = true;
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    return;
  }
  s.inv = doc["total_invocations"] | 0;
  s.err = doc["error_rate"] | 0.0f;
  s.cost = doc["total_cost_usd"] | 0.0f;
  JsonArray top = doc["top_configs"].as<JsonArray>();
  if (!top.isNull() && top.size() > 0) {
    protocol_copy_trunc(s.agent, sizeof(s.agent), top[0]["config"] | "");
  } else if (s.auth_ok) {
    s.agent[0] = 0;
  }
  st.dirty = true;
}

void downsample_fill(JsonArray by_day, uint16_t* out, uint8_t* n_out, bool invocations) {
  constexpr uint8_t kPoints = 7;
  uint16_t raw[64] = {};
  uint8_t nraw = 0;
  if (!by_day.isNull()) {
    // AF returns newest-first in many builds; reverse into oldest-first.
    const size_t len = by_day.size();
    for (int i = static_cast<int>(len) - 1; i >= 0 && nraw < 64; --i) {
      JsonObject row = by_day[i].as<JsonObject>();
      if (invocations) {
        raw[nraw++] = static_cast<uint16_t>(row["invocations"] | 0);
      } else {
        raw[nraw++] = 0;
      }
    }
  }
  if (nraw == 0) {
    for (uint8_t i = 0; i < kPoints; ++i) {
      out[i] = 0;
    }
    *n_out = kPoints;
    return;
  }
  if (nraw <= kPoints) {
    for (uint8_t i = 0; i < nraw; ++i) {
      out[i] = raw[i];
    }
    for (uint8_t i = nraw; i < kPoints; ++i) {
      out[i] = 0;
    }
    *n_out = kPoints;
    return;
  }
  for (uint8_t i = 0; i < kPoints; ++i) {
    const int start = (i * nraw) / kPoints;
    const int end = ((i + 1) * nraw) / kPoints;
    uint32_t sum = 0;
    int count = 0;
    for (int j = start; j < end; ++j) {
      sum += raw[j];
      count++;
    }
    out[i] = count ? static_cast<uint16_t>(sum / count) : 0;
  }
  *n_out = kPoints;
}

void apply_dashboard(const String& body, int code) {
  if (code != 200) {
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    return;
  }
  SeriesState& s = app_state().series;
  s.have = true;
  downsample_fill(doc["by_day"].as<JsonArray>(), s.inv, &s.n_inv, true);
  // Error sparkline: count is_error in recent_invocations by day bucket — simplified to zeros + last.
  for (uint8_t i = 0; i < s.n_inv; ++i) {
    s.err[i] = 0;
  }
  s.n_err = s.n_inv;
  JsonObject total = doc["total"].as<JsonObject>();
  if (!total.isNull() && s.n_err > 0) {
    s.err[s.n_err - 1] = static_cast<uint16_t>(total["error_count"] | 0);
  }
  app_state().dirty = true;
}

void apply_failures(const String& body, int code) {
  if (code != 200) {
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    return;
  }
  FailState& f = app_state().fail;
  f.have = true;
  f.n24 = doc["last_24h_count"] | 0;
  f.n = 0;
  JsonArray items = doc["items"].as<JsonArray>();
  if (!items.isNull()) {
    for (JsonObject o : items) {
      if (f.n >= 5) {
        break;
      }
      protocol_copy_trunc(f.items[f.n].a, sizeof(f.items[f.n].a), o["agent"] | "");
      char ago[8];
      ago_label(o["timestamp"] | "", ago, sizeof(ago));
      protocol_copy_trunc(f.items[f.n].ago, sizeof(f.items[f.n].ago), ago);
      f.n++;
    }
  }
  if (g_prev_n24 >= 0 && f.n24 > g_prev_n24) {
    const char* agent = f.n > 0 ? f.items[0].a : "af";
    char id[40];
    char msg[80];
    snprintf(id, sizeof(id), "fail-%ld", static_cast<long>(f.n24));
    snprintf(msg, sizeof(msg), "failures 24h %ld->%ld", static_cast<long>(g_prev_n24),
             static_cast<long>(f.n24));
    protocol_set_alert(id, "error", "stats.failures", agent, msg);
  }
  g_prev_n24 = f.n24;
  app_state().dirty = true;
}

void apply_project_auth(int code) {
  AppState& st = app_state();
  SnapState& s = st.snap;
  if (!s.reachable) {
    return;
  }
  if (code == 401 || code == 403) {
    s.auth_ok = false;
    protocol_copy_trunc(s.agent, sizeof(s.agent), "need afp_ key");
    st.dirty = true;
    return;
  }
  if (code == 200) {
    s.auth_ok = true;
  }
  st.dirty = true;
}

}  // namespace

void af_client_poll() {
  if (WiFi.status() != WL_CONNECTED) {
    SnapState& s = app_state().snap;
    s.have = true;
    s.reachable = false;
    s.ready = false;
    s.age_s = -1;
    protocol_copy_trunc(s.link, sizeof(s.link), "wifi");
    app_state().dirty = true;
    return;
  }

  String body;
  const int health_code = http_get("/health", body);
  String ready_body;
  const int ready_code = http_get("/ready", ready_body);
  apply_health_ready(body, health_code, ready_code);

  const NetCfg& n = app_state().net;
  if (n.project_slug[0] != 0) {
    char proj_path[72];
    snprintf(proj_path, sizeof(proj_path), "/projects/%s", n.project_slug);
    String proj_body;
    apply_project_auth(http_get(proj_path, proj_body));
  }

  String summary;
  const int sum_code = http_get("/stats/summary", summary);
  apply_summary(summary, sum_code);

  String dash;
  const int dash_code = http_get("/stats/dashboard?days=7", dash);
  apply_dashboard(dash, dash_code);

  String fail;
  const int fail_code = http_get("/stats/failures?limit=5", fail);
  apply_failures(fail, fail_code);
}
