#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "GameState.h"
#include "EventLog.h"

// Abstract Human Interface Device (HID) Actions
// Any physical input device (rotary encoder, tactile button, touchscreen,
// keypad, BLE command, or terminal key) translates into these actions.
enum HIDAction {
    ACT_NONE = 0,
    ACT_INCREMENT,        // e.g. Dial CW, "+" button tap, touch "+" zone, UP arrow
    ACT_DECREMENT,        // e.g. Dial CCW, "-" button tap, touch "-" zone, DOWN arrow
    ACT_CYCLE_MODE,       // e.g. Dial click, mode button tap, touch header badge
    ACT_RESET_PLAYER,     // e.g. Dial long press, player hold
    ACT_ROLL_FIRST,       // e.g. Table button short click, touch "Roll" button
    ACT_RESET_GAME,       // e.g. Table button long press, touch "Reset All" button
    ACT_TOGGLE_LOG        // e.g. Double click, touch "Log" button
};

class AppController {
public:
    AppController(GameState& stateRef, EventLog& logRef);

    // Primary entry point for any HID event
    bool dispatchAction(uint8_t playerIdx, HIDAction action, uint32_t currentMs);

    // Periodic tick to handle time-based rules (e.g. 5-second auto-return to LIFE)
    void tick(uint32_t currentMs);

    // Pick a starting player randomly (0..NUM_PLAYERS-1)
    uint8_t rollStartingPlayer(uint32_t currentMs, uint32_t seed);

    GameState& state;
    EventLog&  log;
};

#endif // APP_CONTROLLER_H

