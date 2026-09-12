// AgentForge ops HMI — brick talks to AF directly (docs/firmware/VISION.md)
#include <M5Unified.h>

#include "net.h"
#include "protocol.h"
#include "ui.h"

namespace {

constexpr uint8_t kVibeLevel = 180;
constexpr uint32_t kVibeMs = 180;
uint32_t g_vibe_until = 0;

void maybe_haptic() {
  AlertState& a = app_state().alert;
  if (!a.fire_haptic || a.acked || protocol_muted()) {
    a.fire_haptic = false;
    return;
  }
  a.fire_haptic = false;
  M5.Power.setVibration(kVibeLevel);
  g_vibe_until = millis() + kVibeMs;
  if (M5.Speaker.isEnabled()) {
    M5.Speaker.tone(880, 160);
  }
}

void vibe_timeout() {
  if (g_vibe_until != 0 && static_cast<int32_t>(millis() - g_vibe_until) >= 0) {
    M5.Power.setVibration(0);
    g_vibe_until = 0;
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.internal_rtc = true;
  cfg.internal_spk = true;
  cfg.internal_mic = true;
  cfg.output_power = true;
  cfg.clear_display = true;
  M5.begin(cfg);

  M5.Display.setBrightness(150);
  M5.Speaker.setVolume(72);
  M5.Power.setLed(255);

  Serial.begin(115200);
  protocol_begin();
  ui_begin();
  net_begin();
}

void loop() {
  M5.update();
  protocol_poll_serial();
  net_poll();

  if (M5.Touch.isEnabled()) {
    const auto count = M5.Touch.getCount();
    if (count > 0) {
      const auto t = M5.Touch.getDetail(0);
      if (t.wasPressed()) {
        ui_handle_touch(t.x, t.y);
      }
      if (t.wasReleased()) {
        ui_touch_release(t.x, t.y);
      }
    }
  }

  if (M5.BtnA.wasPressed()) {
    protocol_prev_screen();
  }
  if (M5.BtnB.wasPressed()) {
    if (app_state().alert.have && !app_state().alert.acked) {
      protocol_ack();
    } else {
      protocol_mute_15m();
    }
  }
  if (M5.BtnC.wasPressed()) {
    protocol_next_screen();
  }

  maybe_haptic();
  vibe_timeout();

  const SnapState& snap = app_state().snap;
  M5.Power.setLed(snap.ready && snap.reachable ? 255 : 0);

  ui_draw();
  delay(16);
}
