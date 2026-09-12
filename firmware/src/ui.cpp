#include "ui.h"

#include <M5Unified.h>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "protocol.h"

namespace {

M5Canvas g_canvas(&M5.Display);
uint32_t g_last_draw_ms = 0;
int16_t g_touch_x0 = -1;
uint32_t g_face_anim_ms = 0;
float g_cube_yaw = 0.f;
uint8_t g_last_face = 255;

constexpr int kScreenW = 320;  // Core2 ILI9342C
constexpr int kScreenH = 240;
constexpr int kBtnH = 40;      // soft keys (~1/6 of height)
constexpr int kStripH = 46;    // face tabs — touch-sized on 320x240
constexpr int kTabPad = 3;
constexpr int kTabGap = 3;
constexpr uint16_t kBg0 = 0x0821;
constexpr uint16_t kBg1 = 0x0C43;
constexpr uint16_t kPanel = 0x1165;
constexpr uint16_t kPanelHi = 0x1CE8;
constexpr uint16_t kShadow = 0x0000;
constexpr uint16_t kCyan = 0x07FD;
constexpr uint16_t kTeal = 0x045A;
constexpr uint16_t kAmber = 0xFCE0;
constexpr uint16_t kCoral = 0xF2AA;
constexpr uint16_t kMint = 0x07F0;
constexpr uint16_t kInk = 0x9CF3;
constexpr uint16_t kMute = 0x528A;
constexpr uint16_t kWhite = 0xFFFF;

const char* kFaceName[] = {"Hub", "Pulse", "Heat", "Trail", "Beam"};

uint16_t status_color(const SnapState& s) {
  if (!s.have || !s.reachable) {
    return kCoral;
  }
  if (!s.ready || !s.auth_ok) {
    return kAmber;
  }
  if (s.degraded) {
    return kAmber;
  }
  return kMint;
}

const char* status_label(const SnapState& s) {
  if (!s.have) {
    return "Waiting";
  }
  if (!s.reachable) {
    return "Wi-Fi";
  }
  if (!s.ready) {
    return "Booting";
  }
  if (!s.auth_ok) {
    return "Need key";
  }
  if (s.degraded) {
    return "Soft";
  }
  return "Live";
}

void fill_bg(M5Canvas& c, int w, int h) {
  for (int y = 0; y < h; ++y) {
    const uint16_t col = (y * 2 < h) ? kBg0 : kBg1;
    c.drawFastHLine(0, y, w, col);
  }
}

void panel(M5Canvas& c, int x, int y, int w, int h, uint16_t fill = kPanel) {
  c.fillSmoothRoundRect(x + 2, y + 3, w, h, 10, kShadow);
  c.fillSmoothRoundRect(x, y, w, h, 10, fill);
}

void label_sm(M5Canvas& c, int x, int y, const char* t, uint16_t col, uint16_t bg) {
  c.setTextDatum(top_left);
  c.setTextSize(1);
  c.setTextColor(col, bg);
  c.setCursor(x, y);
  c.print(t);
}

void draw_iso_cube(M5Canvas& c, int cx, int cy, int s, uint8_t face, float yaw) {
  const float rad = yaw * 0.0174533f;
  const float ca = cosf(rad);
  const float sa = sinf(rad);
  struct Pt {
    int x;
    int y;
  };
  auto vx = [&](float x, float y, float z) -> Pt {
    const float xr = x * ca - z * sa;
    const float zr = x * sa + z * ca;
    return Pt{cx + static_cast<int>((xr - zr) * s * 0.55f),
              cy + static_cast<int>((xr + zr) * s * 0.28f - y * s * 0.72f)};
  };

  const Pt t0 = vx(-1, 1, -1);
  const Pt t1 = vx(1, 1, -1);
  const Pt t2 = vx(1, 1, 1);
  const Pt t3 = vx(-1, 1, 1);
  const Pt f0 = vx(-1, -1, -1);
  const Pt f1 = vx(1, -1, -1);
  const Pt f2 = vx(1, -1, 1);
  const Pt f3 = vx(-1, -1, 1);

  auto tri = [&](Pt a, Pt b, Pt d, uint16_t col) {
    c.fillTriangle(a.x, a.y, b.x, b.y, d.x, d.y, col);
  };

  const uint16_t top = (face == 3) ? kCyan : kTeal;
  const uint16_t left = (face == 2) ? kCoral : kPanelHi;
  const uint16_t right = (face == 1) ? kAmber : 0x14C6;
  const uint16_t front = (face == 0 || face == 4) ? kMint : kPanel;

  tri(t0, t1, t2, top);
  tri(t0, t2, t3, top);
  tri(t3, f3, f0, left);
  tri(t3, f0, t0, left);
  tri(t1, f1, f2, right);
  tri(t1, f2, t2, right);
  tri(t0, t1, f1, front);
  tri(t0, f1, f0, front);

  c.drawSmoothLine(t0.x, t0.y, t1.x, t1.y, kInk);
  c.drawSmoothLine(t1.x, t1.y, t2.x, t2.y, kInk);
  c.drawSmoothLine(t2.x, t2.y, t3.x, t3.y, kInk);
  c.drawSmoothLine(t3.x, t3.y, t0.x, t0.y, kInk);
  c.drawSmoothLine(f0.x, f0.y, f1.x, f1.y, kInk);
  c.drawSmoothLine(t0.x, t0.y, f0.x, f0.y, kInk);
  c.drawSmoothLine(t1.x, t1.y, f1.x, f1.y, kInk);
}

void draw_orb(M5Canvas& c, int cx, int cy, int r, uint16_t col, uint32_t now) {
  const float phase = (now % 2200) / 2200.f;
  const float breath = 0.5f + 0.5f * sinf(phase * 6.28318f);
  for (int i = 3; i >= 1; --i) {
    const int rr = r + static_cast<int>(i * (9 + breath * 5));
    c.drawCircle(cx, cy, rr, (i == 1) ? col : kTeal);
  }
  c.fillSmoothCircle(cx + 1, cy + 2, r, kShadow);
  c.fillSmoothCircle(cx, cy, r, col);
  c.fillSmoothCircle(cx - r / 3, cy - r / 3, max(3, r / 5), kWhite);
}

void draw_bars(M5Canvas& c, int x, int y, int w, int h, const uint16_t* data, uint8_t n,
               uint16_t color) {
  panel(c, x, y, w, h, kPanel);
  c.drawFastHLine(x + 8, y + h - 10, w - 16, kMute);

  if (n == 0) {
    const uint32_t t = millis();
    const int bars = 28;
    const int bw = max(2, (w - 24) / bars);
    for (int i = 0; i < bars; ++i) {
      const float wave = 0.22f + 0.55f * (0.5f + 0.5f * sinf((t / 320.f) + i * 0.38f));
      const int bh = max(3, static_cast<int>((h - 28) * wave));
      const int bx = x + 12 + i * bw;
      c.fillSmoothRoundRect(bx, y + h - 12 - bh, bw - 1, bh, 2, kTeal);
    }
    c.setTextDatum(middle_center);
    c.setTextColor(kInk, kPanel);
    c.drawString("listening", x + w / 2, y + 16);
    c.setTextDatum(top_left);
    return;
  }

  uint16_t mx = 1;
  for (uint8_t i = 0; i < n; ++i) {
    if (data[i] > mx) {
      mx = data[i];
    }
  }
  const int gap = 4;
  const int bar_w = max(5, (w - 24 - gap * (n - 1)) / n);
  for (uint8_t i = 0; i < n; ++i) {
    const int bh = 5 + static_cast<int>(data[i]) * (h - 28) / mx;
    const int bx = x + 12 + i * (bar_w + gap);
    const int by = y + h - 12 - bh;
    c.fillSmoothRoundRect(bx, by, bar_w, bh, 3, color);
    c.fillRect(bx, by, bar_w, max(2, bh / 4), kWhite);
  }
}

void draw_sparkline(M5Canvas& c, int x, int y, int w, int h, const uint16_t* data, uint8_t n,
                    uint16_t color) {
  panel(c, x, y, w, h, kPanel);
  if (n < 2) {
    draw_bars(c, x, y, w, h, data, n, color);
    return;
  }
  uint16_t mx = 1;
  for (uint8_t i = 0; i < n; ++i) {
    if (data[i] > mx) {
      mx = data[i];
    }
  }
  int prevx = x + 10;
  int prevy = y + h - 12;
  for (uint8_t i = 0; i < n; ++i) {
    const int px = x + 10 + (i * (w - 20)) / (n - 1);
    const int py = y + h - 12 - static_cast<int>(data[i]) * (h - 28) / mx;
    if (i > 0) {
      c.fillTriangle(prevx, y + h - 12, prevx, prevy, px, py, kTeal);
      c.fillTriangle(prevx, y + h - 12, px, py, px, y + h - 12, kTeal);
      c.drawWideLine(prevx, prevy, px, py, 2.0f, color);
    }
    c.fillSmoothCircle(px, py, 2, kWhite);
    prevx = px;
    prevy = py;
  }
}

void draw_face_strip(M5Canvas& c, Screen screen, int w) {
  const int n = static_cast<int>(Screen::Count);
  const int tab_h = kStripH - 6;
  const int tw = (w - kTabPad * 2 - kTabGap * (n - 1)) / n;
  c.fillRect(0, 0, w, kStripH, kBg0);
  for (int i = 0; i < n; ++i) {
    const bool on = static_cast<int>(screen) == i;
    const int x = kTabPad + i * (tw + kTabGap);
    const int y = 3;
    c.fillSmoothRoundRect(x + 1, y + 2, tw, tab_h, 10, kShadow);
    c.fillSmoothRoundRect(x, y, tw, tab_h, 10, on ? kTeal : kPanel);
    if (on) {
      c.fillSmoothRoundRect(x + 8, y + tab_h - 5, tw - 16, 3, 1, kCyan);
    }
    c.setTextDatum(middle_center);
    c.setTextColor(on ? kWhite : kInk, on ? kTeal : kPanel);
    c.setTextSize(1);
    c.drawString(kFaceName[i], x + tw / 2, y + tab_h / 2 - (on ? 1 : 0));
  }
  c.setTextDatum(top_left);
}

void draw_soft_keys(M5Canvas& c, int w, int h) {
  const int y = h - kBtnH;
  c.fillRect(0, y - 2, w, 2, kBg1);
  const char* labels[3] = {"<", "Quiet", ">"};
  const int bw = w / 3;
  for (int i = 0; i < 3; ++i) {
    const int x = i * bw + 5;
    c.fillSmoothRoundRect(x + 1, y + 4, bw - 10, kBtnH - 8, 12, kShadow);
    c.fillSmoothRoundRect(x, y + 3, bw - 10, kBtnH - 8, 12, kPanelHi);
    c.setTextDatum(middle_center);
    c.setTextColor(kWhite, kPanelHi);
    c.setTextSize(i == 1 ? 1 : 2);
    c.drawString(labels[i], x + (bw - 10) / 2, y + kBtnH / 2);
  }
  c.setTextDatum(top_left);
  c.setTextSize(1);
}

void draw_hub(M5Canvas& c, const AppState& st, int w, int bat) {
  const SnapState& s = st.snap;
  const uint32_t now = millis();
  const int top = kStripH + 4;
  const int card_h = 92;

  panel(c, 6, top, w - 12, card_h, kPanel);
  draw_orb(c, 52, top + 46, 22, status_color(s), now);
  draw_iso_cube(c, w - 52, top + 46, 26, static_cast<uint8_t>(st.screen), g_cube_yaw);

  c.setTextColor(kWhite, kPanel);
  c.setTextSize(2);
  c.setCursor(90, top + 16);
  c.print(status_label(s));
  c.setTextSize(1);
  label_sm(c, 90, top + 40, s.ver[0] ? s.ver : "AF --", kInk, kPanel);
  char meta[40];
  snprintf(meta, sizeof(meta), "%d%%  %s", bat, s.link[0] ? s.link : "--");
  label_sm(c, 90, top + 56, meta, kMute, kPanel);
  if (st.mute_until_ms) {
    label_sm(c, 90, top + 72, "quiet on", kAmber, kPanel);
  } else if (s.agent[0]) {
    label_sm(c, 90, top + 72, s.agent, kCyan, kPanel);
  }

  char a[16], b[16], d[16];
  if (!s.auth_ok || !s.reachable) {
    snprintf(a, sizeof(a), "--");
    snprintf(b, sizeof(b), "--");
    snprintf(d, sizeof(d), "--");
  } else {
    snprintf(a, sizeof(a), "%ld", static_cast<long>(s.inv));
    snprintf(b, sizeof(b), "%.0f%%", s.err * 100.f);
    snprintf(d, sizeof(d), "$%.1f", s.cost);
  }
  const int y = top + card_h + 6;
  const int tw = (w - 24) / 3;
  auto tile = [&](int i, const char* title, const char* val, uint16_t accent) {
    const int x = 6 + i * (tw + 4);
    panel(c, x, y, tw, 40);
    c.fillSmoothRoundRect(x, y, 5, 40, 2, accent);
    label_sm(c, x + 12, y + 6, title, kMute, kPanel);
    c.setTextColor(kWhite, kPanel);
    c.setTextSize(2);
    c.setCursor(x + 12, y + 18);
    c.print(val);
    c.setTextSize(1);
  };
  tile(0, "INV", a, kCyan);
  tile(1, "ERR", b, kAmber);
  tile(2, "USD", d, kMint);
}

void draw_pulse(M5Canvas& c, const AppState& st, int w) {
  const int top = kStripH + 4;
  const int avail = kScreenH - kStripH - kBtnH - 8;
  panel(c, 6, top, w - 12, 48);
  label_sm(c, 16, top + 6, "INVOCATIONS", kMute, kPanel);
  c.setTextColor(kWhite, kPanel);
  c.setTextSize(3);
  c.setCursor(16, top + 20);
  if (st.snap.auth_ok && st.snap.reachable) {
    c.printf("%ld", static_cast<long>(st.snap.inv));
  } else {
    c.print("--");
  }
  c.setTextSize(1);
  c.setTextDatum(middle_right);
  c.setTextColor(kCyan, kPanel);
  c.drawString(st.snap.agent[0] ? st.snap.agent : "agent", w - 18, top + 34);
  c.setTextDatum(top_left);
  draw_bars(c, 6, top + 54, w - 12, max(48, avail - 54), st.series.inv, st.series.n_inv, kCyan);
}

void draw_heat(M5Canvas& c, const AppState& st, int w) {
  const FailState& f = st.fail;
  const int top = kStripH + 4;
  const int avail = kScreenH - kStripH - kBtnH - 8;
  panel(c, 6, top, w / 2 - 9, 56);
  label_sm(c, 16, top + 6, "FAILS 24H", kMute, kPanel);
  c.setTextColor(kCoral, kPanel);
  c.setTextSize(3);
  c.setCursor(16, top + 24);
  c.printf("%ld", static_cast<long>(f.n24));
  c.setTextSize(1);

  panel(c, w / 2 + 3, top, w / 2 - 9, 56);
  label_sm(c, w / 2 + 13, top + 6, "ERROR %", kMute, kPanel);
  c.setTextColor(kAmber, kPanel);
  c.setTextSize(3);
  c.setCursor(w / 2 + 13, top + 24);
  if (st.snap.auth_ok && st.snap.reachable) {
    c.printf("%.0f", st.snap.err * 100.f);
  } else {
    c.print("--");
  }
  c.setTextSize(1);

  int y = top + 62;
  const int list_h = avail - 62;
  if (!f.have || f.n == 0) {
    panel(c, 6, y, w - 12, list_h);
    c.setTextDatum(middle_center);
    c.setTextColor(kInk, kPanel);
    c.drawString("All quiet", w / 2, y + list_h / 2 - 8);
    c.setTextColor(kMute, kPanel);
    c.drawString("no recent failures", w / 2, y + list_h / 2 + 10);
    c.setTextDatum(top_left);
    return;
  }
  for (uint8_t i = 0; i < f.n; ++i) {
    if (y + 22 > top + avail) {
      break;
    }
    panel(c, 6, y, w - 12, 20);
    c.fillSmoothCircle(18, y + 10, 4, kCoral);
    c.setTextColor(kWhite, kPanel);
    c.setCursor(30, y + 6);
    c.print(f.items[i].a);
    c.setTextDatum(middle_right);
    c.setTextColor(kMute, kPanel);
    c.drawString(f.items[i].ago, w - 14, y + 10);
    c.setTextDatum(top_left);
    y += 22;
  }
}

void draw_trail(M5Canvas& c, const AppState& st, int w) {
  const int top = kStripH + 4;
  const int avail = kScreenH - kStripH - kBtnH - 8;
  const int spark_h = avail / 2 - 12;
  const int bars_h = avail - spark_h - 28;
  label_sm(c, 10, top, "7-DAY INVOCATIONS", kCyan, kBg0);
  draw_sparkline(c, 6, top + 12, w - 12, spark_h, st.series.inv, st.series.n_inv, kCyan);
  label_sm(c, 10, top + 16 + spark_h, "7-DAY ERRORS", kAmber, kBg0);
  draw_bars(c, 6, top + 28 + spark_h, w - 12, max(40, bars_h), st.series.err, st.series.n_err,
            kAmber);
}

void draw_beam(M5Canvas& c, const AppState& st, int w) {
  const SnapState& s = st.snap;
  const NetCfg& n = st.net;
  const int top = kStripH + 4;
  const int avail = kScreenH - kStripH - kBtnH - 8;
  const int cube_h = avail - 52;
  panel(c, 6, top, w - 12, cube_h);
  draw_iso_cube(c, w / 2, top + cube_h / 2, 36, static_cast<uint8_t>(st.screen), g_cube_yaw);

  panel(c, 6, top + cube_h + 4, w - 12, 48);
  char host[48];
  snprintf(host, sizeof(host), "%.40s", n.af_url[0] ? n.af_url : "--");
  label_sm(c, 14, top + cube_h + 10, host, kInk, kPanel);
  char line[48];
  snprintf(line, sizeof(line), "%s  ·  %s", n.project_slug[0] ? n.project_slug : "--",
           s.link[0] ? s.link : "--");
  label_sm(c, 14, top + cube_h + 26, line, kMute, kPanel);
  c.setTextColor(s.auth_ok ? kMint : kAmber, kPanel);
  c.setCursor(14, top + cube_h + 40);
  c.printf("%s  fw %s", s.auth_ok ? "auth ok" : "need afp_ key", CORE2_FW_VERSION);
}

}  // namespace

