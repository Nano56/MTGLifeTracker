# Development, Build, and Testing Workflow

This guide explains how to develop, build, test, and simulate the **MTG Commander Life Tracker** project. It is written for both human contributors and AI coding agents.

---

## 1. Architecture & Design Principles

The project follows a modular, layered embedded architecture designed for rapid development:

```text
┌────────────────────────────────────────────────────────┐
│                   sketch.ino (Arduino)                 │
│         Hardware Polling, Pin Config, FreeRTOS Yield   │
└───────────────┬────────────────────────┬───────────────┘
                │                        │
                ▼                        ▼
     ┌────────────────────┐   ┌────────────────────┐
     │   InputDecoder     │   │   DisplayManager   │
     │  Quadrature Gray   │   │  4-Quadrant UI,    │
     │  Button Debounce   │   │  Tabletop Mirror,  │
     │ (Click/Dbl/Hold)   │   │  Zero-Flicker Blit │
     └──────────┬─────────┘   └──────────▲─────────┘
                │                        │
                ▼                        │
     ┌────────────────────┐              │
     │   AppController    │──────────────┘
     │ Normalized Actions │
     └──────────┬─────────┘
                │
                ▼
     ┌────────────────────┐   ┌────────────────────┐
     │     GameState      │──▶│      EventLog      │
     │ MTG Rules, Counters│   │ Circular Ring Log  │
     │ Lethal Conditions  │   │  Serial & On-Screen│
     └──────────┬─────────┘   └──────────┬─────────┘
                │                        │
                ▼                        ▼
     ┌─────────────────────────────────────────────┐
     │               StorageManager                │
     │   Debounced NVS Flash Persistence (2.5s)    │
     │   Fletcher-32 Checksums, Power Recovery     │
     └─────────────────────────────────────────────┘
```

### Key Principles
1. **Hardware-Agnostic Core**: `GameState`, `EventLog`, `InputDecoder`, `AppController`, and `StorageManager` have zero hardware dependencies. They compile and test on any host system (Windows, Linux, macOS) using standard C++ (C++11 or higher) via clean mock persistence abstractions.
2. **Tabletop Mirroring**: Players 1 and 2 sit across the table from Players 3 and 4. The top half of the screen is rendered rotated 180° into an off-screen `GFXcanvas16` buffer so each seated player reads their quadrant right-side up.
3. **Zero-Flicker Partial Redraws**: Quadrants are rendered in an off-screen canvas buffer and pushed in a single SPI burst via `tft.drawRGBBitmap()`.
4. **Pin Safety**: Strapping-pin conflicts are strictly avoided (GPIO 12 MTDI and GPIO 2 boot pins are not used; remapped to GPIO 32 and GPIO 34). All encoders are powered from 3.3V.
5. **Flash Wear-Leveling & Power Loss Recovery**: Active match states (player count, life, counters, commander damage, deaths, and event log ring buffer) are auto-saved to non-volatile storage (NVS) using a 2.5-second inactivity debounce timer. On boot, if a valid match is detected, the board state is seamlessly restored, displaying a brief `[ MATCH RESTORED ]` emerald toast banner.

---

## 2. Prerequisites & Environment Setup

### Required Tools
- **C++ Host Compiler (`g++`)**: MinGW-w64 on Windows, or GCC/Clang on Linux/macOS. Used for the instant native test suite and CLI simulator.
- **Python 3.10+**: Used for test runner automation and PlatformIO.
- **PlatformIO Core**: Embedded build system that compiles firmware for the ESP32.
- **VS Code**: Recommended IDE with the following extensions:
  - **C/C++** (`ms-vscode.cpptools`)
  - **Wokwi Simulator** (`wokwi.wokwi-vscode`)

### Virtual Environment Setup
A dedicated Python virtual environment contains PlatformIO:
```powershell
# Create virtual environment (if not already created)
python -m venv .venv

# Install PlatformIO inside the venv
.\.venv\Scripts\python.exe -m pip install -U platformio
```

---

## 3. Development Inner Loop

### Step 1: Logic Development & Native Testing (Fastest)
When adding rules, counters, input behaviors, or menu modes, you **do not** need to flash hardware or wait for an emulator.

Run all 327 automated tests natively (runs in < 5 seconds):
```powershell
# PowerShell (interactive with pause & test report)
.\scripts\run_tests.ps1

# PowerShell (non-blocking for automated runs/CI)
.\scripts\run_tests.ps1 -NoPause

# Windows Batch CMD (pauses after report)
.\scripts\run_tests.bat
.\scripts\run_tests.bat /nopause

# Cross-platform Python
python scripts/run_tests.py
python scripts/run_tests.py --no-pause
```

