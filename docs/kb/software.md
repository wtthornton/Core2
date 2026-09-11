# Software, firmware, and “operating systems” on Core2

Core2 runs **firmware on ESP32**, not a desktop OS. The ESP32-D0WDQ6-V3 has
no Linux MMU. Anything advertised as an “OS” is an RTOS, a launcher on
FreeRTOS, or an interpreter image.

## Official M5Stack platforms

| Platform | What you flash | Language | Notes |
|----------|----------------|----------|-------|
| **UiFlow1** | UiFlow firmware via M5Burner | Blockly + MicroPython | Legacy web/desktop IDE |
| **UiFlow2** | UiFlow2 firmware via M5Burner | Blockly + MicroPython | Current; USB or Wi-Fi “access code” |
| **Arduino IDE** | Sketch via USB | C++ | Board package + **M5Unified** / M5GFX |
| **PlatformIO** | `board = m5stack-core2` | C++ Arduino or IDF | 16 MB partitions, `BOARD_HAS_PSRAM` |
| **ESP-IDF** | IDF app + `espressif/m5stack_core_2` BSP | C | Pick AXP192 vs AXP2101 in menuconfig; LVGL demos |

Factory test firmware is distributed as **Easyloader** / M5Burner “UserDemo”.

Recommended Arduino HAL: **M5Unified** (replaces the old per-device
`M5Core2` library). Pair with **M5GFX**.

Example PlatformIO env (from M5 docs):

```ini
[env:m5stack-core2]
platform = espressif32@6.12.0
board = m5stack-core2
framework = arduino
upload_speed = 921600
monitor_speed = 115200
board_build.partitions = default_16MB.csv
build_flags =
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=5
lib_deps =
    M5Unified=https://github.com/m5stack/M5Unified
```

ESP-IDF BSP install:

```bash
idf.py add-dependency "espressif/m5stack_core_2^2.0.0"
idf.py set-target esp32
```

M5 documents ESP-IDF **v5.4.1** for their BSP tutorial. For Core2 v1.1 set
PMU to **AXP2101**; otherwise **AXP192**.

## Interpreters

| Image | Status |
|-------|--------|
| **UiFlow MicroPython** (`uiflow-micropython`) | Official; also usable without Blockly |
| **CircuitPython** | First-class board `m5stack_core2`, stable **10.3.0** as of this research |
| Community **lv_micropython** ports | Exist (e.g. lemariva/micropython-core2); you build them yourself |

CircuitPython 10.3.0 on Core2 includes `wifi`, `_bleio`, `displayio`,
`audiobusio`, `sdcardio`, `dualbank`, `espidf`, `espnow`, and frozen
`adafruit_requests` / connection manager.

## Real-time operating systems

| OS | Board support | Notes |
|----|---------------|-------|
| **FreeRTOS** | Always (ESP-IDF and Arduino-ESP32) | Default under M5 firmware |
| **Zephyr** | Upstream board **`m5stack_core2`**, status Maintained | LCD, touch, AXP192, IMU, SD, speaker; mic and battery-status marked incomplete; **debug not supported** |
| **MCUboot** | Optional Zephyr sysbuild | OTA-capable boot path |
| **NuttX** | ESP32 family / LVGL demos exist | Not a first-class M5 product image; POSIX-like RTOS, not Linux |
| **Tactility** | `sdkconfig.board.m5stack-core2` | Community “app OS” on FreeRTOS + LVGL; SD-card ELF apps; <https://tactility.one> |

Zephyr board name: `m5stack_core2`. Architecture `xtensa`, SoC `esp32`.
Requires `west blobs fetch hal_espressif` for RF binaries.

## Other languages

| Toolchain | Notes |
|-----------|-------|
| **TinyGo** | Espressif workshop drives the ILI9342C and AXP192 on Core2 |
| Rust (`esp-rs`) | Possible on ESP32; no M5-maintained Core2 crate equivalent to M5Unified |
| MicroPython vanilla | Use a Core2 board def; prefer UiFlow build if you want M5 HALs |

## What does **not** run

- **Linux / Debian / Android / Windows** — wrong CPU class (no MMU Linux port).
- **ESP32-S3-only** firmware (CoreS3, USB-OTG, native USB-JTAG).
- **Arduino CoreS3** board defs.
- Stock **M5Stack Basic/Fire** pin maps without a Core2 HAL.

## USB drivers

Install **CP210x** and **CH9102** VCP packages if you do not know which UART
bridge is populated. v1.3 is CH9102F; mixed original production used both.

## Libraries that know this board

- [m5stack/M5Unified](https://github.com/m5stack/M5Unified) — Arduino + ESP-IDF
- [m5stack/M5GFX](https://github.com/m5stack/M5GFX)
- [espressif/m5stack_core_2](https://components.espressif.com/components/espressif/m5stack_core_2) BSP
- Legacy [m5stack/M5Core2](https://github.com/m5stack/M5Core2) — migrate off this

## Graphics / UI

LVGL is the common rich-UI choice (IDF BSP, Zephyr, Tactility, some
MicroPython forks). M5GFX is the official immediate-mode/API layer.

## Flashing mental model

1. Put the device on USB-C.
2. Match drivers to CP2104 vs CH9102.
3. Hold/reset only if the ROM download mode does not auto-reset via DTR/RTS.
4. Use a **16 MB** partition table and enable PSRAM for UI work.
5. If the screen is dark, the sketch never initialized the **PMU** rails.

Step-by-step USB probing, Windows COM setup, and per-OS flash commands:
[usb-flash.md](usb-flash.md). Custom app (no M5Burner): [../firmware/PLAN.md](../firmware/PLAN.md).
