# M5Stack Core2 knowledge base

This folder is the Core2 hardware and software knowledge base for this repo.
It covers the M5Stack **Core2** family (ESP32-D0WDQ6-V3 IoT controller), not
M5Stack **CoreS3** (ESP32-S3, different product).

| Doc | What it answers |
|-----|-----------------|
| [overview.md](overview.md) | What Core2 is and what it can do |
| [hardware.md](hardware.md) | Full hardware bill of materials and mechanical specs |
| [display.md](display.md) | Screen, touch, backlight, SPI pinout |
| [wireless.md](wireless.md) | Wi-Fi, Bluetooth, antenna, mesh/ESP-NOW |
| [power.md](power.md) | PMU, battery, charging, power buttons |
| [pinout.md](pinout.md) | Grove, M-Bus, I2C map, reserved connectors |
| [software.md](software.md) | Firmware, RTOS, and languages that actually run on it |
| [usb-flash.md](usb-flash.md) | USB-UART, esptool, loading a new image |
| [variants.md](variants.md) | Core2 vs v1.1 vs v1.3 vs Core2 for AWS |
| [expansion.md](expansion.md) | Stacking rules, M5GO Bottom2, related SKUs |
| [sources.md](sources.md) | Primary sources used to build this KB |

Start at [overview.md](overview.md). AgentForge desk monitor (brick → AF direct):
[docs/firmware/VISION.md](../firmware/VISION.md),
[IMPLEMENTATION_PLAN.md](../firmware/IMPLEMENTATION_PLAN.md),
[PLAN.md](../firmware/PLAN.md), [PROTOCOL.md](../firmware/PROTOCOL.md).
The generated project index is [docs/INDEX.md](../INDEX.md). Machine-readable
summary: [llms.txt](../../llms.txt).
