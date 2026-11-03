#include "InputDecoder.h"

// 16-element transition table for 2-bit Gray code
// Index: (old_CLK_DT << 2) | new_CLK_DT
static const int8_t QUAD_TABLE[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

RotaryDecoder::RotaryDecoder() {
    reset();
}

void RotaryDecoder::reset() {
    state = 0x03; // Default pulled high (11)
    subCount = 0;
    accumulatedSteps = 0;
}

void RotaryDecoder::addStep(int8_t step) {
    accumulatedSteps += step;
}

int16_t RotaryDecoder::drainSteps() {
    int16_t steps = accumulatedSteps;
    accumulatedSteps = 0;
    return steps;
}

int16_t RotaryDecoder::getPendingSteps() const {
    return accumulatedSteps;
}

int8_t RotaryDecoder::update(bool clkLevel, bool dtLevel) {
    uint8_t currentCode = ((clkLevel ? 1 : 0) << 1) | (dtLevel ? 1 : 0);
    uint8_t transition = ((state & 0x03) << 2) | (currentCode & 0x03);
    state = currentCode;

    int8_t delta = QUAD_TABLE[transition];
    if (delta != 0) {
        subCount += delta;

        // Standard 4-step quadrature detent
        if (subCount >= 4) {
            subCount = 0;
            accumulatedSteps += 1;
            return 1;
        } else if (subCount <= -4) {
            subCount = 0;
            accumulatedSteps -= 1;
            return -1;
        }
        
        // Reached idle detent (1, 1) with partial accumulation
        if (currentCode == 0x03) {
            if (subCount >= 2) {
                subCount = 0;
                accumulatedSteps += 1;
                return 1;
            } else if (subCount <= -2) {
                subCount = 0;
                accumulatedSteps -= 1;
                return -1;
            }
            subCount = 0;
        }
    }
    return 0;
}

// -------------------------------------------------------------
// ButtonDebouncer Implementation
// -------------------------------------------------------------

ButtonDebouncer::ButtonDebouncer() {
    longPressThresholdMs = LONG_PRESS_MS;
    veryLongPressThresholdMs = 0;
    deferSingleClick = false;
    reset();
}

void ButtonDebouncer::setLongPressMs(uint32_t ms) {
    longPressThresholdMs = ms;
}

void ButtonDebouncer::setVeryLongPressMs(uint32_t ms) {
    veryLongPressThresholdMs = ms;
}

void ButtonDebouncer::setDeferSingleClick(bool defer) {
    deferSingleClick = defer;
}

void ButtonDebouncer::reset() {
    state = STATE_RELEASED;
    lastRawLevel = false;
    lastDebounceTime = 0;
    pressStartTime = 0;
    longPressEmitted = false;
    veryLongPressEmitted = false;
    lastClickTime = 0;
    waitingForSecondClick = false;
}

ButtonEvent ButtonDebouncer::update(bool isPressed, uint32_t currentMs) {
    // Check if double-click window has expired for deferred single click
    if (waitingForSecondClick && (currentMs - lastClickTime > DOUBLE_CLICK_WINDOW_MS)) {
        waitingForSecondClick = false;
        if (deferSingleClick && state == STATE_RELEASED) {
            return BTN_EVENT_SHORT_CLICK;
        }
    }

    // If raw pin level changed, reset debounce timer
    if (isPressed != lastRawLevel) {
        lastDebounceTime = currentMs;
        lastRawLevel = isPressed;
    }

    // Wait until state has stabilized beyond debounce window
    if ((currentMs - lastDebounceTime) >= DEBOUNCE_DELAY_MS) {
        if (isPressed && state == STATE_RELEASED) {
            state = STATE_PRESSED;
            pressStartTime = currentMs;
            longPressEmitted = false;
            veryLongPressEmitted = false;
        } else if (!isPressed && state == STATE_PRESSED) {
            state = STATE_RELEASED;
            uint32_t pressDuration = currentMs - pressStartTime;

            if (veryLongPressEmitted) {
                // Already emitted VERY_LONG_PRESS while held
                return BTN_EVENT_NONE;
            }

            // If a very long press threshold is configured, long press emits on release
            // to avoid pre-empting the very long press while still holding
            if (veryLongPressThresholdMs > 0 && pressDuration >= longPressThresholdMs) {
                waitingForSecondClick = false;
                return BTN_EVENT_LONG_PRESS;
            }

            if (!longPressEmitted) {
                if (waitingForSecondClick && (currentMs - lastClickTime <= DOUBLE_CLICK_WINDOW_MS)) {
                    waitingForSecondClick = false;
                    return BTN_EVENT_DOUBLE_CLICK;
                } else {
                    waitingForSecondClick = true;
                    lastClickTime = currentMs;
                    if (!deferSingleClick) {
                        return BTN_EVENT_SHORT_CLICK;
                    }
                }
            }
        }
    }

    // Check long press while still held down
    if (state == STATE_PRESSED) {
        uint32_t pressDuration = currentMs - pressStartTime;

        // If very long press is configured, check it first while held
        if (veryLongPressThresholdMs > 0) {
            if (!veryLongPressEmitted && pressDuration >= veryLongPressThresholdMs) {
                veryLongPressEmitted = true;
                longPressEmitted = true;
                waitingForSecondClick = false;
                return BTN_EVENT_VERY_LONG_PRESS;
            }
        } else {
            // Standard button without very long press threshold: emits on hold
            if (!longPressEmitted && pressDuration >= longPressThresholdMs) {
                longPressEmitted = true;
                waitingForSecondClick = false;
                return BTN_EVENT_LONG_PRESS;
            }
        }
    }

    return BTN_EVENT_NONE;
}

bool ButtonDebouncer::isPressed() const {
    return (state == STATE_PRESSED);
}

uint32_t ButtonDebouncer::getPressDuration(uint32_t currentMs) const {
    if (state == STATE_PRESSED && currentMs >= pressStartTime) {
        return currentMs - pressStartTime;
    }
    return 0;
}

