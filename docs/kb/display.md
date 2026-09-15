# Core2 display and touch

## Panel

| Spec | Value |
|------|-------|
| Size | **2.0 inch** diagonal |
| Type | IPS TFT LCD |
| Resolution | **320 × 240** pixels (QVGA, landscape-native controller) |
| Pixel format | **16-bit RGB565** (65,536 colors) |
| Controller | Ilitek **ILI9342C** (SPI; some BSPs reuse ILI9341 drivers) |
| Interface | 4-wire SPI, typically up to ~40 MHz in examples |
| Touch | Capacitive, FocalTech **FT6336U** |
| Front “buttons” | Three printed dots are **touch hot zones**, not mechanical keys |
| Brightness / reset / panel power | From the PMU, not ESP32 GPIOs |

Espressif TinyGo workshop notes: SPI at 40 MHz, `DisplayInversion: true`,
rotation often `Rotation0Mirror` for this panel.

## SPI pinout (LCD)

Shared SPI bus with the microSD card (different chip-select).

| Signal | ESP32 GPIO | Notes |
|--------|------------|-------|
| MISO | **G38** | Shared with TF card |
| MOSI | **G23** | Shared with TF card |
| SCK | **G18** | Shared with TF card |
| LCD CS | **G5** | |
| LCD DC | **G15** | Data/command |
| LCD RST | PMU | AXP192 `AXP_IO4` or AXP2101 `AXP_ALDO2` |
| Backlight | PMU | AXP192 `AXP_DC3` or AXP2101 `AXP_BLDO1` |
| LCD power | PMU | AXP192 `AXP_LDO2` or AXP2101 `AXP_ALDO4` |

microSD CS is **G4** on the same SPI.

If the panel stays black, the usual cause is the PMU rails (LCD power +
backlight) not enabled — M5Unified / ESP-IDF BSP do this in `begin()`.

## Touch pinout (FT6336U)

| Signal | Connection |
|--------|------------|
| SDA | **G21** (internal I2C) |
| SCL | **G22** |
| INT | **G39** |
| RST | Same PMU rail as LCD RST (AXP192 IO4 / AXP2101 ALDO2) |
| I2C address | **0x38** |

The three front dots are software-defined regions on this controller. Factory
and UiFlow firmware map them as Button A / B / C.

### Touch quirks

M5Stack documents **non-linear touch near the edges** on some panels. The
fix they recommend is flashing updated screen firmware with **M5Tool**.

FT6336U is a capacitive controller in the FT6x36 family (ESP-IDF BSP binds
it via `esp_lcd_touch_ft5x06`). Treat it as a **small multi-touch overlay**,
not a phone-class 10-point panel.

## Graphics stacks that work

| Stack | Notes |
|-------|-------|
| **M5GFX** (with M5Unified) | Official drawing/API for Arduino and ESP-IDF |
| **LVGL** | ESP-IDF BSP `bsp_display_start()` + `esp_lvgl_port`; Zephyr LVGL shell |
| CircuitPython `displayio` | Official `m5stack_core2` build |
| UiFlow widgets | Blockly wraps the MicroPython display driver |
| TinyGo `ili9341` SPI driver | Workshop uses ILI9341 driver against ILI9342C |

A full-screen RGB565 buffer is 320×240×2 = **153,600 bytes**. That belongs in
**PSRAM**, not internal SRAM.

## Desk type scale (firmware UI)

Panel is **200 PPI**. Default M5GFX GLCD: size 1 = 6×8 px (~1 mm, captions
only), size 2 = 12×16 (body), size 3 = 18×24 (hero numbers).

Keep face strip **46 px** and soft keys **40 px** (finger-size). Content band
is **146 px**. Six tabs are ~**49 px** wide, so size-2 words `Pulse` / `Trail`
do not fit.

Use size 2 for Talk transcript/reply, Heat rows, Beam URL, Hub agent, and the
Quiet/Talk key. Selected tab size 2; unselected tabs size 1. Do not shrink
chrome, do not load TTF, do not add a second tab row.

## What Core2 is not (display)

- Not CoreS3’s glass cover / same mechanical stack (panel IC is the same
  ILI9342C family, mechanical and PMU differ).
- Not M5 Tough: Tough uses **CHSC6540** touch and advertises up to **853 nits**.
  Core2 does not publish a nit rating in the same way.
- Not e-ink (that is CoreInk) and not M5Paper.
