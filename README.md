# MTG Commander Life Tracker

A dedicated microcontroller-based physical life counter designed for Magic: The Gathering (MTG) Commander (EDH) games.

## Overview
Commander games present unique tabletop challenges:
- 2 to 4 players seated around a table.
- Starting life total of 40 per player.
- Additional counters: Commander Damage (lethal at 21), Poison (lethal at 10), Commander Tax, and turn order.

This project aims to build an intuitive, easy-to-read, tabletop hardware device for tracking game state without relying on phone apps that drain phone batteries or go to sleep.

For detailed hardware choices, GPIO pin budgets, and design rationale, see [HARDWARE_SPEC.md](HARDWARE_SPEC.md).

---

## Hardware Approaches (Brainstorming)

### Option 1: All-in-One Touchscreen (e.g., ESP32 "CYD" Cheap Yellow Display)
- **MCU & Display:** ESP32 with integrated 2.8"–4.0" TFT/IPS touch display.
- **Form Factor:** Central flat console on the table with a 4-quadrant UI facing each player.
- **Pros:** Minimal assembly / soldering; flexible UI; compact.

### Option 2: Central Display + Tactile Buttons
- **MCU:** Raspberry Pi Pico (RP2040) or ESP32.
- **Display:** 2.4"–3.5" SPI TFT or OLED.
- **Controls:** 8 tactile push buttons (+ / - per player).
- **Pros:** Satisfying physical feedback; reliable inputs.

### Option 3: Multi-Display Tower / Pyramid
- **MCU:** ESP32 or RP2040.
- **Displays:** 4 individual mini OLED displays (SSD1306) angled outward toward each player.
- **Controls:** Dedicated buttons or rotary encoders for each player.
- **Pros:** Ideal viewing angles across the whole table.

---

## Feature Roadmap

### Phase 1: MVP (Minimum Viable Product)
- [ ] 2 to 4 player support.
- [ ] Starting life total: 40.
- [ ] Life increment/decrement (`+1`, `-1`, hold for `+5`/`-5`).
- [ ] Game reset button.

### Phase 2: Commander Essentials
- [ ] Commander damage tracking per player (lethal at 21).
- [ ] Poison counter tracking (lethal at 10).
- [ ] Random first player selector (digital coin flip / die roll).
- [ ] Commander tax tracker ($+2$ mana per cast).

### Phase 3: Hardware & Aesthetics
- [ ] Enclosure (3D printed tabletop case).
- [ ] Rechargeable LiPo / 18650 battery power with USB-C charging.
- [ ] Custom player colors / themes.
