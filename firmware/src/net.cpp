#include "net.h"

#include <WiFi.h>
#include <cstdlib>
#include <time.h>

#include "af_client.h"
#include "af_sse.h"
#include "protocol.h"

namespace {

uint32_t g_last_poll_ms = 0;
uint32_t g_last_wifi_try_ms = 0;
bool g_wifi_started = false;
bool g_ntp_started = false;

constexpr uint32_t kPollMs = 5000;
constexpr uint32_t kWifiRetryMs = 15000;

void wifi_start() {
  const NetCfg& n = app_state().net;
  if (n.ssid[0] == 0) {
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(n.ssid, n.pass);
  g_wifi_started = true;
  g_last_wifi_try_ms = millis();
}

void maybe_ntp() {
  if (g_ntp_started || WiFi.status() != WL_CONNECTED) {
    return;
  }
  g_ntp_started = true;
  setenv("TZ", CORE2_TZ, 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  Serial.println(F("ntp start"));
}

}  // namespace

void net_begin() { wifi_start(); }

void net_reconnect() {
  WiFi.disconnect(true, true);
  delay(50);
  g_wifi_started = false;
  g_ntp_started = false;
  af_sse_reset();
  wifi_start();
}

void net_poll() {
  AppState& st = app_state();
  if (st.net.dirty) {
    st.net.dirty = false;
    net_reconnect();
  }
  if (!g_wifi_started && st.net.ssid[0] != 0) {
    wifi_start();
  }
  if (st.net.ssid[0] == 0) {
    SnapState& s = st.snap;
    if (!s.have || s.reachable) {
      s.have = true;
      s.reachable = false;
      s.ready = false;
      s.age_s = -1;
      protocol_copy_trunc(s.agent, sizeof(s.agent), "set WIFI SSID");
      protocol_copy_trunc(s.link, sizeof(s.link), "none");
      st.dirty = true;
    }
    return;
  }
  if (g_wifi_started && WiFi.status() != WL_CONNECTED) {
    SnapState& s = st.snap;
    s.have = true;
    s.reachable = false;
    s.ready = false;
    s.age_s = -1;
    protocol_copy_trunc(s.link, sizeof(s.link), "wifi");
    protocol_copy_trunc(s.agent, sizeof(s.agent), "joining Wi-Fi");
    st.dirty = true;
    if (millis() - g_last_wifi_try_ms >= kWifiRetryMs) {
      wifi_start();
    }
    return;
  }
  maybe_ntp();
  if (protocol_time_ok()) {
    static bool ntp_ok = false;
    if (!ntp_ok) {
      ntp_ok = true;
      Serial.println(F("ntp ok"));
    }
  }
  if (millis() - g_last_poll_ms < kPollMs && !af_client_busy()) {
    return;
  }
  if (!af_client_busy()) {
    g_last_poll_ms = millis();
  }
  af_client_poll();
}
