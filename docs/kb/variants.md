# Core2 family variants and lookalikes

## Same product family (ESP32-D0WDQ6-V3, 2.0" ILI9342C, 16 MB + 8 MB)

| Name | PMU | IMU | USB-UART | LED | Battery notes | Distinctive |
|------|-----|-----|----------|-----|---------------|-------------|
| **Core2** (v1.0, SKU K010) | AXP192 | MPU6886 | CP2104 **or** CH9102F | Green | 390 mAh then 500 mAh (2023-10) | First release 2020-06 |
| **Core2 v1.1** | **AXP2101 + INA3221** | MPU6886 | CH9102F | **Blue** | 500 mAh + RTC backup | Charge/metering change |
| **Core2 v1.3** | AXP192 | **BMI270** | CH9102F | Green | 500 mAh + MS412FE RTC cell | IMU upgrade, same PMU as v1.0 |
| **Core2 for AWS** | AXP192 | MPU6886 (classic) or BMI270 (v1.3 kit) | CP2104 or CH9102F | Green | 500 mAh in base | **ATECC608** + M5GO Bottom2 + 10× SK6812 + pogo charge |
| **Core2 for AWS v1.3** | AXP192 | BMI270 | CH9102F | Green | 500 mAh | ATECC608B-TNGTLS @ 0x35 |

### Original Core2 changelog (M5)

| Date | Change |
|------|--------|
| 2020-06 | First release |
| 2021-07 | CP2104 → CH9102F (mixed field population) |
| 2023-02 | RTC button cell removed (timing while powered unchanged) |
| 2023-10 | Pack **390 → 500 mAh** |

### v1.1 vs original (M5)

- PMU AXP192 → AXP2101 + INA3221
- LED green → blue
- RTC backup battery added
- Firmware must detect PMU ID

### v1.3 vs v1.1 vs original (M5)

| | v1.3 | v1.1 | Core2 |
|--|------|------|-------|
| IMU | BMI270 | MPU6886 | MPU6886 |
| PMIC | AXP192 | AXP2101 | AXP192 |
| USB-TTL | CH9102 | CH9102 | CP2104/CH9102 |
| LED | Green | Blue | Green |

## Related but **not** Core2

| Product | Why it is different |
|---------|---------------------|
| **M5Stack Core / Basic / Gray / Fire** | First-gen cores, different PMU/buttons, no FT6336 capacitive overlay |
| **CoreS3 / CoreS3-SE / CoreS3-Lite** | **ESP32-S3 LX7**, USB OTG/CDC, camera (except SE), AW88298 + ES7210 audio, AXP2101, magnetometer on full CoreS3 |
| **Tough** | Industrial sealed shell, CHSC6540 touch, 6–24 V/RS485, ~853 nit panel, no vibration motor in the Core2 sense |
| **M5Paper / CoreInk / StickC / Atom / Stamp** | Completely different I/O and mechanics |
| **M5GO Bottom2** | Accessory **base** for Core2, not a CPU |

## AWS kit extras

Core2 for AWS = Core2 + **M5GO Bottom2**-class base with:

- ATECC608 Trust&GO (v1.3: **ATECC608B-TNGTLSU-G**)
- 10× SK6812 side LEDs (data on **G25**)
- Pogo magnetic charge + TP4057
- Extra Grove Port B / C
- Magnets / LEGO holes
- Taller stack (~23.5–23.7 mm)

Use AWS IoT device certs in the ATECC; do not assume every Core2 has a
secure element.

## How firmware should branch

1. Read PMU ID → AXP192 vs AXP2101 (v1.1).
2. Probe IMU 0x68 who-am-I → MPU6886 vs BMI270 (v1.3).
3. Do not assume ATECC at 0x35 unless the AWS bottom is attached.

M5Unified performs (1) and (2) for you if you call `M5.begin()`.
