#ifndef INPUT_DECODER_H
#define INPUT_DECODER_H

#include <stdint.h>
#include <stdbool.h>
#include "Config.h"

enum ButtonEvent {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_SHORT_CLICK,
    BTN_EVENT_LONG_PRESS,
    BTN_EVENT_DOUBLE_CLICK
};

class RotaryDecoder {
public:
    RotaryDecoder();

    // Update with latest digital levels of CLK and DT pins.
    // Returns: +1 for clockwise click, -1 for counter-clockwise click, or 0 if no full detent.
    int8_t update(bool clkLevel, bool dtLevel);

    // Reset internal state
    void reset();

private:
    uint8_t state;
    int8_t  subCount;
};

class ButtonDebouncer {
public:
    ButtonDebouncer();

    // Configure long press duration threshold (default: LONG_PRESS_MS)
    void setLongPressMs(uint32_t ms);

    // Update with current pin level (true if pressed / LOW on active-low pin) and timestamp.
    // Returns ButtonEvent if an event triggered this tick.
    ButtonEvent update(bool isPressed, uint32_t currentMs);

    // Reset debouncer
    void reset();

private:
    enum DebounceState {
        STATE_RELEASED,
        STATE_PRESSED
    };

    DebounceState state;
    bool lastRawLevel;
    uint32_t lastDebounceTime;
    uint32_t pressStartTime;
    uint32_t longPressThresholdMs;
    bool longPressEmitted;

    // Double-click tracking
    uint32_t lastClickTime;
    bool waitingForSecondClick;
};

#endif // INPUT_DECODER_H
