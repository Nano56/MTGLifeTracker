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
            return 1;
        } else if (subCount <= -4) {
            subCount = 0;
            return -1;
        }
        
        // Reached idle detent (1, 1) with partial accumulation
        if (currentCode == 0x03) {
            if (subCount >= 2) {
                subCount = 0;
                return 1;
            } else if (subCount <= -2) {
                subCount = 0;
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
    reset();
}

void ButtonDebouncer::reset() {
    state = STATE_RELEASED;
    lastRawLevel = false;
    lastDebounceTime = 0;
    pressStartTime = 0;
    longPressEmitted = false;
    lastClickTime = 0;
    waitingForSecondClick = false;
}

ButtonEvent ButtonDebouncer::update(bool isPressed, uint32_t currentMs) {
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
        } else if (!isPressed && state == STATE_PRESSED) {
            state = STATE_RELEASED;
            if (!longPressEmitted) {
                // Short click detected
                return BTN_EVENT_SHORT_CLICK;
            }
        }
    }

    // Check long press while still held down
    if (state == STATE_PRESSED && !longPressEmitted) {
        if ((currentMs - pressStartTime) >= LONG_PRESS_MS) {
            longPressEmitted = true;
            return BTN_EVENT_LONG_PRESS;
        }
    }

    return BTN_EVENT_NONE;
}

