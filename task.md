# Remediation Tasks (from Code Review)

Track progress and implementation details for each task identified in the comprehensive code review.

- [x] **Task 1: Fix AppController Non-Random ACT_ROLL_FIRST (M5)**
  - Problem: `rollStartingPlayer(currentMs, currentMs)` uses `(2 * currentMs) % count`. For 2P, always chooses P1 (even % 2 == 0). For 4P, only chooses P1 or P3.
  - Solution: Update `AppController` to mix/hash `currentMs` or accept an entropy seed so all player indices 0..count-1 can be rolled.
  - Tests: Add `test_roll_fairness()` in `tests/test_scenarios.cpp`.
  - Status: Completed (Verified with 1868 assertions in `test_scenarios`).

- [x] **Task 2: Fix ButtonDebouncer Hold & Double-Click Conflict (H1, M1, L2)**
  - Problem: `SHORT_CLICK` is emitted immediately on first release, conflicting with double-click. 3-second hold fires 1.5s `LONG_PRESS` before 3.0s `VERY_LONG_PRESS`. Calling `reset()` clears customized threshold timings.
  - Solution: In `ButtonDebouncer`, preserve thresholds in `reset()`. Support deferred single-click mode (`setDeferSingleClick(true)`). Ensure `VERY_LONG_PRESS` does not emit `LONG_PRESS` while button is held.
  - Tests: Add unit tests in `tests/test_inputdecoder.cpp` for deferred click, double-click suppression, and very long press isolation.
  - Status: Completed (Verified with 27 assertions in `test_inputdecoder`).

- [x] **Task 3: Fix sketch.ino Event Log & Table Button Flow (H1, M1, L6)**
  - Problem: Table button double-click cannot open event log because single click triggers roll animation which blocks polling. 1.5s long press resets game and erases flash before 3s player count menu opens. Controls guide is duplicated.
  - Solution: Enable deferred single-click on `tableButton`. Ensure 3s hold only opens menu. Consolidate quick controls print helper.
  - Tests: Verify via `tests/test_scenarios.cpp` and test runners.
  - Status: Completed (Verified with 44 assertions in `test_display` and 1868 assertions in `test_scenarios`).

- [x] **Task 4: Fix Storage Bounds Checking & Restored Timestamps (M2, M3, L1)**
  - Problem: Event log timestamps go backwards after power loss restore because restored events keep old millis while new events start at boot millis. `eventCount` is not clamped to `EVENT_LOG_CAPACITY`. Failed log write clears dirty flag.
  - Solution: Clamp `eventCount` to `EVENT_LOG_CAPACITY`. Add timestamp offset continuity. Return false if log blob fails and keep dirty flag.
  - Tests: Update `tests/test_storage.cpp` with corrupted event count test and timestamp continuity verification.
  - Status: Completed (Verified with 59 assertions in `test_storage`).

- [x] **Task 5: Fix DisplayManager Text Wrapping & Toast Timing (L3, L5, L7)**
  - Problem: `"*** DEAD: 21 CMDR (P%d) ***"` wraps past 160px width. `showToast` called with `currentMs = 0`. Redundant `drawStaticLayout()` calls inside `renderQuadrant`.
  - Solution: Truncate lethal text to `"DEAD: 21 CMDR P%d"`. Pass active `currentMs` to `showToast`. Remove per-quadrant `drawStaticLayout()`.
  - Tests: Update `tests/test_display.cpp`.
  - Status: Completed (Verified with 47 assertions in `test_display`).

- [x] **Task 6: Fix Self-Test Animation Freeze & Add Assertions (M4)**
  - Problem: Self-test roll animation step freezes because `updateAnimation` returns early when screen is `SCREEN_SELF_TEST`. Steps print `[PASS]` unconditionally without verifying state.
  - Solution: Allow `updateAnimation` during self-test roll animation step. Add validation assertions before printing `[PASS]`.
  - Tests: Update `tests/test_display.cpp`.
  - Status: Completed (Verified with 47 assertions in `test_display`).

- [x] **Task 7: Protect Encoder Polling with Step Accumulation Buffer (H2)**
  - Problem: Synchronous polling in `loop()` drops steps during 38.4KB SPI redraw bursts.
  - Solution: Add step accumulation counter/buffer in `InputDecoder` / `sketch.ino` that can capture fast detents.
  - Tests: Update `tests/test_inputdecoder.cpp`.
  - Status: Completed (Verified with 34 assertions in `test_inputdecoder`).
