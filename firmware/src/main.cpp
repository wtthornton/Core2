// AgentForge ops HMI — brick talks to AF directly (docs/firmware/VISION.md)
#include <M5Unified.h>
#include <esp_system.h>

#include "af_sse.h"
#include "net.h"
#include "protocol.h"
#include "pwr.h"
#include "ui.h"
#include "voice.h"
#include "voice_client.h"

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
  if (app_state().voice.phase != VoicePhase::Listening && M5.Speaker.isEnabled()) {
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
  Serial.setRxBufferSize(512);
  Serial.begin(115200);

  auto cfg = M5.config();
  // IMU sits on the rear board (shared I2C with AXP/touch). We never read it;
  // probing 0x68 on a flexed pogo can stall the bus when the brick is moved.
  cfg.internal_imu = false;
  cfg.internal_rtc = true;
  cfg.internal_spk = true;
  cfg.internal_mic = true;
  cfg.output_power = true;
  cfg.clear_display = true;
  cfg.serial_baudrate = 0;  // Serial already started; do not re-begin
  M5.begin(cfg);

  M5.Speaker.setVolume(72);
  M5.Power.setLed(255);

  Serial.printf("core2 fw %s reset %d\n", CORE2_FW_VERSION, static_cast<int>(esp_reset_reason()));
  protocol_begin();
  ui_begin();
  pwr_begin();
  ui_draw();  // paint Hub before mic/Wi-Fi so a later hang is not a black panel
  voice_begin();
  voice_client_begin();
  net_begin();
  af_sse_begin();
}

void loop() {
  M5.update();
  protocol_poll_serial();

  // Touch first so UI stays snappy even if a poll step runs next.
  if (M5.Touch.isEnabled()) {
    const auto t = M5.Touch.getDetail(0);
    if (t.wasClicked() || t.wasPressed() || t.wasReleased()) {
      pwr_note_activity();
    }
    if (t.wasClicked()) {
      ui_handle_touch(t.x, t.y);
      ui_touch_release(t.x, t.y);  // clear any swipe latch
    } else {
      if (t.wasPressed()) {
        ui_handle_touch(t.x, t.y);
      }
      if (t.wasReleased()) {
        ui_touch_release(t.x, t.y);
      }
    }
  }

  net_poll();
  af_sse_poll();
  voice_poll();
  voice_client_poll();

  if (M5.BtnA.wasPressed()) {
    pwr_note_activity();
    if (ui_setup_open()) {
      ui_setup_cancel();
    } else {
      protocol_prev_screen();
    }
  }
  if (M5.BtnB.wasPressed()) {
    pwr_note_activity();
    if (ui_setup_open()) {
      ui_setup_save();
    } else if (app_state().screen == Screen::Talk) {
      voice_ptt_start();
    } else if (app_state().alert.have && !app_state().alert.acked) {
      protocol_ack();
    } else {
      protocol_mute_15m();
      voice_silence();
    }
  }
  if (M5.BtnB.wasReleased() && !ui_setup_open() && app_state().screen == Screen::Talk) {
    pwr_note_activity();
    if (voice_ptt_stop()) {
      voice_client_submit();
    }
  }
  if (M5.BtnC.wasPressed()) {
    pwr_note_activity();
    if (ui_setup_open()) {
      ui_setup_toggle_kb();
    } else {
      protocol_next_screen();
    }
  }

  pwr_poll();
  maybe_haptic();
  vibe_timeout();

  const SnapState& snap = app_state().snap;
  M5.Power.setLed(snap.ready && snap.reachable ? 255 : 0);

  ui_draw();
  delay(10);
}