#### Test Execution Report & Automated Logging
Running any of the test scripts automatically:
1. **Compiles & executes** all 6 test targets with zero warnings.
2. **Generates a full structured Test Execution Report** detailing assertions passed, individual suite execution times, and overall verdict.
3. **Persists full timestamped logs** to `logs/test_run_<timestamp>.log` and `logs/test_run_latest.log` for debugging and review.
4. **Pauses interactively** (`Press [Enter] to continue...`) in interactive terminals so you can comfortably read the report before the console closes. Use `-NoPause` (or `/nopause`, `--no-pause`) in automated workflows.

#### Test Suite Breakdown
| Suite | File | What It Tests |
|---|---|---|
| `test_gamestate` | [`tests/test_gamestate.cpp`](tests/test_gamestate.cpp) | MTG Commander rules, starting life (40), lethal commander damage ($\ge 21$ from single opponent), commander damage reducing life in tandem, poison counters (lethal at 10), commander tax stepping ($\pm 2$), life limits, 2P/3P/4P dynamic player bounds. |
| `test_eventlog` | [`tests/test_eventlog.cpp`](tests/test_eventlog.cpp) | 50-item circular ring buffer, timestamp formatting, event string serialization. |
| `test_inputdecoder` | [`tests/test_inputdecoder.cpp`](tests/test_inputdecoder.cpp) | Quadrature Gray-code decoding, contact bounce rejection (< 35ms), short click release detection, rapid double-click detection, long-press thresholds (1.5s vs 3.0s). |
| `test_scenarios` | [`tests/test_scenarios.cpp`](tests/test_scenarios.cpp) | Full multi-turn match simulations combining HID inputs, multi-player combat damage, poison elimination, and game resets. |
| `test_display` | [`tests/test_display.cpp`](tests/test_display.cpp) | Native compilation of `DisplayManager`, screen mode state machine, startup self-test sequence, 2P/3P/4P layouts, 180° rotation, and roll animation lifecycle. |
| `test_storage` | [`tests/test_storage.cpp`](tests/test_storage.cpp) | Power loss recovery, debounced NVS commit timing (2500ms), packed byte serialization with Fletcher-32 checksums, round-trip state & event log restoration, dynamic 2P/3P/4P modes, and match reset purge. |

---

### Step 2: Interactive Terminal Simulation
To manually test input flows, mode cycling, and event logging without launching a graphical simulator:
```powershell
# Run the compiled CLI simulator
.\tools\cli_simulator.exe
```
Controls:
- Select player: `1` – `4`
- Turn dial: `+` / `-`
- Click knob: `c` (cycle mode)
- Double-click knob: `l` (instant return to life)
- Hold knob: `p` (reset player)
- Table button: `r` (roll starting player), `R` (reset game), `e` (view match log)
- Quit: `q`

---

### Step 3: Compiling ESP32 Firmware (PlatformIO)

The firmware is compiled targeting the ESP32 DevKit using PlatformIO:

```powershell
# Build firmware binary and ELF
.\.venv\Scripts\pio.exe run
```
*(Or press `Ctrl+Shift+B` in VS Code to trigger the default build task).*

#### PlatformIO Configuration ([`platformio.ini`](platformio.ini))
- Target: `esp32dev` (Espressif 32 platform)
- Framework: `arduino`
- Core Directory: `.platformio` (isolated to workspace)
- Output Artifacts:
  - `.pio/build/esp32dev/firmware.bin` (flash image)
  - `.pio/build/esp32dev/firmware.elf` (debug symbols & simulation)
- Dependencies:
  - `adafruit/Adafruit GFX Library@^1.11.9`
  - `adafruit/Adafruit ILI9341@^1.6.0`

---

### Step 4: Hardware Simulation (Wokwi)

The hardware circuit is defined in [`diagram.json`](diagram.json) and configured for simulation in [`wokwi.toml`](wokwi.toml).

#### How to Run
1. Open [`diagram.json`](diagram.json) in VS Code.
2. Click the green **Play / Start Simulation** button in the diagram tab (or press `F1` &rarr; `Wokwi: Start Simulator`).
3. Wokwi boots the ESP32, mounts the ILI9341 display, wires the 4 rotary encoders, and opens the serial monitor.

#### Startup Self-Test & Visual Demo Sequence
When the device powers on or resets, an automated visual self-test sequence runs to validate the display, graphics, and rules engine:
1. **Color Palette & Hardware Banner**: Validates RGB565 primary player colors (Red, Cyan, Green, Gold).
2. **4-Player Layout**: Demonstrates the 4-quadrant layout with 180° inverted rendering for top seated players (P1 & P2).
3. **Multi-Mode Counters**: Automatically steps counters (Life -5 on P1, Commander Damage +5 on P2, Poison +3 on P3, Commander Tax +1 on P4).
4. **Lethal Eliminaton Alerts**: Triggers lethal conditions to show the skull icon and death banners (P3 poison lethal at 10, P2 commander lethal at 21).
5. **Match Event Log**: Inspects the real-time event log containing all preceding combat and lethal actions.
6. **3-Player Dynamic Layout**: Demonstrates 1 wide top half (P1) and 2 bottom quads (P2 & P3).
7. **2-Player Dynamic Layout**: Demonstrates 2 wide split-screen halves (P1 top flipped, P2 bottom normal).
8. **Player Count Setup Menu**: Displays the interactive 2P / 3P / 4P selector screen.
9. **First-Player Roll Roulette**: Runs the randomized spinner animation and displays the celebration banner.
10. **Done / Playable Match**: Automatically resets all players to 40 Life and enters live gameplay.

