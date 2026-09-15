#include "pwr.h"

#include <M5Unified.h>
#include <cstdio>
#include <esp32-hal-cpu.h>

#include "protocol.h"

namespace {

constexpr uint8_t kBrightDesk = 150;
constexpr uint8_t kBrightIdle = 24;
constexpr uint32_t kIdleMs = 45000;
constexpr uint32_t kMhzDesk = 240;
constexpr uint32_t kMhzIdle = 80;
constexpr int kDieWarnC = 70;
constexpr int kDieClearC = 65;
constexpr int16_t kVbusUsbMv = 4000;
constexpr int16_t kPackMinMv = 3000;

uint32_t g_last_ms = 0;
bool g_idle = false;
bool g_die_hot = false;

bool usb_in() {
  const int16_t vbus = M5.Power.getVBUSVoltage();
  if (vbus > kVbusUsbMv) {
    return true;
  }
  return M5.Power.isCharging() == m5::Power_Class::is_charging;
}

bool pack_present() {
  const int16_t mv = M5.Power.getBatteryVoltage();
  return mv >= kPackMinMv;
}

bool voice_hot() {
  switch (app_state().voice.phase) {
    case VoicePhase::Listening:
    case VoicePhase::Thinking:
    case VoicePhase::Playing:
      return true;
    default:
      return false;
  }
}

void apply_idle(bool idle) {
  if (idle == g_idle) {
    return;
  }
  g_idle = idle;
  if (idle) {
    M5.Display.setBrightness(kBrightIdle);
    setCpuFrequencyMhz(kMhzIdle);
    Serial.println(F("pwr idle 80MHz dim"));
  } else {
    setCpuFrequencyMhz(kMhzDesk);
    M5.Display.setBrightness(kBrightDesk);
    Serial.println(F("pwr desk 240MHz"));
  }
}

}  // namespace

void pwr_begin() {
  g_last_ms = millis();
  g_idle = false;
  M5.Display.setBrightness(kBrightDesk);
  setCpuFrequencyMhz(kMhzDesk);
}

void pwr_note_activity() { g_last_ms = millis(); }

void pwr_poll() {
  if (voice_hot()) {
    pwr_note_activity();
  }
  if (app_state().alert.fire_haptic) {
    pwr_note_activity();
  }
  const bool want_idle =
      !usb_in() && !voice_hot() && (millis() - g_last_ms) >= kIdleMs;
  apply_idle(want_idle);

  const int die = pwr_die_c();
  if (die >= kDieWarnC && !g_die_hot) {
    g_die_hot = true;
    Serial.printf("die %dC (warn >= %d)\n", die, kDieWarnC);
  } else if (die < kDieClearC) {
    g_die_hot = false;
  }
}

void pwr_format_hub(char* out, size_t n) {
  if (out == nullptr || n == 0) {
    return;
  }
  const auto chg = M5.Power.isCharging();
  const int32_t lvl = M5.Power.getBatteryLevel();
  const bool pack = pack_present();
  const bool have_pct = pack && lvl >= 0 && lvl <= 100;
  const bool usb = usb_in();

  if (chg == m5::Power_Class::is_charging) {
    if (have_pct) {
      snprintf(out, n, "chg %ld%%", static_cast<long>(lvl));
    } else {
      snprintf(out, n, "charging");
    }
    return;
  }
  if (usb) {
    if (have_pct && lvl >= 1) {
      snprintf(out, n, "USB %ld%%", static_cast<long>(lvl));
    } else {
      snprintf(out, n, "USB");
    }
    return;
  }
  if (have_pct) {
    snprintf(out, n, "%ld%%", static_cast<long>(lvl));
    return;
  }
  snprintf(out, n, "--");
}

int pwr_die_c() { return static_cast<int>(temperatureRead()); }
