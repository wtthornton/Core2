#include "net.h"

#include <WiFi.h>

#include "af_client.h"
#include "protocol.h"

namespace {

uint32_t g_last_poll_ms = 0;
uint32_t g_last_wifi_try_ms = 0;
bool g_wifi_started = false;

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

}  // namespace

void net_begin() { wifi_start(); }

void net_poll() {
  AppState& st = app_state();
  if (st.net.dirty) {
    st.net.dirty = false;
    wifi_start();
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
  if (millis() - g_last_poll_ms < kPollMs) {
    return;
  }
  g_last_poll_ms = millis();
  af_client_poll();
}
