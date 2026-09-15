# Core2 hardware specifications

Values below come from M5Stack docs, the K010 datasheet, Core2 v1.1 / v1.3
product pages, and the Zephyr `m5stack_core2` board description. Where
revisions differ, the table says so. See [variants.md](variants.md).

## Compute

| Spec | Value |
|------|-------|
| SoC | Espressif **ESP32-D0WDQ6-V3** |
| CPU | Dual-core Xtensa® 32-bit **LX6**, independently controllable |
| Clock | Up to **240 MHz** (also 160 MHz) |
| Integer performance | **600 DMIPS** |
| On-chip SRAM | **520 KB** |
| Flash | **16 MB** SPI flash |
| PSRAM | **8 MB Quad** PSRAM |
| Crypto (on-chip) | Hardware RNG, AES, SHA, RSA, ECC (ESP32 silicon) |
| Deep-sleep (silicon) | On the order of 5 µA for the ESP32 die (board PMU still draws) |

The SoC also contains peripherals that are **not all brought out** as
user-friendly Core2 features: 12-bit ADC, 8-bit DAC, TWAI (CAN 2.0), Ethernet
MAC (no PHY on this board), RMT, LEDC PWM, MCPWM, PCNT, SDMMC host, 3× UART,
2× I2C, 2× I2S, 4× SPI. SPI and I2S already use DMA via M5GFX / `M5.Speaker` /
`M5.Mic` — do not rewrite a DMA engine.

**Do not ship these ESP32 maker tricks on Core2** (VISION + locked pinout):

| Feature | Why not |
|---------|---------|
| GPIO-matrix remap of Core2 buses | I2S is G0/G12/G2 (G0 is also a boot strap). Internal I2C is G21/G22 (AXP, touch, RTC). |
| ESP32 capacitive touch pads | UI is FT6336 on the glass plus A/B/C keys, not GPIO touch. |
| Wi-Fi promiscuous / sniffer | Product path is STA client to AgentForge, not a radio analyzer. |
| Internal Hall “lid” sensor | Noisy, chip-variant; not a cover detector. |
| AM radio / DAC “transmitter” | Regulated extra radio. Speaker is NS4168 I2S, not DAC GPIO25. |
| ULP coprocessor + deep sleep | Drops Wi-Fi and AF SSE. Desk HMI stays associated. Idle dim + 80 MHz on pack is the power policy (`pwr.cpp`). |

Die temperature (`temperatureRead()`, Beam `die NN C`) is the ESP32 **junction**, not room air. Firmware Serial-logs when it crosses **70 C** and clears the warn below **65 C**.

## Memory map (practical)

- **16 MB flash** — Arduino/PlatformIO use `default_16MB.csv` partitions.
- **8 MB PSRAM** — enable with `-DBOARD_HAS_PSRAM`. Needed for LVGL frame
  buffers and UiFlow.
- Internal SRAM is shared with Wi-Fi/BT stacks; keep large buffers in PSRAM.

## Display and touch

See [display.md](display.md). Summary: 2.0" IPS **320×240** ILI9342C over SPI,
capacitive **FT6336U**, AXP-controlled backlight/reset.

## Audio

| Part | Role |
|------|------|
| **NS4168** | I2S class-D amplifier |
| Speaker | **1 W**, size **0928** |
| **SPM1423** | PDM digital microphone (original / v1.1 / v1.3 rear board) |
| I2S pins | BCLK G12, LRCK G0, DATA G2; mic DATA G34, CLK on G0 |

AXP GPIO enables the speaker (`SPK_EN`). M5GO Bottom2 historically used
SPM1423; 2025-09 docs list **LMD4737** for that base’s mic.

Firmware Talk face records hold-to-talk into PSRAM (16 kHz mono PCM/WAV, ~6 s).
I2S is shared: `Speaker.end()` then `Mic.begin()` (no duplex). If PSRAM alloc
fails, optional spill to `/talk.wav` on microSD (CS **G4**). Missing rear board
or `Mic.begin()` failure shows **mic missing**, not a hang. Playback of AF TTS
is record-then-play on the same I2S port.

Device **status** speech (e.g. “blocked, no AF voice API”) is a canned WAV via
`M5.Speaker.playWav`, not on-device TTS and not a cloud TTS key. Jarvis replies
wait on AF TAP-7554. Quiet / `protocol_muted()` stops cues the same way it
stops haptics. Do not use PicoTTS / Talkie-on-DAC / VoiceText from the brick.

## Motion and haptics

| Part | Role | Where |
|------|------|-------|
| **MPU6886** | 6-axis IMU (gyro + accel), I2C **0x68** | Original Core2, v1.1 |
| **BMI270** | 6-axis IMU, I2C **0x68** | Core2 v1.3 (and AWS v1.3 bottom) |
| Vibration motor | 1027-class DC pager motor | Driven from AXP LDO (AXP192 LDO3 / AXP2101 DLDO1) |

