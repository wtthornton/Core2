// Core2 bring-up dashboard. Plan: docs/firmware/PLAN.md
#include <M5Unified.h>
#include <SD.h>

namespace {

constexpr uint32_t kVibeMs = 180;
constexpr uint8_t kVibeLevel = 180;

const char* boardName(m5::board_t board) {
  switch (board) {
    case m5::board_t::board_M5StackCore2:
      return "Core2";
    default:
      return "other";
  }
}

const char* pmicName(m5::Power_Class::pmic_t pmic) {
  switch (pmic) {
    case m5::Power_Class::pmic_axp192:
      return "AXP192";
    case m5::Power_Class::pmic_axp2101:
      return "AXP2101";
    default:
      return "unknown";
  }
}

bool g_sd_ok = false;
bool g_led_on = true;
uint32_t g_vibe_until = 0;

void drawDashboard() {
  auto& d = M5.Display;
  const int w = d.width();
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setCursor(6, 6);
  d.printf("Core2 bring-up  %s  %s", boardName(M5.getBoard()),
           pmicName(M5.Power.getType()));

  d.setCursor(6, 24);
  const int bat = M5.Power.getBatteryLevel();
  const int mv = M5.Power.getBatteryVoltage();
  const auto chg = M5.Power.isCharging();
  const char* chg_s = (chg == m5::Power_Class::is_charging)    ? "chg"
                      : (chg == m5::Power_Class::is_discharging) ? "dsg"
                                                                 : "?";
  d.printf("Bat %d%%  %dmV  %s  LED %s", bat, mv, chg_s, g_led_on ? "on" : "off");

  d.setCursor(6, 40);
  if (M5.Rtc.isEnabled()) {
    m5::rtc_datetime_t dt{};
    if (M5.Rtc.getDateTime(&dt)) {
      d.printf("RTC %04d-%02d-%02d %02d:%02d:%02d", dt.date.year, dt.date.month,
               dt.date.date, dt.time.hours, dt.time.minutes, dt.time.seconds);
    } else {
      d.print("RTC read failed");
    }
  } else {
    d.print("RTC disabled");
  }

  d.setCursor(6, 56);
  if (M5.Imu.isEnabled()) {
    float ax = 0, ay = 0, az = 0;
    M5.Imu.getAccel(&ax, &ay, &az);
    d.printf("IMU a %.2f %.2f %.2f", ax, ay, az);
  } else {
    d.print("IMU disabled (rear board?)");
  }

  d.setCursor(6, 72);
  d.printf("Spk %s  Mic %s  SD %s", M5.Speaker.isEnabled() ? "yes" : "no",
           M5.Mic.isEnabled() ? "yes" : "no", g_sd_ok ? "ok" : "none");

  d.setCursor(6, 88);
  if (M5.Touch.getCount() > 0) {
    const auto t = M5.Touch.getDetail(0);
    d.printf("Touch %d,%d", t.x, t.y);
  } else {
    d.print("Touch --");
  }

  d.setCursor(6, 112);
  d.print("A beep   B vibe   C LED");

  const int y = d.height() - 28;
  d.fillRect(0, y, w / 3, 28, M5.BtnA.isPressed() ? TFT_DARKGREEN : TFT_NAVY);
  d.fillRect(w / 3, y, w / 3, 28, M5.BtnB.isPressed() ? TFT_DARKGREEN : TFT_NAVY);
  d.fillRect(2 * w / 3, y, w / 3, 28, M5.BtnC.isPressed() ? TFT_DARKGREEN : TFT_NAVY);
  d.setTextDatum(middle_center);
  d.drawString("A", w / 6, y + 14);
  d.drawString("B", w / 2, y + 14);
  d.drawString("C", 5 * w / 6, y + 14);
  d.setTextDatum(top_left);
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

  M5.Display.setBrightness(128);
  M5.Speaker.setVolume(96);
  M5.Power.setLed(255);

  g_sd_ok = SD.begin(GPIO_NUM_4, SPI, 25000000);

  Serial.begin(115200);
  Serial.printf("Core2 bring-up board=%s pmic=%s sd=%d\n",
                boardName(M5.getBoard()), pmicName(M5.Power.getType()),
                static_cast<int>(g_sd_ok));
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed() && M5.Speaker.isEnabled()) {
    M5.Speaker.tone(2000, 120);
  }
  if (M5.BtnB.wasPressed()) {
    M5.Power.setVibration(kVibeLevel);
    g_vibe_until = millis() + kVibeMs;
  }
  if (g_vibe_until != 0 && static_cast<int32_t>(millis() - g_vibe_until) >= 0) {
    M5.Power.setVibration(0);
    g_vibe_until = 0;
  }
  if (M5.BtnC.wasPressed()) {
    g_led_on = !g_led_on;
    M5.Power.setLed(g_led_on ? 255 : 0);
  }

  drawDashboard();
  delay(50);
}
