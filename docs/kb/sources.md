# Sources

Compiled 2026-09-11 from public vendor and upstream docs. Prefer the
revision-specific M5 page that matches the unit in hand.

## M5Stack (primary)

- Core2 (v1.0): <https://docs.m5stack.com/en/core/core2>
- Core2 v1.1: <https://docs.m5stack.com/en/core/Core2%20v1.1>
- Core2 v1.3: <https://docs.m5stack.com/en/core/Core2_v1.3>
- Core2 for AWS: <https://docs.m5stack.com/en/core/core2_for_aws>
- Core2 for AWS v1.3: <https://docs.m5stack.com/en/core/Core2_For_AWS_v1.3>
- Shop v1.3: <https://shop.m5stack.com/products/m5stack-core2-esp32-iot-development-kit-v1-3>
- M5GO Bottom2: <https://docs.m5stack.com/en/base/m5go_bottom2>
- CoreS3 (contrast only): <https://docs.m5stack.com/en/core/CoreS3>
- Tough (contrast only): <https://docs.m5stack.com/en/core/tough>
- ESP-IDF BSP tutorial: <https://docs.m5stack.com/en/esp_idf/m5core2/bsp>
- UiFlow2 Core2: <https://docs.m5stack.com/en/uiflow2/m5core2/program>
- Learn / tool overview: <https://docs.m5stack.com/en/learn/intro>
- K010 PDF (Digi-Key): <https://media.digikey.com/pdf/Data%20Sheets/M5Stack%20PDFs/K010.pdf>
- Core2 v1.3 PDF: <https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/core/Core2_v1.3.pdf>

## Espressif / Zephyr / CircuitPython

- ESP component `espressif/m5stack_core_2`
- Zephyr board: <https://docs.zephyrproject.org/latest/boards/m5stack/m5stack_core2/doc/index.html>
- CircuitPython: <https://circuitpython.org/board/m5stack_core2/>
- TinyGo display notes: <https://developer.espressif.com/workshops/tinygo/assignment-3/>

## Libraries and community OS

- M5Unified: <https://github.com/m5stack/M5Unified>
- M5Core2 (legacy): <https://github.com/m5stack/M5Core2>
- Tactility: <https://github.com/ByteWelder/Tactility> / <https://tactility.one>
- Tactility web installer: <https://install.tactility.one>
- WLED (previous image on the research unit; replaced 2026-09-11): <https://github.com/Aircoookie/WLED>
- Silicon Labs CP210x Universal Windows Driver: <https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers>
- M5Burner / USB drivers: <https://docs.m5stack.com/en/download>
- Adafruit CircuitPython ESP32 esptool recipe: <https://learn.adafruit.com/circuitpython-with-esp32-quick-start>
- lv_micropython Core2 port (community): <https://github.com/lemariva/micropython-core2>

## Research method (this repo)

- Public web pages above (fetched 2026-09-11)
- TappsMCP CLI: `lookup-docs` (no Context7 corpus for this SKU), `doctor`,
  `fleet start`, `docsmcp scan`, DocsMCP `DocIndexGenerator` / `LlmsTxtGenerator`
- AgentForge brain search (`tapps_research` / `web_research` on `:8080`) was
  **offline** in this session (no tapps-brain HTTP, no Postgres DSN). Durable
  facts are staged in `docs/kb/memory-seed.json` for:

  `tapps-mcp memory import-file --file docs/kb/memory-seed.json --overwrite`
