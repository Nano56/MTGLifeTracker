# MTG Commander Life Tracker

A dedicated microcontroller-based physical life counter designed for Magic: The Gathering (MTG) Commander (EDH) games.

## Overview
Commander games present unique tabletop challenges:
- 4 players seated around a table.
- Starting life total: 40 per player.
- Additional counters: Commander Combat Damage (tracked per opponent commander, lethal at $\ge 21$ from a single commander, reduces life in tandem per MTG CR 903.10a / 704.6c), Poison (lethal at 10), Commander Tax (+2 mana per cast), and turn order.

This project builds an intuitive, easy-to-read, tabletop hardware device for tracking game state without relying on phone apps that drain phone batteries or go to sleep.

For detailed hardware choices, GPIO pin budgets, and design rationale, see [docs/HARDWARE_SPEC.md](docs/HARDWARE_SPEC.md).

---

## Repository Structure

```text
MTGLifeTracker/
├── docs/                       # Architecture decisions and hardware specifications
│   └── HARDWARE_SPEC.md        # ESP32 pin budget, component rationale, power plan
├── scripts/                    # Build scripts and test runners
│   ├── run_tests.ps1           # PowerShell native test runner
│   ├── run_tests.bat           # Windows CMD batch test runner
│   └── run_tests.py            # Cross-platform Python test runner
├── tests/                      # 100% native host C++ automated test suite (no emulator needed)
│   ├── test_gamestate.cpp      # Rules, counter arithmetic, MTG lethal conditions
│   ├── test_eventlog.cpp       # Ring buffer FIFO and string formatting
│   ├── test_inputdecoder.cpp   # Quadrature Gray-code decoding, debouncing, double-clicks
│   ├── test_scenarios.cpp      # Multi-turn combat match simulations
│   ├── test_display.cpp        # Native compilation and layout validation of DisplayManager
│   ├── Adafruit_GFX.h          # Host mock header for native display testing
│   └── Adafruit_ILI9341.h      # Host mock header for native display testing
├── tools/                      # Developer tools & desktop utilities
│   └── cli_simulator.cpp       # Interactive ANSI terminal simulator (HID agnostic)
├── diagram.json                # Wokwi circuit simulation wiring (ESP32 + ILI9341 + 4x KY-040)
├── libraries.txt               # Wokwi Arduino library dependencies
├── wokwi.toml                  # Wokwi simulation configuration
├── sketch.ino                  # Main firmware setup and loop
├── Config.h                    # Pinouts, color palette, timing thresholds
├── GameState.h / .cpp          # 4-player state model & MTG rules engine
├── EventLog.h / .cpp           # 50-entry circular event log
├── InputDecoder.h / .cpp       # KY-040 rotary decoder & button debouncer
├── AppController.h / .cpp      # Normalized HID action dispatcher
├── DisplayManager.h / .cpp     # 320x240 ILI9341 4-quadrant UI engine
└── to_fix.md                   # Project bug tracker & resolved remediation audit
```

---

## Validation & Testing

### 1. Run Native Automated Tests
Execute all 242 unit, display, and scenario tests locally without opening any emulator:
```powershell
.\scripts\run_tests.ps1
```
*(Or `.\scripts\run_tests.bat` in CMD / `python scripts/run_tests.py`)*

### 2. Interactive Desktop Simulator
Test gameplay and rules in your terminal with keyboard controls:
```powershell
.\tools\cli_simulator.exe
```

### 3. Wokwi Hardware Simulation
Open `diagram.json` in VS Code / Antigravity IDE with the Wokwi extension and click **Start Simulation**.

---

## Features Implemented
- [x] 4-player quadrant display on 320×240 ILI9341 color TFT.
- [x] Starting life total: 40 per player.
- [x] Commander Damage per opponent: tracked independently for each opponent, reduces life in tandem, lethal at $\ge 21$ from any single commander (CR 903.10a / 704.6c).
- [x] Dual-counter view: Large Life total alongside 3-row opponent Commander Damage breakdown.
- [x] Instant return to Life mode: double-click knob jumps straight to LIFE from any counter mode.
- [x] Poison counter tracking: lethal at 10.
- [x] Commander tax tracker: increments by $\pm 2$ mana per cast.
- [x] 50-entry circular event log streaming live to USB Serial and viewable on screen (double-click table button).
- [x] Random First Player roll roulette animation with highlight clearing.
- [x] Dedicated tactile Game Reset button (hold 1.5s for full reset; single player reset requires 3s hold).
- [x] Safe ESP32 pin budget: avoids boot strapping hazards (GPIO 12 and GPIO 2 remapped to GPIO 32 and GPIO 34).
- [x] 100% native host C++ test validation with mock GFX engine.