void ui_begin() {
  g_canvas.setPsram(true);
  g_canvas.setColorDepth(16);
  g_canvas.createSprite(M5.Display.width(), M5.Display.height());
  g_face_anim_ms = millis();
  M5.Display.setBrightness(150);
}

bool ui_handle_touch(int x, int y) {
  const int w = M5.Display.width();
  const int h = M5.Display.height();

  if (y < kStripH) {
    const int n = static_cast<int>(Screen::Count);
    const int tw = (w - kTabPad * 2 - kTabGap * (n - 1)) / n;
    if (x < kTabPad) {
      return false;
    }
    const int idx = (x - kTabPad) / (tw + kTabGap);
    if (idx >= 0 && idx < n) {
      protocol_set_screen(static_cast<Screen>(idx));
      g_face_anim_ms = millis();
      return true;
    }
  }
  if (y >= h - kBtnH) {
    const int btn = x * 3 / w;
    if (btn == 0) {
      protocol_prev_screen();
      g_face_anim_ms = millis();
      return true;
    }
    if (btn == 1) {
      if (app_state().alert.have && !app_state().alert.acked) {
        protocol_ack();
      } else {
        protocol_mute_15m();
      }
      return true;
    }
    if (btn == 2) {
      protocol_next_screen();
      g_face_anim_ms = millis();
      return true;
    }
  }
  if (g_touch_x0 < 0) {
    g_touch_x0 = x;
  }
  return false;
}

