# Hardware Specification & Architecture Decisions (ADR)

This document records the architectural choices, component selections, pin allocations, and design rationale for the MTG Commander Life Tracker.

---

## 1. Context & Design Goals
Magic: The Gathering (MTG) Commander (EDH) typically involves:
- **4 players** seated around a rectangular or square table.
- **Starting life total: 40** per player.
- **Dynamic counter types:** Life, Commander Damage from 3 opponents (lethal at 21), and Poison counters (lethal at 10).
- **Physical ergonomics:** Players are seated 2–4 feet away from the device; inputs must be fast, tactile, and unambiguous to avoid bumping or misclicks.
- **Simulation requirement:** The design must be 100% emulatable in software (Wokwi) before committing to buying physical components.

---

## 2. Key Architectural Decisions

### Decision 1: Microcontroller — ESP32 DevKit (WROOM-32 / ESP32-D0WD)
* **Status:** Accepted
* **Rationale:**
  * **Processing & Speed:** Dual-core Xtensa 32-bit LX6 running at 240 MHz. Easily drives SPI color graphics at high frame rates while running encoder debouncing interrupts concurrently.
  * **GPIO Pin Count:** Standard 38-pin DevKit board exposes ~28 usable GPIOs. This is enough to host a SPI display, touch controller, 4 rotary encoders with push switches, and auxiliary buttons without needing external I2C I/O expanders.
  * **Cost & Availability:** Available globally for ~$3.50 – $5.00.
  * **Emulation Support:** Natively supported in Wokwi simulator with cycle-accurate timing and peripheral emulation.
* **Alternatives Considered:**
  * *Arduino Uno / Nano (ATmega328P):* Insufficient RAM (2 KB) to buffer color display graphics; too few GPIO pins (14).
  * *Raspberry Pi (Zero / 4 / 5):* Full Linux OS requires 15–30 second boot times, risk of SD card corruption on sudden power off, and high power consumption.
  * *Raspberry Pi Pico (RP2040):* Strong contender, but ESP32 offers built-in hardware timers, wider availability of pre-integrated display shields, and identical Wokwi support.

---

### Decision 2: Display & Touch — 2.8"–3.2" ILI9341 SPI TFT with XPT2046 Resistive Touch
* **Status:** Accepted
* **Rationale:**
  * **Form Factor & Resolution:** 320×240 resolution provides plenty of screen area for a 4-player quadrant split. High contrast color display makes life totals visible across the table.
  * **Bus Efficiency:** Shares a single high-speed SPI bus (MOSI, MISO, SCK) between both the display and the touch controller.
  * **Emulation:** Fully emulated in Wokwi, including interactive mouse-driven touch events.
* **Alternatives Considered:**
  * *0.96" OLEDs (SSD1306):* Monochromatic, small font size, hard to read from across a playmat.
  * *E-Paper / E-Ink:* Too slow refresh rate (~1–2 seconds per full update); unusable when rapidly adjusting life totals during combat.
  * *7-Segment Displays:* Cannot render menus, player names, or multi-counter modes (Commander damage / poison).

---

### Decision 3: Hybrid Input Interface (Touch + 4× Rotary Encoders + Push Buttons)
* **Status:** Accepted
* **Rationale:**
  * **The Problem with Touch-Only:** In a heated Commander game, repeatedly tapping a flat touchscreen 15 times to resolve a large combat swing can cause missed taps, wobbles the device, and lacks physical confirmation.
  * **The Problem with Buttons-Only:** Navigating menus, switching player counts (2 vs 3 vs 4), and resetting games using only buttons requires clumsy menu trees.
  * **The Hybrid Solution:**
    * **Rotary Encoders (EC11 with push-button):** Dedicated to each player's corner. Rotating the knob gives instant, clicky incremental feedback (+/- life). Pressing the knob cycles the active counter:
      $$\text{Life Total} \longrightarrow \text{Commander Damage} \longrightarrow \text{Poison Counters} \longrightarrow \text{Life Total}$$
    * **Touchscreen:** Handles high-level tabletop configuration (selecting 2, 3, or 4 players, resetting all life totals, starting player roll).
    * **Dedicated Physical Reset/Die Button:** One-touch hardware button for immediate game reset or rolling a digital die.

---

## 3. GPIO Pin Budget & Allocation

The standard ESP32 38-pin DevKit provides:
- **Standard GPIOs:** Bidirectional with internal pull-ups/pull-downs.
- **Input-Only GPIs (GPIO 34, 35, 36, 39):** Ideal for encoder inputs and buttons (external or software pull-ups).
- **Strapping Pins (GPIO 0, 2, 12, 15):** Assigned carefully so boot states are unaffected.

| Subsystem | Signal Name | ESP32 Pin | Notes / Hardware Detail |
| :--- | :--- | :--- | :--- |
| **SPI Bus (Shared)** | `SCK` | GPIO 18 | VSPI Clock |
| | `MOSI` | GPIO 23 | VSPI Data Out |
| | `MISO` | GPIO 19 | VSPI Data In (from Touch & TFT) |
| **ILI9341 TFT Display** | `TFT_CS` | GPIO 15 | Chip Select (Active Low) |
| | `TFT_DC` | GPIO 2 | Data / Command Select |
| | `TFT_RST` | GPIO 4 | Hardware Reset |
| **XPT2046 Touch** | `TOUCH_CS` | GPIO 21 | Touch Controller Chip Select |
| | `TOUCH_IRQ` | GPIO 22 | Touch Interrupt |
| **Player 1 Encoder** | `P1_CLK` | GPIO 25 | Quadrature Channel A |
| | `P1_DT` | GPIO 26 | Quadrature Channel B |
| | `P1_SW` | GPIO 27 | Integrated Push-Button (Switch) |
| **Player 2 Encoder** | `P2_CLK` | GPIO 13 | Quadrature Channel A |
| | `P2_DT` | GPIO 14 | Quadrature Channel B |
| | `P2_SW` | GPIO 32 | Integrated Push-Button (Switch) |
| **Player 3 Encoder** | `P3_CLK` | GPIO 33 | Quadrature Channel A |
| | `P3_DT` | GPIO 34 | Quadrature Channel B (Input-only pin) |
| | `P3_SW` | GPIO 35 | Integrated Push-Button (Input-only pin) |
| **Player 4 Encoder** | `P4_CLK` | GPIO 36 (VP) | Quadrature Channel A (Input-only pin) |
| | `P4_DT` | GPIO 39 (VN) | Quadrature Channel B (Input-only pin) |
| | `P4_SW` | GPIO 16 | Integrated Push-Button |
| **Global Hardware Button** | `BTN_RESET` | GPIO 17 | Tactile button for Quick Reset / Menu |

**Summary:** 20 GPIO pins utilized. 6+ pins remain available for expansions (e.g., Piezo buzzer for audio cues, WS2812 status LED, or LiPo battery voltage sensing).

---

## 4. Power & Electrical Considerations
- **Operating Voltage:** 3.3V logic level (native to ESP32 and ILI9341).
- **Prototyping Power:** Standard 5V USB-C or Micro-USB cable connected directly to the ESP32 dev board regulator.
- **Portable Expansion (Future):** Single-cell 3.7V LiPo battery with a TP4056 charging + boost module providing 5V to the board VIN pin.
