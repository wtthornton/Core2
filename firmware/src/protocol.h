#pragma once

#include <Arduino.h>

#include "config.h"

enum class Screen : uint8_t { Hub = 0, Pulse = 1, Heat = 2, Trail = 3, Beam = 4, Count = 5 };

struct SnapState {
  bool have = false;
  bool reachable = false;
  bool ready = false;
  bool degraded = false;
  bool auth_ok = true;
  char ver[24] = "";
  int32_t inv = 0;
  float err = 0;
  float cost = 0;
  int32_t age_s = -1;
  char agent[24] = "";
  char link[8] = "none";
};

struct AlertState {
  bool have = false;
  bool acked = true;
  bool fire_haptic = false;
  char id[40] = "";
  char sev[12] = "";
  char src[28] = "";
  char agent[24] = "";
  char msg[80] = "";
};

struct SeriesState {
  bool have = false;
  uint8_t n_inv = 0;
  uint8_t n_err = 0;
  uint16_t inv[40] = {};
  uint16_t err[40] = {};
};

struct FailItem {
  char a[20] = "";
  char ago[8] = "";
};

struct FailState {
  bool have = false;
  int32_t n24 = 0;
  uint8_t n = 0;
  FailItem items[5] = {};
};

struct NetCfg {
  char ssid[33] = "";
  char pass[65] = "";
  char af_url[96] = "";
  char api_key[96] = "";
  char project_slug[48] = "";
  bool dirty = false;
};

struct AppState {
  SnapState snap;
  AlertState alert;
  SeriesState series;
  FailState fail;
  NetCfg net;
  Screen screen = Screen::Hub;
  uint32_t mute_until_ms = 0;
  bool dirty = true;
};

AppState& app_state();
void protocol_begin();
void protocol_poll_serial();
void protocol_set_alert(const char* id, const char* sev, const char* src, const char* agent,
                        const char* msg);
bool protocol_muted();
void protocol_ack();
void protocol_mute_15m();
void protocol_next_screen();
void protocol_prev_screen();
void protocol_set_screen(Screen screen);
void protocol_copy_trunc(char* dst, size_t dst_len, const char* src);
