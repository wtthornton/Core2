# Plan: custom Core2 app (no M5 tools)

Status: **bring-up firmware built and flashed to COM4**.
This unit: original Core2, CP2104 **COM4**, AXP192, MPU6886, 16 MB flash.
On-device image: this repo’s `firmware/` dashboard (replaced WLED on 2026-09-11).

## Goal

A custom application owned in this repo that uses the Core2’s onboard
hardware. No M5Burner, no UiFlow, no M5 desktop IDE. Agents flash over
USB-UART with PlatformIO / esptool.

Python is the **host** language (probe, build, upload, serial, later
clients). On-device code is **C++** because that is the only stack that
turns on every peripheral without reinventing the HAL.

## Decision (locked)

| Layer | Choice | Why |
|-------|--------|-----|
| On-device HAL | **M5Unified + M5GFX** (GitHub libraries) | `M5.begin()` brings up display, touch, IMU, RTC, speaker, mic, correct PMU (AXP192 vs AXP2101). CircuitPython only inits PMU + LCD. |
| Build / flash | **PlatformIO CLI** + esptool | Not an M5 app. `board = m5stack-core2`, 16 MB partitions, PSRAM. |
| Host tools | **Python 3** in `scripts/` | Probe COM port, wrap `pio run -t upload`, serial monitor. |
| First app | **Bring-up dashboard** | Prove every onboard feature before product UI. |

Out of scope for this pass: CircuitPython, Zephyr, Tactility, UiFlow,
Linux, CoreS3 images, Wi-Fi/BLE product features, Grove accessories.

If the constraint later becomes “zero M5-authored *code*” (not just no M5
*tools*), switch the HAL to `espressif/m5stack_core_2` and keep the same
repo layout.

## Hardware the bring-up must show

On this brick (rear board attached):

1. Display ILI9342C 320×240
2. Touch FT6336U + three virtual buttons (BtnA / BtnB / BtnC)
3. AXP192: battery %, charging, power LED
4. Vibration motor (`M5.Power.setVibration`)
5. Speaker NS4168 (`M5.Speaker.tone`)
6. PDM mic present (`M5.Mic.isEnabled`)
7. IMU MPU6886 accel
8. RTC BM8563
9. microSD probe (`SD.begin` on CS G4; OK if no card)
10. Grove 5 V boost (`cfg.output_power = true`)
11. USB serial log at 115200

BtnA = beep, BtnB = vibe pulse, BtnC = LED toggle.

## Repo layout

```text
docs/firmware/PLAN.md     this document
firmware/platformio.ini   PlatformIO env
firmware/src/main.cpp     bring-up app
scripts/core2_usb.py      read-only USB probe
scripts/core2_dev.py      build / upload / monitor
requirements.txt          esptool, pyserial, platformio
```

Do not commit `.pio/`, `.tools/` dumps, or USB drivers.

## Flash path

1. `pip install -r requirements.txt`
2. `python scripts/core2_usb.py` — confirm COM4 / ESP32-D0WDQ6-V3
3. `python scripts/core2_dev.py upload --port COM4`
4. Device resets into the dashboard; `python scripts/core2_dev.py monitor`

PlatformIO uses DTR/RTS auto-reset. If connect fails: tap RST while
“Connecting…”, or `--baud 115200`.

## Implementation order

1. Write this plan (this file).
2. Add `firmware/` PlatformIO project and bring-up `main.cpp`.
3. Add `scripts/core2_dev.py` and ignore `.pio/`.
4. Point README / KB at the plan and firmware tree.
5. Install PlatformIO, compile, upload to COM4.
6. Quality-check changed Python.

## Later (not this pass)

- Product UI on top of the same HAL
- Optional serial/Wi-Fi RPC so host Python can drive `M5.*`
- OTA / 16 MB filesystem once the dashboard is proven
- CircuitPython only if a future app must be `code.py`

## Risks

| Risk | Mitigation |
|------|------------|
| WLED wiped | Expected; no backup unless we dump flash first |
| Screen dark | Always `M5.begin()` before drawing |
| SD init fights LCD SPI | Init SD after M5; treat missing card as OK |
| Mic + speaker share G0 | Don’t record and play at the same time in bring-up |
| Core2 v1.1/v1.3 | M5Unified auto-detects PMU/IMU; this unit is v1.0 |
