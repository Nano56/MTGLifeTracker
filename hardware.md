# MTG Commander Life Tracker — Hardware Requirements & BOM

This document provides the complete Bill of Materials (BOM), exact manufacturer/generic **Model IDs**, direct **part search links**, **battery integration solutions**, and a breakdown of **soldering vs. solderless assembly** for building the physical 4-player MTG Commander Life Tracker.

> [!IMPORTANT]
> **Pinout & Electrical Compatibility:** This BOM matches the firmware pin allocations in [`Config.h`](Config.h), the circuit layout in [`diagram.json`](diagram.json), and the architecture decisions in [`docs/HARDWARE_SPEC.md`](docs/HARDWARE_SPEC.md).

---

## 1. Core Bill of Materials (BOM)

| Qty | Component | Primary Model ID / Part # | Fallback / Alternate IDs | Role in Tracker | Est. Cost (USD) |
| :-: | :--- | :--- | :--- | :--- | :---: |
| **1** | **ESP32 Development Board** (Pre-Soldered Pins) | `ESP32-DEVKITC-32E` / `ESP-WROOM-32` (30-pin or 38-pin DevKit V1) | `CP2102 ESP32-WROOM-32D`, `NodeMCU-32S` | Main 240 MHz dual-core MCU driving SPI graphics, interrupts, and NVS flash save | \$5.00 – \$8.00 |
| **1** | **2.8" or 3.2" ILI9341 IPS SPI TFT LCD** (240×320, 3.3V/5V) | `MSP2833` (2.8" IPS) / `MSP2807` (2.8" TFT) | `MSP3226` (3.2" IPS), `MSP3218`, `TJCTM24028-SPI`, Adafruit `1770` | 320×240 color tabletop display (178° IPS viewing angle recommended) split into 2–4 player quadrants | \$10.00 – \$16.00 |
| **4** | **KY-040 Rotary Encoder Module** (with Push Button & Knob Cap) | `KY-040` (uses `EC11` / Bourns `PEC11R-4215F-S0024` encoder) | `HW-040` | 1 per player corner: turn to adjust Life/Cmdr/Poison/Tax, click to cycle mode, hold 3s to reset player | \$6.00 – \$9.00 (4-pack) |
| **1** | **Red Momentary Pushbutton** (Table Reset / Dice Roll) | `PBS-33B` (12mm Waterproof Panel Mount) | Omron `B3F-4055` (12×12mm Tactile Switch w/ Red Cap), Adafruit `1445` | Global table button: click to roll starting player, 2×click for Event Log, hold 1.5s reset, hold 3s player menu | \$1.00 – \$3.00 |
| **1** | **Wiring & Interconnect Kit** | `40P-20CM-FF` (2.54mm DuPont Female-to-Female Ribbon) | ElectroCookie `Mini-PCB` or `WAGO 221-415` (5-port lever nuts) | Connects ESP32 GPIOs to display, encoders, and shared `3V3`/`GND` power rails | \$5.00 – \$8.00 |

---

## 2. Detailed Part Specifications & Search Links

If a specific vendor listing goes out of stock, search any electronics distributor (DigiKey, Mouser, Adafruit, Amazon, AliExpress) using the **Exact Model IDs** below.

### 2.1 Microcontroller (ESP32)
* **Primary Model IDs:** `ESP-WROOM-32 DevKit V1` (DOIT 30-pin or 38-pin) | Espressif Official: `ESP32-DEVKITC-32E`
* **USB-to-UART Bridge ID:** `CP2102` or `CH340C` (USB-C or Micro-USB)
* **Key Specs Required:**
  * Must expose standard VSPI pins (`GPIO 18`, `19`, `21`, `22`, `23`) and encoder pins (`GPIO 4`, `5`, `13`, `14`, `15`, `16`, `17`, `25`, `26`, `27`, `32`, `33`, `34`).
  * Buy the **pre-soldered header pins** version to avoid soldering 60–76 header pins yourself.
* **Direct Links:**
  * [DigiKey — Espressif ESP32-DEVKITC-32E](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-DEVKITC-32E/12091810)
  * [Adafruit — ESP32 DevKitC V4 (`ADA3269`)](https://www.adafruit.com/product/3269)
  * [Amazon Search — `ESP-WROOM-32 DevKit V1 CP2102 Pre-Soldered`](https://www.amazon.com/s?k=ESP-WROOM-32+DevKit+V1+CP2102)

### 2.2 Display (2.8" or 3.2" ILI9341 IPS SPI TFT — Recommended for Tabletop Viewing)
* **Primary Model IDs:**
  * **IPS Wide-Angle Panel (Recommended):** `MSP2833` (2.8" IPS SPI) | `MSP3226` (3.2" IPS SPI) | Waveshare `2.4inch LCD Module` (`18366`)
  * **Standard TFT Panel:** `MSP2807` (2.8" SPI Red PCB) | `MSP3218` (3.2" SPI Red PCB) | `TJCTM24028-SPI` | Adafruit `1770`
* **Driver IC:** `ILI9341` (4-wire SPI interface, 240×320 RGB565)
* **Why IPS over Standard TFT or OLED:**
  * Because the tracker sits flat in the center of a Commander table and is viewed from 4 different seating directions, an **IPS (In-Plane Switching) 240×320 SPI TFT** provides OLED-like deep contrast and **178° viewing angles from all 4 sides** without color inversion, while avoiding OLED burn-in and keeping 100% compatibility with our `320×240` color UI and wiring.
* **Key Specs Required:**
  * Must be the **4-wire SPI version** (typically a red PCB with a 9-pin or 14-pin single-row header: `VCC`, `GND`, `CS`, `RESET`, `DC/RS`, `SDI/MOSI`, `SCK`, `LED`, `SDO/MISO`), **NOT** the 8-bit/16-bit parallel UNO shield version.
  * Note on `RESET` and `LED` pins on physical red PCB modules: While Wokwi simulates `TFT_RST = -1` and auto-lights the backlight, on physical `MSP2833`/`MSP2807`/`TJCTM24028-SPI` modules you must tie the `LED` (backlight) pin and `RESET` pin to `3.3V` (so the screen lights up and stays out of hardware reset).
* **Direct Links:**
  * [LCDWiki Hardware Reference — `MSP2833` 2.8" IPS SPI Module](http://www.lcdwiki.com/2.8inch_IPS_SPI_Module_ILI9341_SKU:MSP2833)
  * [LCDWiki Hardware Reference — `MSP2807` 2.8" Standard SPI Module](http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807)
  * [Adafruit — 2.8" TFT LCD with Touchscreen Breakout (`ADA1770`)](https://www.adafruit.com/product/1770)
  * [Amazon Search — `ILI9341 2.8 IPS SPI TFT LCD 240x320`](https://www.amazon.com/s?k=ILI9341+2.8+IPS+SPI+TFT+LCD+240x320)

### 2.3 Player Rotary Encoders (4× KY-040)
* **Primary Model IDs:** `KY-040` / `HW-040` Breakout Module (uses mechanical encoder `EC11` / Bourns `PEC11R-4215F-S0024`)
* **Key Specs Required:**
  * 5-pin header (`CLK`, `DT`, `SW`, `+`, `GND`) with on-board $10\text{ k}\Omega$ pull-up resistors.
  * Threaded bushing with hex nut and washer (allows bolting directly through a 3D-printed or project-box lid) plus aluminum/plastic knob caps.
  * **CRITICAL ELECTRICAL RULE:** Connect the `+` (`VCC`) pin on all four `KY-040` modules to the ESP32 **`3V3` (3.3V) rail**, **NEVER to `5V`/`VIN`**. Because `GPIO 34` (`P4_DT`) relies on the `KY-040` on-board pull-up resistor, powering the module with 3.3V pulls the signal safely to 3.3V.
* **Direct Links:**
  * [DigiKey — Bourns `PEC11R-4215F-S0024` (Bare EC11 Encoder)](https://www.digikey.com/en/products/detail/bourns-inc/PEC11R-4215F-S0024/4499665)
  * [Amazon Search — `KY-040 Rotary Encoder Module with Knob Cap`](https://www.amazon.com/s?k=KY-040+Rotary+Encoder+Module)

### 2.4 Global Table Reset / Roll Button
* **Primary Model IDs:**
  * **For Enclosure Mounting (Recommended):** `PBS-33B` (12mm Momentary Push Button, Red, Normally Open) or Adafruit `1445` (16mm Panel Mount Momentary Red)
  * **For Breadboard / PCB Mounting:** Omron `B3F-4055` + `B32-1310` (12×12mm Tactile Switch with Red Keycap)
* **Direct Links:**
  * [Adafruit — 16mm Panel Mount Momentary Pushbutton Red (`ADA1445`)](https://www.adafruit.com/product/1445)
  * [DigiKey — Omron `B3F-4055` Tactile Switch](https://www.digikey.com/en/products/detail/omron-electronics-inc-emc-div/B3F-4055/44252)
  * [Amazon Search — `PBS-33B 12mm Red Momentary Push Button`](https://www.amazon.com/s?k=PBS-33B+12mm+Red+Momentary+Push+Button)

---

## 3. Battery Integration Solutions

### Power Consumption & Runtime Estimate
* **ESP32 (Active, Wi-Fi/BT radios off):** ~40–60 mA
* **2.8"–3.2" ILI9341 TFT + Backlight:** ~50–80 mA
* **4× KY-040 Encoders + Pull-ups:** < 2 mA
* **Total Draw:** **~100–145 mA @ 5V**
* A **2000 mAh LiPo or 18650 cell** will run the tracker continuously for **10 to 14 hours** (2–3 full Commander game nights per charge). Furthermore, because firmware auto-saves match state to NVS Flash (`StorageManager`), even if the battery dies mid-game, the match resumes automatically upon recharge!

Here are the **three best battery integration options**, ranked from easiest/no-solder to most compact custom integration:

### Option A: Zero-Solder / Easiest — 18650 Battery Shield V3 / V8 (Recommended for Fast Build)
An all-in-one board that holds a standard 18650 rechargeable lithium cell, includes a built-in USB-C/Micro-USB charging port, an On/Off power switch, and regulated `5V` and `3.3V` output headers/USB port.
* **Parts Needed:**
  1. **18650 Battery Shield:** Model ID `18650-SHIELD-V3` or `18650-SHIELD-V8` ([Amazon Search](https://www.amazon.com/s?k=18650+Battery+Shield+V3+ESP32))
  2. **18650 Cell (Unprotected Flat-Top or Protected Button-Top depending on holder, 2500–3400 mAh):** Model ID `INR18650-25R` (Samsung) or `NCR18650B` (Panasonic) ([Adafruit `ADA1781`](https://www.adafruit.com/product/1781))
* **How it connects:** Either plug a short 6-inch USB cable from the shield's USB-A output into the ESP32's USB port, **or** run two Female-to-Female DuPont jumper wires from the shield's `5V` and `GND` pins to the ESP32's `VIN` and `GND` pins. **Zero soldering required.**

### Option B: Pro Tabletop Enclosure — Adafruit PowerBoost 1000C + LiPo Pouch (Best Form Factor)
If you are putting the tracker inside a thin custom 3D-printed case, a flat LiPo pouch + load-sharing UPS boost converter lets you play while charging via USB without resetting the ESP32.
* **Parts Needed:**
  1. **Charger/Booster:** **Adafruit PowerBoost 1000 Charger (`ADA2465`)** (Uses `TPS61090` boost + `MCP73871` load-sharing charger) — [Adafruit Link](https://www.adafruit.com/product/2465) | [DigiKey `1528-1244-ND`](https://www.digikey.com/en/products/detail/adafruit-industries-llc/2465/5353663)
  2. **LiPo Pouch Battery (3.7V 2000mAh–2500mAh with JST-PH 2-pin):** Model ID `LP785060` or **Adafruit `ADA328` (2500mAh)** / **`ADA2011` (2000mAh)** — [Adafruit Link](https://www.adafruit.com/product/328)
  3. **Power Slide Switch:** Model ID `SS-12D00G3` or Breadboard SPDT Slide Switch (`ADA805`) — [Adafruit Link](https://www.adafruit.com/product/805)
* **How it connects:**
  * Plug the LiPo battery's JST-PH connector into the `PowerBoost 1000C`.
  * Solder the slide switch to the `EN` (Enable) and `GND` pins on the PowerBoost (turns the whole device on/off cleanly).
  * Connect the PowerBoost `5V` and `G` outputs to the ESP32 `VIN` (5V in) and `GND` pins.

### Option C: Budget DIY Module — TP4056 USB-C + MT3608 Boost Converter (or IP5306 Module)
* **Parts Needed:**
  1. **Integrated 5V 2A Boost/Charger Module:** Model ID `IP5306` / `MH-CD42` Power Bank Module ([Amazon Search](https://www.amazon.com/s?k=MH-CD42+5V+Boost+Charger+Module)) **OR** discrete `TP4056` USB-C Charger + `MT3608` DC-DC Step-Up Converter.
  2. **Battery:** Any 3.7V `103450` (2000mAh) LiPo cell.
* **Note:** Watch out for "auto-standby" on cheap power-bank ICs like `MH-CD42` if current drops below 50 mA; since the ESP32 + ILI9341 backlight draws ~120 mA continuously, it stays above the 50 mA threshold and will not auto-shutoff.

---

## 4. Is Soldering Required? (What Requires Solder & How to Avoid It)

**Short answer:** Yes, **light soldering is normally required for the final enclosure build**, primarily because the ESP32 only has a few `3V3` and `GND` pins that must be split across **6 devices** (1 display + 4 encoders + 1 button), plus the panel-mount reset button and battery power switch.

However, you can build this in **two ways** depending on whether you want to solder:

### Where Soldering Normally Happens
1. **Shared Power & Ground Distribution (`3V3` and `GND` Buses):**
   * All 13 signal pins (`CLK`, `DT`, `SW`, `CS`, `DC`, `MOSI`, `SCK`, `MISO`, `BTN_RESET`) connect **1-to-1** directly to individual ESP32 pins using standard Female-to-Female DuPont jumper wires—no soldering needed if you buy pre-soldered boards.
   * However, you need **6× `3.3V` connections** (4× KY-040 `VCC` + TFT `LED` + TFT `RESET`) and **7× `GND` connections** (4× KY-040 `GND` + TFT `GND` + Reset Button `GND` + Battery `GND`). In a permanent enclosure, makers typically solder a small stripboard/perfboard power bus or splice wires together.
2. **Panel-Mount Reset Button (`PBS-33B`) & Power Switch:**
   * Panel-mount switches come with metal solder lugs rather than 2.54mm DuPont pins.
3. **Display Header Pins (Sometimes):**
   * About 80% of `MSP2807` / `ILI9341` modules ship with the 9-pin male header **already soldered**, while ~20% include the header loose in the anti-static bag. Check listing photos to confirm pins are pre-soldered.
4. **Battery Boost Module (Option B/C):**
   * Boards like the `PowerBoost 1000C` require soldering the 2-wire `5V`/`GND` output header and the 2-wire On/Off switch.

### How to Build It 100% Solderless (If You Don't Want to Solder)
If you do not own a soldering iron or want a completely solder-free build:
1. **Buy Pre-Soldered Modules:** Verify product photos show pins pre-soldered on the **ESP32 DevKit**, **ILI9341 TFT**, and **KY-040 Encoders** (KY-040s almost always come pre-soldered).
2. **Use Wago Lever Nuts or a Mini Breadboard for `3V3` and `GND`:**
   * Option 1: Stick a **Mini 170-Tie-Point Breadboard** (`SYB-170`, peel-and-stick backing) inside your enclosure and use Male-to-Female DuPont wires to create a 7-pin `3V3` row and a 7-pin `GND` row.
   * Option 2: Strip one end of your DuPont wires and clamp them together inside two **Wago 221-415 (5-port) Lever Nuts** (one for `3V3`, one for `GND`).
3. **Use an 18650 Battery Shield V3 (Option A):** Has pre-soldered headers, a built-in On/Off switch, and a USB-A port.
4. **Use Quick-Disconnect Spade Crimps or a Pre-Wired Button:** Buy a `PBS-33B` button that comes with **pre-soldered wire leads** ([Amazon Search — `Pre-Wired 12mm Momentary Push Button`](https://www.amazon.com/s?k=Pre-Wired+12mm+Momentary+Push+Button+Red)), then clamp or plug those leads straight into your DuPont harness.

---

## 5. Complete Physical Wiring Reference Table

When assembling the physical hardware (extending [`diagram.json`](diagram.json) with real-world power & backlight pins), wire according to this table:

| Device | Device Pin | Connects To | Wire Color (Ref) | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Power / Battery** | `5V Out` | `ESP32 VIN` (5V) | Red | Regulated 5V from PowerBoost / 18650 Shield |
| | `GND` | `Shared GND Bus` | Black | Common ground reference |
| **ILI9341 TFT** | `VCC` | `ESP32 VIN` (5V) or `3V3` | Red | If module has `J1` regulator jumper open, accepts 5V or 3.3V |
| | `GND` | `Shared GND Bus` | Black | |
| | `CS` | `ESP32 GPIO 21` | Purple | Chip Select |
| | `RESET` | `Shared 3V3 Bus` | Red / White | Pull HIGH to `3.3V` on physical module (since `TFT_RST = -1`) |
| | `DC` / `RS` | `ESP32 GPIO 22` | Gray | Data / Command |
| | `SDI` / `MOSI` | `ESP32 GPIO 23` | Green | VSPI MOSI |
| | `SCK` | `ESP32 GPIO 18` | Yellow | VSPI Clock |
| | `LED` | `Shared 3V3 Bus` | Red | Backlight power (`3.3V`) |
| | `SDO` / `MISO` | `ESP32 GPIO 19` | Blue | VSPI MISO |
| **P1 Knob (Top-Left)** | `CLK` | `ESP32 GPIO 25` | Green | Interrupt-driven quadrature A |
| | `DT` | `ESP32 GPIO 26` | Blue | Quadrature B |
| | `SW` | `ESP32 GPIO 27` | Orange | Pushbutton switch |
| | `+` (`VCC`) | `Shared 3V3 Bus` | Red | **MUST BE 3.3V** |
| | `GND` | `Shared GND Bus` | Black | |
| **P2 Knob (Top-Right)** | `CLK` | `ESP32 GPIO 5` | Green | Interrupt-driven quadrature A |
| | `DT` | `ESP32 GPIO 17` (`TX2`) | Blue | Quadrature B |
| | `SW` | `ESP32 GPIO 16` (`RX2`) | Orange | Pushbutton switch |
| | `+` (`VCC`) | `Shared 3V3 Bus` | Red | **MUST BE 3.3V** |
| | `GND` | `Shared GND Bus` | Black | |
| **P3 Knob (Bottom-Left)**| `CLK` | `ESP32 GPIO 33` | Green | Interrupt-driven quadrature A |
| | `DT` | `ESP32 GPIO 14` | Blue | Quadrature B |
| | `SW` | `ESP32 GPIO 32` | Orange | Pushbutton switch |
| | `+` (`VCC`) | `Shared 3V3 Bus` | Red | **MUST BE 3.3V** |
| | `GND` | `Shared GND Bus` | Black | |
| **P4 Knob (Bottom-Right)**| `CLK` | `ESP32 GPIO 4` | Green | Interrupt-driven quadrature A |
| | `DT` | `ESP32 GPIO 34` | Blue | Input-only GPI; relies on KY-040 pull-up to `3V3` |
| | `SW` | `ESP32 GPIO 15` | Orange | Pushbutton switch |
| | `+` (`VCC`) | `Shared 3V3 Bus` | Red | **MUST BE 3.3V** |
| | `GND` | `Shared GND Bus` | Black | |
| **Red Table Button** | `Pin 1` | `ESP32 GPIO 13` | Red | Uses internal `INPUT_PULLUP` |
| | `Pin 2` | `Shared GND Bus` | Black | Active-low when pressed |

