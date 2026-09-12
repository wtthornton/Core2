# Core2 power, battery, and PMU

Core2 always uses a dedicated PMIC. **Which chip** is the main hardware
revision flag used by M5Unified and the ESP-IDF BSP.

## PMU by revision

| Unit | PMU | Extra | Power LED |
|------|-----|-------|-----------|
| Core2 (v1.0 / “Core2”) | **AXP192** @ 0x34 | — | **Green** (AXP_IO1) |
| Core2 **v1.1** | **AXP2101** @ 0x34 | **INA3221** @ 0x40 | **Blue** |
| Core2 **v1.3** | **AXP192** @ 0x34 | — | **Green** |

AXP192 vs AXP2101 **chip IDs differ**; firmware reads the ID to pick rails.
ESP-IDF BSP: `CONFIG_BSP_PMU_AXP192` vs `CONFIG_BSP_PMU_AXP2101`.

### AXP192 vs AXP2101 (M5 comparison table)

| Feature | AXP2101 (v1.1) | AXP192 (Core2 / v1.3) |
|---------|----------------|------------------------|
| Battery voltage range | 0.7–4.2 V | 0.7–4.2 V |
| Charge current (table) | 100 mA | 500 mA |
| Charge efficiency | 94% | 90% |
| Charge termination | 10 mA | 50 mA |
| Discharge efficiency | 96% | 95% |
| Power output current | 300 mA | 500 mA |
| Power output efficiency | 95% | 90% |

v1.1 is the better **metering** board (INA3221). Original/v1.3 AXP192 can
source more charge current per that table.

## Battery

| Spec | Value |
|------|-------|
| Chemistry | Li-ion pack, nominal **3.7 V** |
| Early Core2 | **390 mAh** (K010 / CircuitPython page still mention this) |
| Current docs | **500 mAh** (change logged **2023-10** on original Core2 page) |
| Connector | 1.25 mm-2P on the PCB; pack is in the rear board |

Core2 for AWS / M5GO Bottom2 also use a **500 mAh** pack in the base.

There is **no fuel-gauge IC** on original Core2. Charge current / “full”
currents are measured at the USB input in M5’s table, not a remaining-mAh
register. Zephyr still marks “query current battery status” as incomplete
on the stock board support.

### USB charge currents (original / v1.3 AXP192 docs)

| Condition | Current |
|-----------|---------|
| Charging | 0.219 A |
| Full, unit powered off | 0.055 A |
| Full, unit powered on | 0.147 A |

Input spec: **5 V @ 500 mA** via USB Type-C.

## Boost and 5 V bus

**SY7088** boosts for the 5 V Grove/M-Bus rail. On Zephyr, **bus_5v** is a
regulator that is **off by default** — Grove devices that need 5 V must
enable it.

## Power button behavior

| Action | Original Core2 | v1.1 | v1.3 docs |
|--------|----------------|------|-----------|
| On | Click left power | Click | Click |
| Off | Hold **6 seconds** | Hold **4 seconds** | Long-press (duration not restated) |
| Reset | Bottom RST | Bottom RST | Bottom RST |

## Rails the PMU actually owns (AXP192 original)

| Rail | Typical use |
|------|-------------|
| AXP_IO4 | LCD + touch reset |
| AXP_DC3 | LCD backlight |
| AXP_LDO2 | LCD power |
| AXP_IO2 | Speaker enable |
| AXP_IO1 | Green LED |
| AXP_LDO3 | Vibration motor |
| AXP_PWR | RTC INT path |

v1.1 maps the same functions onto AXP2101 ALDO/BLDO/DLDO/VRTC names
(see Core2 v1.1 docs). Copy-pasting AXP192 register pokes onto v1.1 will
fail; use M5Unified.

## RTC backup

- Early Core2: RTC runs from main power; **inaccurate after full power-off**.
- **2023-02**: original Core2 dropped an RTC coin cell; timing while powered
  still works.
- **v1.1**: dedicated RTC backup battery.
- **v1.3**: **MS412FE 3 V 1.0 mAh** rechargeable micro cell listed.

## Charging accessories

M5GO Bottom2 / AWS base add a **pogo-pin** magnetic charge pad and **TP4057**
charge IC in the base. That is extra hardware, not on the bare Core2 brick.

## Safety / stacking

- Do **not** stack Core2 with M5 **Base** series bases: the vibration motor
  **mechanically interferes** and can damage the stack.
- To stack M5 **modules**, remove the Core2 battery bottom. To keep battery +
  IMU + mic, use **M5GO Bottom2** instead of the stock rear board.

## IEC

Original Core2 docs list **IEC 62133** for the pack.
