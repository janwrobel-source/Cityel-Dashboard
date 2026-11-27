# Option 3 — Off-the-Shelf (OTS) Modules & Parts Shopping Guide

This guide accompanies [`cityel_dashboard_modules_bom.csv`](./cityel_dashboard_modules_bom.csv) and [`cityel_dashboard_modules_gerbers.zip`](./cityel_dashboard_modules_gerbers.zip) for the **Bare PCB + Breakout Modules (100% Through-Hole)** build.

> **Tip on links:** Direct seller listings on AliExpress/Amazon expire frequently, so each item below includes **evergreen search links** (AliExpress, Amazon, LCSC/TME) plus **official brand pages** (Adafruit, Waveshare) where available, along with the exact PCB socket pinout and setup notes.

---

## 1. Off-the-Shelf Breakout Modules (`M1` – `M8`)

| Ref | Module | Qty | PCB Socket Pinout | Est. Price | Where to Buy |
| :--- | :--- | :---: | :--- | :---: | :--- |
| **`M1`** | **MP1584EN 5V 3A Buck Converter** (fixed 5V or adjustable mini module) | 1 | `1x04` (2.54 mm):<br>`1=12V_PROT`, `2=GND`, `3=+5V_BUCK`, `4=GND` | \$1–2 | • [AliExpress: MP1584EN 5V buck](https://www.aliexpress.com/w/wholesale-MP1584EN-5V-buck-module.html)<br>• [Amazon: MP1584EN 5V buck](https://www.amazon.com/s?k=MP1584EN+5V+buck+converter+module)<br>• [Pololu 5V 2.5A D24V22F5 (premium drop-in)](https://www.pololu.com/product/2858) |
| **`M2`** | **DS3231 RTC Module** (`ZS-042` 6-pin goldpin module) | 1 | `1x06` (2.54 mm):<br>`1=32K (NC)`, `2=SQW (NC)`, `3=I2C_SCL`, `4=I2C_SDA`, `5=+3V3`, `6=GND` | \$2–3 | • [AliExpress: DS3231 RTC module](https://www.aliexpress.com/w/wholesale-DS3231-RTC-module.html)<br>• [Amazon: DS3231 RTC module](https://www.amazon.com/s?k=DS3231+RTC+module)<br>• [Adafruit DS3231 Precision RTC](https://www.adafruit.com/product/3013) |
| **`M3` + `M4`** | **PCF8575 16-Bit I2C I/O Expander Breakout** (standard blue/green `2×12` / `0.6"` wide module) | 1 | `M3` (`1x12`): `1..8=P00..P07`, `9=+3V3`, `10=GND`, `11=SDA`, `12=SCL`<br>`M4` (`1x12`, spaced `15.24 mm`): `1..4=P10..P13`, `5..12=NC` | \$1.50–3 | • [AliExpress: PCF8575 16-bit I2C module](https://www.aliexpress.com/w/wholesale-PCF8575-I2C-module.html)<br>• [Amazon: PCF8575 I2C breakout](https://www.amazon.com/s?k=PCF8575+16+bit+I2C+module)<br>• [Adafruit PCF8575 Breakout](https://www.adafruit.com/product/5611) |
| **`M5`** | **MAX98357A I2S 3W Class-D Audio Amplifier** | 1 | `1x07` (2.54 mm):<br>`1=LRC`, `2=BCLK`, `3=DIN`, `4=GAIN` (`R34=100k` to GND), `5=SD (NC)`, `6=GND`, `7=+5V` | \$2–6 | • [AliExpress: MAX98357A I2S amp](https://www.aliexpress.com/w/wholesale-MAX98357A-I2S-module.html)<br>• [Amazon: MAX98357A breakout](https://www.amazon.com/s?k=MAX98357A+I2S+amplifier+breakout)<br>• [Adafruit MAX98357A Breakout (original)](https://www.adafruit.com/product/3006) |
| **`M6`** | **Isolated TTL-to-RS485 Module** (3.3V/5V, **auto-direction**, built-in isolated DC-DC) | 1 | `1x04` (2.54 mm):<br>`1=+3V3 (VCC)`, `2=GND`, `3=JKBMS_TX (RXD)`, `4=JKBMS_RX (TXD)`<br>*(Connect JK-BMS `A/B/GND` to the module's own isolated screw terminal)* | \$4–8 | • [Waveshare TTL to RS485 (C) — Isolated Mini](https://www.waveshare.com/ttl-to-rs485-c.htm)<br>• [AliExpress: Isolated TTL RS485 auto-direction](https://www.aliexpress.com/w/wholesale-isolated-TTL-to-RS485-module-auto.html)<br>• [Amazon: Waveshare TTL to RS485 C](https://www.amazon.com/s?k=Waveshare+TTL+to+RS485+C) |
| **`M7`** | **ADuM1201 Dual-Channel Digital Isolator** (`CJMCU-1201`, isolated UART2 for FarDriver) | 1 | `1x04` (2.54 mm, Side 1):<br>`1=+3V3 (V1)`, `2=FARD_UART_RX (AO)`, `3=FARD_UART_TX (BI)`, `4=GND (G1)`<br>*(Connect FarDriver TX/RX/5V/GND to Side 2 of the module)* | \$2–4 | • [AliExpress: ADuM1201 CJMCU-1201 module](https://www.aliexpress.com/w/wholesale-ADuM1201-CJMCU-1201-module.html)<br>• [Amazon: ADuM1201 isolator module](https://www.amazon.com/s?k=ADuM1201+digital+isolator+module) |
| **`M8`** | **I2C FRAM Breakout (`MB85RC256V` 32 KB or `FM24C64B` 8 KB)** (zero-wear odometer storage, `0x50`) | 1 | `1x08` (2.54 mm):<br>`1=+3V3 (VCC)`, `2=GND`, `3=WP (NC)`, `4=I2C_SCL`, `5=I2C_SDA`, `6=A2 (NC)`, `7=A1 (NC)`, `8=A0 (NC)` | \$2–5 | • [AliExpress: MB85RC256V I2C FRAM module](https://www.aliexpress.com/w/wholesale-MB85RC256V-I2C-FRAM-module.html)<br>• [Amazon: MB85RC256V FRAM breakout](https://www.amazon.com/s?k=MB85RC256V+I2C+FRAM+breakout)<br>• [Adafruit I2C Non-Volatile FRAM 32KB (MB85RC256V)](https://www.adafruit.com/product/1895) |

> **Total for all 7 breakout modules:** ~**\$15 – \$31** (AliExpress) or ~**\$30 – \$55** (Amazon / Adafruit / Waveshare).

---

## 2. Important Module Setup Notes (Read Before Power-On!)

1. **`M1` (MP1584EN Buck):**
   - If you buy the **adjustable** version (with the tiny potentiometer), power it on the bench from 12V and **set the output to `5.00 V` with a multimeter BEFORE plugging it into `M1`**.
   - Better yet, buy the **fixed 5V** version so vibration in the CityEl cannot drift the pot.
   - Check pin order against `M1` (`1=12V_PROT, 2=GND, 3=+5V_BUCK, 4=GND`); if your module's pins are at the 4 corners, use short 2.54 mm jumper wires or bent header pins.
2. **`M2` (DS3231 RTC — `ZS-042` board):**
   - If using a standard **non-rechargeable CR2032** cell on a `ZS-042` board, **desolder or cut the tiny 200 Ω resistor (or diode) next to the `SCL`/`VCC` charging circuit** so the board does not try to charge the CR2032.
   - `M2` is a full **6-pin** socket (`32K, SQW, SCL, SDA, VCC, GND`) so a `ZS-042` module plugs straight in via its 6-pin goldpin header (`32K` and `SQW` are NC mechanically supported).
3. **`M3` + `M4` (PCF8575):**
   - Leave `A0`, `A1`, `A2` at default (`GND`) so the I2C address is **`0x20`** (matches `EXPANDER_I2C_ADDR` in `src/pins.h`).
   - Check that your PCF8575 board's `VCC/GND/SDA/SCL` pins match `M3` pins `9–12` (some clone boards put the 4-pin I2C header on the short edge — if so, wire those 4 pins to `M3` pins `9–12` with short jumpers).
4. **`M5` (MAX98357A):**
   - Pin order matches the standard Adafruit/clone 7-pin layout (`LRC, BCLK, DIN, GAIN, SD, GND, VIN`).
   - On-board resistor `R34 = 100 kΩ` pulls `GAIN` to `GND` for **15 dB** gain (good cabin volume). Connect a `4 Ω` or `8 Ω` 3W speaker directly to the module's speaker terminal.
5. **`M6` (Isolated RS485 — JK-BMS):**
   - Must be an **automatic-direction** isolated module (no separate `DE/RE` pin needed). The **Waveshare TTL to RS485 (C)** is plug-and-play at 3.3V and includes TVS/ESD protection and isolated power.
6. **`M7` (ADuM1201 — FarDriver UART2):**
   - **Optional** if you only use the 1-wire SIF / stock Hall speedometer on `J5`.
   - Because `ADuM1201` has no built-in DC-DC converter, the FarDriver side (`VDD2`, `GND2`) must be powered from the FarDriver's own 5V/3.3V programming/Bluetooth port supply.
7. **`M8` (I2C FRAM — `MB85RC256V` / `FM24C64B`):**
   - `M8` is a full **8-pin** socket (`VCC, GND, WP, SCL, SDA, A2, A1, A0`) so standard 8-pin `MB85RC256V` breakouts (AliExpress, Amazon, Adafruit) plug straight in via their goldpin header.
   - `WP`, `A2`, `A1`, and `A0` are left NC on the PCB (internally pulled down on the module), giving the default I2C address **`0x50`** (matches `ODO_FRAM_I2C_ADDR` in `src/pins.h`).

---

## 3. Key Through-Hole (THT) Parts & Connectors

All remaining parts in [`cityel_dashboard_modules_bom.csv`](./cityel_dashboard_modules_bom.csv) are standard through-hole components easily ordered in one cart from **LCSC**, **TME**, **Mouser/DigiKey**, or **AliExpress**:

| Part | Qty | Spec / Notes | Quick Search Links |
| :--- | :---: | :--- | :--- |
| **Supercapacitors (`C8`, `C9`)** | 2 | `5F 2.7V` radial, `Ø10 mm`, `5.0 mm` lead pitch | [AliExpress: 5F 2.7V supercapacitor](https://www.aliexpress.com/w/wholesale-5F-2.7V-supercapacitor-10mm.html) · [LCSC](https://www.lcsc.com/search?q=5F%202.7V%20supercapacitor) |
| **Quad Optocouplers (`U1`–`U3`)** | 3 | `LTV-847` (or `PC847` / `EL847`), **DIP-16** (+ optional 3× DIP-16 IC sockets) | [AliExpress: LTV-847 DIP-16](https://www.aliexpress.com/w/wholesale-LTV-847-DIP16.html) · [LCSC](https://www.lcsc.com/search?q=LTV-847%20DIP-16) |
| **Single Optocoupler (`U4`)** | 1 | `PC817` (or `EL817`), **DIP-4** | [AliExpress: PC817 DIP-4](https://www.aliexpress.com/w/wholesale-PC817-DIP4.html) · [LCSC](https://www.lcsc.com/search?q=PC817%20DIP-4) |
| **Logic-Level MOSFET (`Q1`)** | 1 | `IRLZ44N`, **TO-220** (frontlight PWM) | [AliExpress: IRLZ44N TO-220](https://www.aliexpress.com/w/wholesale-IRLZ44N-TO220.html) · [LCSC](https://www.lcsc.com/search?q=IRLZ44N) |
| **Schottky Diodes (`D2`, `D3`, `D4`)** | 3 | `1N5822` (3A 40V, DO-201AD) | [AliExpress: 1N5822](https://www.aliexpress.com/w/wholesale-1N5822-diode.html) · [LCSC](https://www.lcsc.com/search?q=1N5822) |
| **TVS Diode (`D1`)** | 1 | `1.5KE15A` (15V 1500W, DO-201AD) | [AliExpress: 1.5KE15A](https://www.aliexpress.com/w/wholesale-1.5KE15A.html) · [LCSC](https://www.lcsc.com/search?q=1.5KE15A) |
| **Zener & Rectifier (`D5`, `D6`, `D7`)** | 1 / 2 | `1N4728A` (3.3V 1W Zener, DO-41) + 2× `1N4007` (DO-41) | [AliExpress: 1N4728A](https://www.aliexpress.com/w/wholesale-1N4728A.html) |
| **PTC Resettable Fuse (`F1`)** | 1 | `2A` radial/axial PTC polyfuse (16V–30V) | [AliExpress: 2A PTC resettable fuse](https://www.aliexpress.com/w/wholesale-2A-PTC-resettable-fuse.html) |
| **1W Resistors (`R4`, `R29`, `R30`)** | 3 | `10 Ω 1W` axial metal-film/metal-oxide | [AliExpress: 10 ohm 1W resistor](https://www.aliexpress.com/w/wholesale-10-ohm-1W-resistor.html) |
| **1/4W Resistors & `100 nF` Cap** | 20 | `100R` (1), `2.2k` (11), `3.3k` (1), `4.7k` (1), `10k` (5), `100k` (1) + 1× `100 nF` ceramic disc | Standard 1/4W resistor assortment kit |
| **5.08 mm Screw Terminals (`J1, J3, J4, J5, J8`)** | 5 | `5.08 mm` pitch PCB screw terminals: 2× 2-pin, 1× 4-pin, 1× 5-pin, 1× 8-pin (or interlockable 2/3-pin blocks) | [AliExpress: 5.08mm screw terminal block](https://www.aliexpress.com/w/wholesale-5.08mm-PCB-screw-terminal-block.html) |
| **2.54 mm Female Pin Headers (`M1`–`M8`, `J2`)** | Assorted | `1×40` breakaway female strips (cut to 3× 4-pin, 1× 6-pin, 1× 7-pin, 1× 8-pin, 2× 12-pin) + `2×10` (20-pin) header & ribbon/Dupont cable to CrowPanel | [AliExpress: 2.54mm female pin header](https://www.aliexpress.com/w/wholesale-2.54mm-female-pin-header-strip.html) · [2×10 IDC cable](https://www.aliexpress.com/w/wholesale-2x10-20-pin-2.54mm-ribbon-cable.html) |
