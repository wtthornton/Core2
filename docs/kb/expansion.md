# Expansion, stacking, and related hardware

## Hard stacking rules (M5)

1. **Do not stack Core2 with M5 Base-series bases.** The vibration motor
   collides mechanically and can destroy parts.
2. **To stack M5 modules** on Core2, **remove the stock battery/IMU/mic
   rear board**. The module bus needs that volume.
3. To keep **battery + IMU + microphone** while adding modules, use
   **[Base M5GO Bottom2](https://docs.m5stack.com/en/base/m5go_bottom2)**
   instead of the stock rear board.

## M5GO Bottom2 (Core2-specific)

| Item | Spec |
|------|------|
| IMU | MPU6886 (on the base) |
| Mic | LMD4737 (docs since 2025-09; was SPM1423) |
| LEDs | **10× SK6812**, data **G25** |
| Battery | 500 mAh |
| Extra ports | Port B (G26/G36), Port C (G13/G14) |
| Charge | Pogo pins + **TP4057**; pogo also exposes I2C |
| Size | 54.0 × 54.0 × 15.0 mm, 30 g |
| Mount | Magnets + LEGO-compatible holes |

M5GO Bottom3 is the CoreS3 counterpart (WS2812, different mic/IR). Do not
buy Bottom3 for Core2.

## Core2 for AWS bottom

Same idea as Bottom2 plus **ATECC608** crypto. See [variants.md](variants.md).

## What you can attach

Anything on **M5-Bus** or **Grove Port A** that is electrically compatible
with ESP32 3.3 V logic and the 5 V rail (enable 5 V boost when required).

Common patterns:

- Env / motion / relay **Units** on Port A I2C
- GPS / LoRa / LTE **modules** on the bus (after removing the stock bottom)
- Magnetic charger dock on Bottom2 pogo pins
- microSD for maps, audio, Tactility apps, UiFlow assets (≤ 16 GB official)

## What you cannot assume

- Fire/GO RGB bars unless a Bottom2-class base is present
- Port B/C on the bare brick face
- JTAG for Zephyr debug
- Camera (CoreS3)
- Waterproofing (Tough)

## Mechanical keep-outs

- Vibration motor under the rear board
- 2.4 GHz antenna along the PCB; metal plates hurt Wi-Fi/BLE
- 54 mm envelope; stacked modules add height quickly (AWS kit ~24 mm)
