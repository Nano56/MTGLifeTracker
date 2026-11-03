#ifndef INPUT_DECODER_H
#define INPUT_DECODER_H

#include <stdint.h>
#include <stdbool.h>
#include "Config.h"

enum ButtonEvent {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_SHORT_CLICK,
    BTN_EVENT_LONG_PRESS,
    BTN_EVENT_DOUBLE_CLICK,
    BTN_EVENT_VERY_LONG_PRESS
};

class RotaryDecoder {
public:
    RotaryDecoder();

    // Update with latest digital levels of CLK and DT pins.
    // Returns: +1 for clockwise click, -1 for counter-clockwise click, or 0 if no full detent.
    int8_t update(bool clkLevel, bool dtLevel);

    // Reset internal state
    void reset();

    // Step accumulator methods for buffered / interrupt-driven acquisition
    void addStep(int8_t step);
    int16_t drainSteps();
    int16_t getPendingSteps() const;

private:
    uint8_t state;
    int8_t  subCount;
    volatile int16_t accumulatedSteps;
};

class ButtonDebouncer {
public:
    ButtonDebouncer();

    // Configure long press duration threshold (default: LONG_PRESS_MS)
    void setLongPressMs(uint32_t ms);

    // Configure very long press duration threshold (0 = disabled)
    void setVeryLongPressMs(uint32_t ms);

    // Configure whether short click is deferred until double click window expires
    void setDeferSingleClick(bool defer);

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
    uint32_t veryLongPressThresholdMs;
    bool longPressEmitted;
    bool veryLongPressEmitted;

    // Double-click tracking
    uint32_t lastClickTime;
    bool waitingForSecondClick;
    bool deferSingleClick;
};

#endif // INPUT_DECODER_H
