# Core2 wireless, radio, and networking

## What is on the board

| Radio | Reality on Core2 |
|-------|------------------|
| **Wi-Fi** | Yes. ESP32 2.4 GHz only, onboard **2.4 GHz 3D antenna**. |
| **Bluetooth Classic (BR/EDR)** | Yes in silicon (Bluetooth **4.2**). Dual-mode with BLE. |
| **Bluetooth LE** | Yes (BLE 4.2). CircuitPython ships `_bleio`. |
| **5 GHz Wi-Fi** | No. |
| **Bluetooth 5 / 5.x LE long range** | No (original ESP32, not ESP32-S3/C3/C6). |
| **802.15.4 / Thread / Zigbee** | No (wrong chip). |
| **LoRa / cellular / GNSS** | Not onboard. Add a module or Grove unit. |
| **Ethernet** | ESP32 has a MAC; Core2 has **no PHY or RJ45**. |

M5Stack’s Core2 HTML spec table often lists only “2.4 GHz Wi-Fi”. Digi-Key
K010 and GitHub `m5stack/M5Core2` still say **Wi-Fi and dual-mode Bluetooth**.
Zephyr’s board page is explicit: **802.11 b/g/n/e/i** and **Bluetooth v4.2
BR/EDR and BLE**.

## Wi-Fi details (ESP32-D0WDQ6-V3)

| Item | Value |
|------|-------|
| Band | 2.4 GHz ISM |
| PHYs | 802.11 **b/g/n** (ESP32 also implements e/i in the stack) |
| Antenna | PCB/3D 2.4 GHz antenna on the Core2 PCB |
| SoftAP + STA | Supported by ESP-IDF / Arduino |
| Practical use | UiFlow2 can take SSID/password at flash time |

Coexistence: Wi-Fi and Bluetooth share the 2.4 GHz radio. Heavy BLE + high
throughput Wi-Fi needs coexistence settings in ESP-IDF; keep expectations
modest.

## Bluetooth details

- **Classic SPP / A2DP / HFP** are possible at the ESP-IDF level but are
  large; most Core2 apps use **BLE GATT** instead.
- Arduino: `BluetoothSerial` (Classic) or BLE libraries; M5Unified does not
  replace the Espressif BT stack.
- CircuitPython: `_bleio` is in the 10.3.0 Core2 build.
- Use case called out in M5 literature: phone ↔ Core2 BLE (e.g. “smartwatch”
  style demos in *ESP32 formats and communication protocols*).

## Protocols that run over this radio

These are **software**, not extra chips:

| Protocol | Works? | Notes |
|----------|--------|-------|
| TCP/UDP/HTTP/MQTT/WebSocket | Yes | Arduino, IDF, UiFlow, CircuitPython |
| TLS | Yes | mbedTLS in IDF; watch RAM, use PSRAM |
| **ESP-NOW** | Yes | Connectionless ESP32 vendor protocol |
| **ESP-MESH** | Yes | IDF mesh |
| mDNS | Yes | CircuitPython includes `mdns` |
| Matter / Thread | Not native | No 802.15.4; Wi-Fi Matter is theoretically possible but not a Core2 product feature |

## Cloud kits

**Core2 for AWS** adds **ATECC608B-TNGTLS** (I2C 0x35) on the expansion base
so AWS IoT Core device identity can use a hardware key. The Wi-Fi radio is
still the same ESP32.

## Antenna and enclosure notes

- The 3D antenna is inside the 54 mm brick. Metal bases, thick 3D-printed
  shells, or holding the antenna edge will drop RSSI.
- There is no external antenna U.FL on standard Core2.

## What to tell firmware

- Set Wi-Fi country/channel to 2.4 GHz only.
- Do not copy CoreS3 USB-JTAG or Wi-Fi 6 notes.
- For Arduino/PlatformIO, PSRAM + 16 MB partitions matter more for TLS + UI
  than for the radio itself.
