# MTG Commander Life Tracker

A dedicated microcontroller-based physical life counter designed for Magic: The Gathering (MTG) Commander (EDH) games.

## Overview
Commander games present unique tabletop challenges:
- 4 players seated around a table.
- Starting life total: 40 per player.
- Additional counters: Commander Health (starts at 21, lethal at 0), Poison (lethal at 10), Commander Tax, and turn order.

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
├── tests/                      # 100% native C++ automated test suite
│   ├── test_gamestate.cpp      # Rules, counter arithmetic, lethal conditions
│   ├── test_eventlog.cpp       # Ring buffer FIFO and string formatting
│   ├── test_inputdecoder.cpp   # Quadrature Gray-code decoding and debouncing
│   └── test_scenarios.cpp      # Multi-turn combat match simulations
├── tools/                      # Developer tools & desktop utilities
│   └── cli_simulator.cpp       # Interactive ANSI terminal simulator (HID agnostic)
├── diagram.json                # Wokwi circuit simulation wiring
├── libraries.txt               # Wokwi Arduino library dependencies
├── wokwi.toml                  # Wokwi simulation configuration
├── sketch.ino                  # Main firmware setup and loop
├── Config.h                    # Pinouts, color palette, timing thresholds
├── GameState.h / .cpp          # 4-player state model & MTG rules engine
├── EventLog.h / .cpp           # 50-entry circular event log
├── InputDecoder.h / .cpp       # KY-040 rotary decoder & button debouncer
├── AppController.h / .cpp      # Normalized HID action dispatcher
└── DisplayManager.h / .cpp     # 320x240 ILI9341 4-quadrant UI engine
```

---

## Validation & Testing

### 1. Run Native Automated Tests
Execute all 162 unit and scenario tests locally without opening any emulator:
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
- [x] Commander Health: starts at 21, counted separately, lethal at $\le 0$.
- [x] Dual-counter view: Life and Commander Health displayed side-by-side.
- [x] Poison counter tracking: lethal at 10.
- [x] Commander tax tracker: increments by $\pm 2$ mana per cast.
- [x] 50-entry circular event log streaming live to USB Serial.
- [x] Random First Player roll roulette animation.
- [x] Dedicated tactile Game Reset button (hold 2s for full reset).
- [x] Zero-flicker partial display updates.
- [x] 100% automated native C++ test coverage.
