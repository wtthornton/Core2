# USB communication and flashing a Core2 “OS”

Core2 talks to a PC over **USB-C → USB-UART bridge → virtual COM port**.
The ESP32 ROM bootloader is what `esptool` uses to erase and write flash.
There is no Linux, no mass-storage UF2 drive, and no USB-OTG (that is CoreS3).

## What “OS” means here

Anything you load is firmware in the 16 MB SPI flash: UiFlow, Arduino/M5Unified,
CircuitPython, Zephyr, Tactility, WLED, etc. See [software.md](software.md).

## Host path (this Windows PC, 2026-09-11)

| Item | Value |
|------|-------|
| USB identity | Silicon Labs **CP2104** `VID_10C4` `PID_EA60` serial `022D5B31` |
| Hardware revision hint | **Original Core2 (v1.0)** — later v1.1 / v1.3 boards ship **CH9102F** |
| Windows port after driver | **COM4** (`Silicon Labs CP210x USB to UART Bridge`) |
| SoC | **ESP32-D0WDQ6-V3** rev v3.0, 40 MHz crystal |
| MAC | `08:3a:f2:44:97:94` |
| Flash | 16 MB (JEDEC `20 6018`), 3.3 V |
| Tool | `python -m esptool` **v5.4.0** |

Windows Code 28 (`CM_PROB_FAILED_INSTALL`) means the CP210x VCP driver is
missing. Install Silicon Labs **CP210x Universal Windows Driver**
(`silabser.inf`, WHQL 11.6) with an elevated `pnputil /add-driver … /install`.
If the UART chip is CH9102 instead, install M5’s CH9102 VCP package.

Power: USB enumerates the CP2104 from VBUS even if the ESP32 is off. Short-press
the **left power button** if `esptool` cannot connect. Auto-reset via DTR/RTS
worked on this unit; the bottom **RST** button is the fallback.

## Probe (read-only)

```powershell
python -m serial.tools.list_ports -v
python -m esptool --chip esp32 --port COM4 chip-id
python -m esptool --chip esp32 --port COM4 flash-id
python -m esptool --chip esp32 --port COM4 read-flash 0x8000 3072 partitions.bin
```

Repo helper: `python scripts/core2_usb.py` (defaults to COM4).

Custom firmware (PlatformIO + M5Unified, no M5Burner): [../firmware/PLAN.md](../firmware/PLAN.md) and `python scripts/core2_dev.py upload --port COM4`.

### Firmware currently on this unit

Bring-up dashboard from this repo (`firmware/`, PlatformIO + M5Unified), flashed
2026-09-11 over COM4. Partition table is Arduino **`default_16MB.csv`**
(app0/app1 ~6.25 MB each). That upload replaced the previous **WLED** image.

Re-flashing overwrites the dashboard. Confirm the target firmware first.

## Load a new image

Close anything holding COM4 (Arduino Serial Monitor, M5Burner, another
`esptool`). Prefer a USB **data** cable, not charge-only.

If connect fails, retry at `--baud 115200` and tap **RST** when esptool prints
`Connecting...`.

### Official UiFlow2 (M5Burner)

1. Install [M5Burner](https://docs.m5stack.com/en/download) (Win10 x64 v3.0).
2. Download the **Core2** UiFlow2 package (not CoreS3).
3. Plug USB-C, wait for `Found New Device`, **Burn** → COM port → **Start**.
4. Optional: Wi-Fi SSID/password and boot option.
5. After burn, program from <https://uiflow2.m5stack.com> over USB WebSerial
   or the on-screen access code.

M5Burner wraps esptool (`--baud 1500000`, `--flash_size detect`, chip
`ESP32-D0WDQ6-V3`).

### CircuitPython 10.x

Board page: <https://circuitpython.org/board/m5stack_core2/>

Original ESP32 has **no CIRCUITPY USB disk**. Flash the combined `.bin` at
`0x0` (Adafruit’s ESP32 recipe):

```powershell
python -m esptool --chip esp32 --port COM4 erase-flash
python -m esptool --chip esp32 --port COM4 write-flash -z 0x0 adafruit-circuitpython-m5stack_core2-en_US-10.3.0.bin
```

Talk over the same COM port (Thonny / `mpremote`), not a USB MSC drive.

### Arduino / PlatformIO / ESP-IDF

Board `m5stack-core2`, 16 MB partitions, `BOARD_HAS_PSRAM`, **M5Unified**.
IDF BSP: `espressif/m5stack_core_2`; pick AXP192 (this CP2104 unit) vs AXP2101
(v1.1). Upload uses the same COM port and DTR/RTS reset.

### Zephyr

Board `m5stack_core2`. `west blobs fetch hal_espressif` then `west flash`.
Debug is **not** supported (no JTAG pins).

### Tactility (community app OS)

Web installer: <https://install.tactility.one> (redirects to
install.tactilityproject.org). Use the **m5stack-core2** image, not S3 boards.
CLI equivalent is a merged binary at `0x0` via esptool.

## Do not flash

- CoreS3 / ESP32-S3 images
- Linux / Android / Debian
- Generic ESP32 images that never bring up the **AXP192** rails (black screen)

## MCP / skills on this host

Cursor MCP in `.cursor/mcp.json`: Linear (OAuth) plus local NLT fleet
`127.0.0.1:8760-8765`. There is **no hardware MCP** for Core2; USB access is
`esptool` / serial from the agent shell. Linear Agent skills list was empty.
Project skills are Linear-only unless `tapps-mcp upgrade --host cursor` is
applied.
