#include "nvs_cfg.h"

#include <Preferences.h>
#include <cstring>

namespace {

constexpr const char* kNs = "core2";

void fill_defaults(NetCfg& net) {
  protocol_copy_trunc(net.ssid, sizeof(net.ssid), CORE2_WIFI_SSID);
  protocol_copy_trunc(net.pass, sizeof(net.pass), CORE2_WIFI_PASS);
  protocol_copy_trunc(net.af_url, sizeof(net.af_url), AF_BASE_URL);
  protocol_copy_trunc(net.api_key, sizeof(net.api_key), AF_API_KEY);
  protocol_copy_trunc(net.project_slug, sizeof(net.project_slug), AF_PROJECT_SLUG);
  net.from_nvs = false;
  net.dirty = false;
  net.quiet_en = false;
  net.quiet_start = 2200;
  net.quiet_end = 700;
}

void overlay_key(Preferences& prefs, const char* key, char* dst, size_t dst_len) {
  if (!prefs.isKey(key)) {
    return;
  }
  const String v = prefs.getString(key, "");
  protocol_copy_trunc(dst, dst_len, v.c_str());
}

void overlay_quiet(Preferences& prefs, NetCfg& net) {
  if (prefs.isKey("qen")) {
    net.quiet_en = prefs.getBool("qen", false);
  }
  if (prefs.isKey("qs")) {
    net.quiet_start = prefs.getUShort("qs", net.quiet_start);
  }
  if (prefs.isKey("qe")) {
    net.quiet_end = prefs.getUShort("qe", net.quiet_end);
  }
}

}  // namespace

void nvs_cfg_load(NetCfg& net) {
  fill_defaults(net);
  Preferences prefs;
  // RW open creates an empty namespace on first boot so Preferences does not
  // log nvs_open NOT_FOUND (read-only miss).
  if (!prefs.begin(kNs, false)) {
    return;
  }
  overlay_quiet(prefs, net);
  const bool have = prefs.isKey("ssid") || prefs.isKey("pass") || prefs.isKey("url") ||
                    prefs.isKey("slug") || prefs.isKey("key");
  if (!have) {
    prefs.end();
    return;
  }
  overlay_key(prefs, "ssid", net.ssid, sizeof(net.ssid));
  overlay_key(prefs, "pass", net.pass, sizeof(net.pass));
  overlay_key(prefs, "url", net.af_url, sizeof(net.af_url));
  overlay_key(prefs, "slug", net.project_slug, sizeof(net.project_slug));
  overlay_key(prefs, "key", net.api_key, sizeof(net.api_key));
  net.from_nvs = true;
  prefs.end();
}

bool nvs_cfg_save(const NetCfg& net) {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return false;
  }
  prefs.putString("ssid", net.ssid);
  prefs.putString("pass", net.pass);
  prefs.putString("url", net.af_url);
  prefs.putString("slug", net.project_slug);
  prefs.putString("key", net.api_key);
  prefs.putBool("qen", net.quiet_en);
  prefs.putUShort("qs", net.quiet_start);
  prefs.putUShort("qe", net.quiet_end);
  prefs.end();
  return true;
}

bool nvs_cfg_save_quiet(const NetCfg& net) {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) {
    return false;
  }
  prefs.putBool("qen", net.quiet_en);
  prefs.putUShort("qs", net.quiet_start);
  prefs.putUShort("qe", net.quiet_end);
  prefs.end();
  return true;
}

void nvs_cfg_wipe(NetCfg& net) {
  Preferences prefs;
  if (prefs.begin(kNs, false)) {
    prefs.clear();
    prefs.end();
  }
  fill_defaults(net);
  net.dirty = true;
}
