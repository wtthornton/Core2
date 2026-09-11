# M5Stack Core2 — what it is and what it can do

M5Stack Core2 is the second-generation **main controller** in the M5Stack
stackable IoT kit series. It is a palm-sized ESP32 board with a 2.0-inch
capacitive color touchscreen, speaker, microphone, IMU, RTC, vibration motor,
microSD slot, lithium battery, and M5-Bus / Grove expansion.

SKU for the original unit is commonly listed as **K010**. Later hardware
revisions are Core2 **v1.1** and Core2 **v1.3**. See [variants.md](variants.md).

Official product page: <https://docs.m5stack.com/en/core/core2>

## What it is good at

- **IoT HMI terminals** — local UI on the touchscreen plus 2.4 GHz Wi-Fi to a
  LAN or cloud.
- **STEM / Blockly / MicroPython** — official UiFlow1 and UiFlow2 firmware.
- **Arduino / ESP-IDF / PlatformIO** firmware with the **M5Unified** + **M5GFX**
  libraries (current recommended HAL).
- **Motion / pose sensing** — 6-axis IMU on the rear expansion board
  (MPU6886 on original and v1.1, BMI270 on v1.3).
- **Audio I/O** — PDM microphone in, I2S speaker out through NS4168.
- **Haptic feedback** — onboard vibration motor.
- **Offline storage** — microSD (TF) slot, officially up to 16 GB.
- **Timekeeping** — BM8563 RTC. v1.1 / v1.3 add an RTC backup cell so the clock
  survives power-off better than early Core2 units.
- **Secure AWS prototyping** — Core2 for AWS kits add an ATECC608 Trust&GO
  crypto element on the M5GO Bottom2-style base.
- **RTOS research / hobby OS** — first-class Zephyr board `m5stack_core2`,
  ESP-IDF FreeRTOS, CircuitPython, TinyGo, and community Tactility OS.

## What it is not

- It is **not a Linux computer**. The ESP32-D0WDQ6-V3 has no MMU suitable for
  Linux. “OS” here means firmware, FreeRTOS, Zephyr, NuttX-class RTOSes, or
  interpreter firmware (UiFlow MicroPython, CircuitPython).
- It has **no 5 GHz Wi-Fi**, **no Bluetooth 5**, **no onboard Ethernet PHY**,
  **no camera**, and **no cellular modem**.
- It is **not CoreS3**. CoreS3 is ESP32-S3 with USB-OTG, camera, dual mics, and
  a different power/audio stack. Do not mix CoreS3 docs into Core2 firmware.

## Typical applications (vendor)

- IoT controller / dashboard
- STEM education
- DIY and maker projects
- Smart-home panels
- Pose detection / simple IMU UI (especially v1.3 BMI270)
- AWS IoT classroom kits (Core2 for AWS)

## Physical identity

| Item | Value |
|------|-------|
| Form factor | M5Stack 54 × 54 mm “core” brick |
| Size | 54.0 × 54.0 × 16.5 mm (main unit) |
| Case | PC plastic |
| Front | 2.0" IPS + three capacitive “dot” hot zones |
| Left | Power button |
| Bottom | RST button, USB Type-C, Grove Port A |
| Rear | Removable expansion board (IMU + mic + battery) |
| Bottom bus | 30-pin M5-Bus for modules/bases |

## First-power sequence

- **Power on:** short-press the left power button.
- **Power off:** hold the left power button (about **6 s** on original Core2,
  about **4 s** on v1.1; v1.3 docs say long-press).
- **Reset:** short-press the bottom RST button.

## Development at a glance

| Path | Role |
|------|------|
| **This repo** `firmware/` + PlatformIO | Custom M5Unified bring-up; see [PLAN.md](../firmware/PLAN.md) |
| M5Burner + UiFlow2 | Official Blockly / MicroPython, Wi-Fi or USB |
| Arduino IDE + M5Unified | C++ sketches, largest example set |
| PlatformIO `m5stack-core2` | Same Arduino core, 16 MB partition, PSRAM flag |
| ESP-IDF + `espressif/m5stack_core_2` BSP | LVGL demos, pick AXP192 vs AXP2101 |
| CircuitPython 10.x | `m5stack_core2` board build |
| Zephyr `m5stack_core2` | Maintained upstream board |
| Tactility OS | App-launcher firmware on FreeRTOS (community) |

Details: [software.md](software.md).