void ui_touch_release(int x, int y) {
  (void)y;
  if (g_touch_x0 >= 0) {
    const int dx = x - g_touch_x0;
    if (dx > 36) {
      protocol_prev_screen();
      g_face_anim_ms = millis();
    } else if (dx < -36) {
      protocol_next_screen();
      g_face_anim_ms = millis();
    }
  }
  g_touch_x0 = -1;
}

void ui_draw() {
  AppState& st = app_state();
  const uint32_t now = millis();
  if (!st.dirty && (now - g_last_draw_ms) < 33) {
    return;
  }
  st.dirty = false;
  g_last_draw_ms = now;

  const float target = static_cast<float>(st.screen) * 72.f;
  g_cube_yaw += (target - g_cube_yaw) * 0.22f;

  const uint8_t face = static_cast<uint8_t>(st.screen);
  if (face != g_last_face) {
    g_last_face = face;
    if (M5.Speaker.isEnabled()) {
      M5.Speaker.tone(660, 28);
    }
  }

  const int w = g_canvas.width();
  const int h = g_canvas.height();
  fill_bg(g_canvas, w, h);

  if (st.alert.have && !st.alert.acked) {
    g_canvas.fillSmoothRoundRect(6, kStripH + 2, w - 12, 28, 8, kCoral);
    g_canvas.setTextColor(kWhite, kCoral);
    g_canvas.setCursor(14, kStripH + 10);
    g_canvas.printf("%s  %s", st.alert.sev, st.alert.agent);
  }

  draw_face_strip(g_canvas, st.screen, w);

  const int bat = M5.Power.getBatteryLevel();
  switch (st.screen) {
    case Screen::Pulse:
      draw_pulse(g_canvas, st, w);
      break;
    case Screen::Heat:
      draw_heat(g_canvas, st, w);
      break;
    case Screen::Trail:
      draw_trail(g_canvas, st, w);
      break;
    case Screen::Beam:
      draw_beam(g_canvas, st, w);
      break;
    case Screen::Hub:
    default:
      draw_hub(g_canvas, st, w, bat);
      break;
  }

  draw_soft_keys(g_canvas, w, h);
  g_canvas.pushSprite(0, 0);
}