IMU and microphone live on the **rear expansion board**, not the display PCB.
Removing that board (required to stack many M5 modules) removes IMU, mic, and
the pack battery unless you use M5GO Bottom2.

Firmware does **not** poll the IMU (`internal_imu = false`). A flexed rear
pogo on 0x68 can stall the shared G21/G22 I2C bus (AXP + touch) when the brick
is moved. Talk PTT is the Talk key / BtnB only — a palm on the glass while
picking it up used to start `Mic.begin()` on I2S G0 and hang the panel.

## Timekeeping

| Part | Role |
|------|------|
| **BM8563** | I2C RTC, address **0x51**, INT from PMU |
| RTC backup | None on early Core2 (clock drifts after power-off). **v1.1** adds a backup cell. **v1.3** lists **MS412FE 3 V 1.0 mAh** rechargeable micro cell. Feb 2023 original Core2 shipments removed a previous RTC button cell without killing RTC while powered. |

## Power and USB

See [power.md](power.md).

| Part | Role |
|------|------|
| **AXP192** | PMU on original Core2 and v1.3 |
| **AXP2101 + INA3221** | PMU + current monitor on v1.1 (INA3221 at **0x40**) |
| **SY7088** | DC-DC boost |
| USB-UART | **CP2104** and/or **CH9102F** (functionally equivalent). v1.3 is CH9102F. |
| USB connector | **Type-C**, 5 V @ 500 mA input spec |
| Battery | 3.7 V Li-ion, **390 mAh** on early units, **500 mAh** from Oct 2023 / current docs |
| Power LED | Green (AXP192 units) or **blue** (v1.1 AXP2101) |

## Storage

| Spec | Value |
|------|-------|
| microSD (TF) | SPI shared with LCD (MISO/MOSI/SCK), CS = **G4** |
| Official max | **16 GB** |
| Filesystem | Typical FAT on SD; not an eMMC |

LCD CS is **G5**; do not confuse with SD CS.

## Wireless

See [wireless.md](wireless.md). Onboard **2.4 GHz 3D antenna**. Wi-Fi 802.11
b/g/n on 2.4 GHz. Bluetooth 4.2 BR/EDR + BLE in the ESP32 silicon (M5
marketing pages often only list Wi-Fi).

## Mechanical

| Spec | Original Core2 | Core2 v1.1 | Core2 v1.3 |
|------|----------------|------------|------------|
| Product size | 54.0 × 54.0 × 16.5 mm | same | same |
| Net weight | 54.9 g | 45.1 g | 58.8 g |
| Package | 80.0 × 59.9 × 21.6 mm | same | same |
| Gross weight | 100.8 g | 74.3 g | 88.2 g |
| Base screws | Hex socket countersunk **M3** | same | same |
| Operating temp | 0–60 °C | 0–60 °C | 0–60 °C |
| Case | PC plastic | PC plastic | PC plastic |

Core2 for AWS v1.3 with bottom is **54.0 × 54.0 × 23.7 mm**, 72.1 g, 0–40 °C.

## In the box (typical)

- Core2 (or v1.1 / v1.3) unit
- USB Type-C cable (~20 cm)
- Hex key (2.0 mm L-key for M2.5 on later kits; M3 base screws on the brick)

## Certifications (original Core2 docs)

- CE / MIC / FCC / RCM
- IEC 62133 (battery)

## Internal I2C map (shared G21 SDA / G22 SCL)

| Device | Address | Notes |
|--------|---------|-------|
| AXP192 or AXP2101 | 0x34 | PMU ID distinguishes v1.0/v1.3 vs v1.1 |
| FT6336U | 0x38 | Touch; INT on G39 |
| INA3221 | 0x40 | v1.1 current sense only |
| BM8563 | 0x51 | RTC |
| MPU6886 or BMI270 | 0x68 | IMU on rear board |
| ATECC608B | 0x35 | Core2 for AWS bottom only |

## Onboard user I/O (not the M-Bus)

- 1× USB Type-C (charge + UART download)
- 1× Grove **Port A** (HY2.0-4P, I2C on G32/G33) on the main unit
- Power button, RST button
- 3× capacitive virtual buttons (screen hot zones)
- Green or blue PMU LED (not a user RGB LED unless a bottom is attached)

Port B (DAC/ADC) and Port C (UART) appear when a compatible bottom/module
exposes them, or on M-Bus. Original Core2 docs still document Port B/C pin
maps because they exist on the M-Bus even if only Port A is on the brick face.

## Reserved PCB footprints (under the rear board)

- Battery: **1.25 mm-2P**
- USB/UART cable: **1.25 mm-4P** (CP2104/CH9102 footprint)

## Sensors the SoC has that Core2 does not usefully expose

Hall sensor and capacitive-touch pads exist on ESP32 silicon but Core2’s
touch UI is the FT6336U panel, not ESP32 touch GPIOs. There is no Ethernet
jack. TWAI/CAN is not a first-class Grove port.
