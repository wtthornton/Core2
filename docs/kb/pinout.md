# Core2 pinout, Grove, and M-Bus

GPIO numbers are ESP32 GPIO names as used in M5 docs (`G21` = GPIO21).

## Front-of-brick connectors

| Connector | Pins | Function |
|-----------|------|----------|
| USB Type-C | USB 2.0 device | 5 V charge, UART download (CP2104 or CH9102F) |
| Grove **Port A** (red, HY2.0-4P) | GND, 5 V, **G32**, **G33** | I2C (SDA/SCL) |
| Power button | left side | PMU power |
| RST | bottom | ESP32 EN |

## Grove ports (logical)

M5 color convention (same as Core):

| Port | Color | GPIOs | Typical function |
|------|-------|-------|------------------|
| **A** | Red | G32 / G33 | I2C |
| **B** | Black | G26 / G36 | DAC / ADC |
| **C** | Blue | G13 / G14 | UART2 (RX/TX) |

HY2.0-4P wire colors in M5 docs: Black GND, Red 5 V, Yellow / White signal.

On a **bare Core2**, only Port A is on the housing. Ports B and C are on the
**M-Bus** and on bottoms such as M5GO Bottom2 (Port B and Port C exposed).

## UART download

| ESP32 | USB-UART chip |
|-------|----------------|
| G1 | RXD of CP2104/CH9102F (ESP32 TX) |
| G3 | TXD of CP2104/CH9102F (ESP32 RX) |

If flash fails with timeout or “Failed to write to target RAM”, reinstall
**both** CP210x and CH9102 VCP drivers, then retry cable/port. CH9102 macOS
v1.7 installer may show a false error after a successful install.

## Internal I2C (do not steal blindly)

| GPIO | Role |
|------|------|
| **G21** | SDA — PMU, RTC, touch, IMU, (v1.1 INA3221), (AWS ATECC608) |
| **G22** | SCL |

This bus is busy. Grove Port A (G32/G33) is the external I2C you should use
for units.

## LCD / SD SPI

| GPIO | LCD | microSD |
|------|-----|---------|
| G38 | MISO | MISO |
| G23 | MOSI | MOSI |
| G18 | SCK | SCK |
| G5 | CS | — |
| G15 | DC | — |
| G4 | — | CS |

## Audio I2S

| GPIO | NS4168 | SPM1423 mic |
|------|--------|-------------|
| G12 | BCLK | |
| G0 | LRCK | CLK |
| G2 | DATA (out) | |
| G34 | | DATA (in) |

G0 is a boot-strapping pin on ESP32; the I2S LRCK assignment is why some
bare-metal examples are picky about reset/download mode.

## Touch interrupt

**G39** = FT6336U INT.

## M5-Bus (30 pin)

M5 numbering (LEFT column pins 1,3,5… and RIGHT 2,4,6…):

| Pin | Signal | Pin | Signal |
|-----|--------|-----|--------|
| 1 GND | | 2 | G35 ADC |
| 3 GND | | 4 | G36 ADC (Port B) |
| 5 GND | | 6 | RST / EN |
| 7 G23 MOSI | | 8 | G25 DAC (also SK6812 data on M5GO Bottom2) |
| 9 G38 MISO | | 10 | G26 DAC (Port B) |
| 11 G18 SCK | | 12 | 3V3 |
| 13 G3 RXD0 | | 14 | G1 TXD0 |
| 15 G13 RXD2 | | 16 | G14 TXD2 |
| 17 G21 Int SDA | | 18 | G22 Int SCL |
| 19 G32 Port A SDA | | 20 | G33 Port A SCL |
| 21 G27 GPIO | | 22 | G19 GPIO |
| 23 G2 I2S_DOUT | | 24 | G0 I2S_LRCK |
| 25 NC | | 26 | G34 I2S_DATA |
| 27 NC | | 28 | 5V |
| 29 NC | | 30 | BAT |

This is **not** identical to original M5Stack Core M-Bus. Do not assume Fire
or Basic pin maps.

## Pins that are easy to break software with

- **G0** — boot mode + I2S LRCK + mic clock
- **G2** — I2S data to the amp
- **G12** — I2S BCLK (also a strapping-adjacent ESP32 concern)
- **G21/G22** — internal I2C
- **G38/G23/G18** — shared SPI

## Debugging

Zephyr: **on-board debugging is not supported** due to pinout limits (no
easy JTAG header). Use UART logs. OpenOCD is listed as a runner but the
board page says debugging is not supported in practice.

Arduino/IDF: USB-UART at 115200 (PlatformIO example) or 921600 upload.
