# Core2 — Documentation Index

Curated map of this repo’s M5Stack Core2 knowledge base.
Generated 2026-09-11, then categorized by topic (the raw DocsMCP scanner
mis-bins files whose body text contains short tokens like `ci`).

**13 KB documents** plus project entry files.

## Knowledge base (start here)

| Doc | Topic |
|-----|--------|
| [docs/kb/README.md](kb/README.md) | Hub |
| [docs/kb/overview.md](kb/overview.md) | What Core2 is and what it can do |
| [docs/kb/hardware.md](kb/hardware.md) | SoC, memory, BOM, mechanical |
| [docs/kb/display.md](kb/display.md) | 2.0" 320×240 ILI9342C + FT6336U |
| [docs/kb/wireless.md](kb/wireless.md) | 2.4 GHz Wi-Fi, Bluetooth 4.2, ESP-NOW |
| [docs/kb/power.md](kb/power.md) | AXP192 / AXP2101, battery, buttons |
| [docs/kb/pinout.md](kb/pinout.md) | Grove, M-Bus, I2C, SPI, I2S |
| [docs/kb/software.md](kb/software.md) | UiFlow, Arduino, IDF, Zephyr, CircuitPython |
| [docs/kb/usb-flash.md](kb/usb-flash.md) | USB-UART, esptool, loading a new image |
| [docs/firmware/VISION.md](firmware/VISION.md) | **Locked vision: brick → AF direct (no host bridge)** |
| [docs/firmware/IMPLEMENTATION_PLAN.md](firmware/IMPLEMENTATION_PLAN.md) | Full reset plan: docs, Linear, code, verify |
| [docs/firmware/PLAN.md](firmware/PLAN.md) | AgentForge ops HMI plan + middleware cleanup |
| [docs/firmware/PROTOCOL.md](firmware/PROTOCOL.md) | Device → AF HTTP/SSE (direct) |
| [docs/kb/variants.md](kb/variants.md) | v1.0 / v1.1 / v1.3 / AWS vs CoreS3 |
| [docs/kb/expansion.md](kb/expansion.md) | Stacking rules, M5GO Bottom2 |
| [docs/kb/sources.md](kb/sources.md) | Vendor URLs |

## Project

| Doc | Topic |
|-----|--------|
| [README.md](../README.md) | Repo entry |
| [AGENTS.md](../AGENTS.md) | Agent / Linear scope |
| [llms.txt](../llms.txt) | Short machine-readable summary |
| [llms-full.txt](../llms-full.txt) | Longer DocsMCP dump |
| [docs/kb/memory-seed.json](kb/memory-seed.json) | tapps-mcp memory import payload |

## Recall keys (tapps-brain)

When the brain HTTP service is up (`127.0.0.1:8080`):

```text
tapps-mcp memory import-file --file docs/kb/memory-seed.json --overwrite
```

Pinned in `.tapps-mcp.yaml` `memory_hooks.auto_recall.recall_keys`:

- `core2-hardware-identity`
- `core2-display`
- `core2-wireless`
- `core2-software-os`
- `core2-variants`
- `core2-pinout`
- `core2-power`
- `core2-stacking-limits`
- `core2-af-direct-vision`