> [!TIP]
> **Instant Skip**: Turn any knob or click any button at any moment during the startup test sequence to immediately skip ahead to a clean, playable 4-player game.

#### Interactive Controls in Wokwi
- **Rotate Knob**: Hover mouse over knob and drag the circular dial clockwise / counter-clockwise.
- **Push Knob**: Click the center shaft of the knob.
  - *Single Click*: Cycle modes (`LIFE` &rarr; `CMDR P(x)` &rarr; `POISON` &rarr; `TAX`).
  - *Double Click*: Instant jump back to `LIFE` mode.
  - *Hold 3s*: Reset player to 40 Life.
- **Red Table Button**:
  - *Single Click*: Roll random starting player (triggers clockwise roulette animation).
  - *Double Click*: Open full-screen **Match Event Log** (click again to return).
  - *Hold 1.5s*: Full game reset (all active players reset to 40 Life).
  - *Hold 3.0s*: Open **Player Count Setup Menu** (configure 2, 3, or 4 players). Turn any knob to choose, click knob or button to confirm.

---

## 4. Hardware Pin Mapping & Wiring Reference

Matches [`Config.h`](Config.h), [`HARDWARE_SPEC.md`](docs/HARDWARE_SPEC.md), and [`diagram.json`](diagram.json):

| Subsystem | Signal | ESP32 GPIO | Notes |
|---|---|---|---|
| **ILI9341 Display** | SCK | GPIO 18 | VSPI Clock |
| | MOSI | GPIO 23 | VSPI Data Out |
| | MISO | GPIO 19 | VSPI Data In |
| | CS | GPIO 21 | Chip Select |
| | D/C | GPIO 22 | Data/Command |
| | RST | -1 | Software Reset |
| | VCC | 3.3V / VIN | Powered from 3V3 rail |
| **Player 1 Knob (Top-Left)** | CLK | GPIO 25 | Input Pullup |
| | DT | GPIO 26 | Input Pullup |
| | SW | GPIO 27 | Input Pullup |
| **Player 2 Knob (Top-Right)** | CLK | GPIO 5 | Safe strapping pin |
| | DT | GPIO 17 (TX2) | Input Pullup |
| | SW | GPIO 16 (RX2) | Input Pullup |
| **Player 3 Knob (Bottom-Left)** | CLK | GPIO 33 | Input Pullup |
| | DT | GPIO 14 | Input Pullup |
| | SW | GPIO 32 | Remapped from strapping pin 12 |
| **Player 4 Knob (Bottom-Right)** | CLK | GPIO 4 | Input Pullup |
| | DT | GPIO 34 | GPI Input-only with module pullup; remapped from pin 2 |
| | SW | GPIO 15 | Safe strapping pin |
| **Table Button (Reset/Roll)** | PIN | GPIO 13 | Active-low with internal pullup |

> [!WARNING]
> **Power All Encoders from 3.3V (`3V3`)**: The KY-040 breakout board has on-board 10 kΩ pull-up resistors. Wiring encoder VCC to 5V (`VIN`) will put 5V on ESP32 GPIO inputs and damage the chip.

---

## 5. Adding New Features or Modifying Rules

When implementing changes, follow this checklist:

1. **Rules & State**: Update constants in [`Config.h`](Config.h) and state methods in [`GameState.h`](GameState.h)/[`.cpp`](GameState.cpp).
2. **Event Logging**: If a new event is introduced, add an `EventType` to [`EventLog.h`](EventLog.h) and format string in [`EventLog.cpp`](EventLog.cpp).
3. **HID Actions**: Update [`AppController.h`](AppController.h)/[`.cpp`](AppController.cpp) if a new user gesture is handled.
4. **Display**: Adjust UI elements in [`DisplayManager.cpp`](DisplayManager.cpp) using local quadrant coordinates `(0, 0)` to `(160, 120)`. The `quadCanvas` automatically takes care of mirroring for top-half players.
5. **Update Native Tests**: Add test cases to [`tests/test_gamestate.cpp`](tests/test_gamestate.cpp) or [`tests/test_scenarios.cpp`](tests/test_scenarios.cpp). Run `.\scripts\run_tests.ps1`.
6. **Compile Firmware**: Run `.\.venv\Scripts\pio.exe run`.
7. **Simulate in Wokwi**: Test in Wokwi emulator to confirm interactive UX.

