#include "af_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <time.h>

#include "protocol.h"

namespace {

constexpr uint32_t kHttpTimeoutMs = 4000;
constexpr size_t kBodyMax = 4096;
int32_t g_prev_errors = -1;

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
  protocol_copy_trunc(s.link, sizeof(s.link), "wifi");
  s.reachable = health_code == 200;
  s.ready = s.reachable && ready_code == 200;
  s.age_s = s.reachable ? 0 : -1;
  if (!s.reachable) {
    s.degraded = false;
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

// GET /projects/{slug}/stats — project-scoped (afp_ works; fleet /stats/* does not).
void apply_project_stats(const String& body, int code) {
  AppState& st = app_state();
  SnapState& s = st.snap;
  if (!s.reachable || !s.auth_ok) {
    return;
  }
  if (code == 401 || code == 403) {
    s.auth_ok = false;
    protocol_copy_trunc(s.agent, sizeof(s.agent), "need afp_ key");
    st.dirty = true;
    return;
  }
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
  JsonArray by_agent = doc["by_agent"].as<JsonArray>();
  if (!by_agent.isNull() && by_agent.size() > 0) {
    protocol_copy_trunc(s.agent, sizeof(s.agent), by_agent[0]["agent"] | by_agent[0]["name"] | "");
  }
  st.dirty = true;
}

// GET /projects/{slug}/activity-series?days=7
void apply_activity_series(const String& body, int code) {
  if (code != 200) {
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    return;
  }
  SeriesState& s = app_state().series;
  s.have = true;
  s.n_inv = 0;
  s.n_err = 0;
  JsonArray points = doc["points"].as<JsonArray>();
  if (!points.isNull()) {
    for (JsonObject row : points) {
      if (s.n_inv >= 40) {
        break;
      }
      s.inv[s.n_inv++] = static_cast<uint16_t>(row["invocations"] | 0);
      s.err[s.n_err++] = static_cast<uint16_t>(row["errors"] | 0);
    }
  }
  app_state().dirty = true;
}

// Heat face: dual-meters error_count + recent invocations as rows.
void apply_heat(const String& meters_body, int meters_code, const String& inv_body, int inv_code) {
  FailState& f = app_state().fail;
  f.have = true;
  f.n = 0;
  f.n24 = 0;

  if (meters_code == 200) {
    JsonDocument doc;
    if (!deserializeJson(doc, meters_body)) {
      f.n24 = doc["consumption"]["error_count"] | 0;
    }
  }

  // Fallback: sum errors from already-parsed series last day if meters missing.
  if (meters_code != 200) {
    const SeriesState& ser = app_state().series;
    if (ser.n_err > 0) {
      f.n24 = ser.err[ser.n_err - 1];
    }
  }

  if (inv_code == 200) {
    JsonDocument doc;
    if (!deserializeJson(doc, inv_body)) {
      JsonArray items = doc["items"].as<JsonArray>();
      if (!items.isNull()) {
        for (JsonObject o : items) {
          if (f.n >= 5) {
            break;
          }
          const char* name = o["agent"] | o["config"] | o["name"] | "invoke";
          protocol_copy_trunc(f.items[f.n].a, sizeof(f.items[f.n].a), name);
          char ago[8];
          ago_label(o["created_at"] | o["timestamp"] | "", ago, sizeof(ago));
          protocol_copy_trunc(f.items[f.n].ago, sizeof(f.items[f.n].ago), ago);
          f.n++;
        }
      }
    }
  }

  if (g_prev_errors >= 0 && f.n24 > g_prev_errors) {
    const char* agent = f.n > 0 ? f.items[0].a : "core2";
    char id[40];
    char msg[80];
    snprintf(id, sizeof(id), "err-%ld", static_cast<long>(f.n24));
    snprintf(msg, sizeof(msg), "errors %ld->%ld", static_cast<long>(g_prev_errors),
             static_cast<long>(f.n24));
    protocol_set_alert(id, "error", "project.errors", agent, msg);
  }
  g_prev_errors = f.n24;
  app_state().dirty = true;
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
  if (n.project_slug[0] == 0) {
    return;
  }

  char path[96];
  snprintf(path, sizeof(path), "/projects/%s", n.project_slug);
  String proj_body;
  apply_project_auth(http_get(path, proj_body));
  if (!app_state().snap.auth_ok) {
    return;
  }

  snprintf(path, sizeof(path), "/projects/%s/stats", n.project_slug);
  String stats_body;
  apply_project_stats(stats_body, http_get(path, stats_body));

  snprintf(path, sizeof(path), "/projects/%s/activity-series?days=7", n.project_slug);
  String series_body;
  apply_activity_series(series_body, http_get(path, series_body));

  snprintf(path, sizeof(path), "/projects/%s/dual-meters", n.project_slug);
  String meters_body;
  const int meters_code = http_get(path, meters_body);

  snprintf(path, sizeof(path), "/projects/%s/invocations?limit=5", n.project_slug);
  String inv_body;
  const int inv_code = http_get(path, inv_body);
  apply_heat(meters_body, meters_code, inv_body, inv_code);
}
