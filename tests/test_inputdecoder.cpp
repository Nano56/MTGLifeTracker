#include <stdio.h>
#include <assert.h>
#include "../InputDecoder.h"

static int g_assertCount = 0;
#define TEST_ASSERT(cond) do { \
    g_assertCount++; \
    if (!(cond)) { \
        printf("FAILED ASSERTION: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

int main() {
    printf("--- Running test_inputdecoder ---\n");

    // 1. Rotary Encoder Clockwise Detent
    RotaryDecoder rot;
    // Detent rest: (1, 1)
    int8_t step = 0;
    step += rot.update(false, true);  // (0, 1)
    step += rot.update(false, false); // (0, 0)
    step += rot.update(true, false);  // (1, 0)
    step += rot.update(true, true);   // (1, 1) - Detent complete!
    TEST_ASSERT(step == 1);
    printf("  [PASS] Full Clockwise detent emits +1\n");

    // 2. Rotary Encoder Counter-Clockwise Detent
    step = 0;
    step += rot.update(true, false);  // (1, 0)
    step += rot.update(false, false); // (0, 0)
    step += rot.update(false, true);  // (0, 1)
    step += rot.update(true, true);   // (1, 1) - Detent complete!
    TEST_ASSERT(step == -1);
    printf("  [PASS] Full Counter-Clockwise detent emits -1\n");

    // 3. Contact Bounce / Incomplete Jitter Rejection
    step = 0;
    step += rot.update(false, true);  // (0, 1)
    step += rot.update(true, true);   // bounces right back to (1, 1)
    TEST_ASSERT(step == 0);
    printf("  [PASS] Jitter / contact bounce rejected without false ticks\n");

    // 4. Button Debounce: Noise rejection (< 35ms)
    ButtonDebouncer btn;
    ButtonEvent evt = btn.update(true, 100);  // Press begins at t=100
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    evt = btn.update(false, 115);            // Noise glitched back high at t=115 (15ms < 35ms)
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    printf("  [PASS] Button contact bounce (<35ms) rejected\n");

    // 5. Button Debounce: Clean Short Click
    btn.reset();
    evt = btn.update(true, 200);   // Press begins at t=200
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    evt = btn.update(true, 240);   // Debounced stable press
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    evt = btn.update(false, 450);  // Released at t=450 (held 250ms)
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    evt = btn.update(false, 490);  // Debounced stable release
    TEST_ASSERT(evt == BTN_EVENT_SHORT_CLICK);
    printf("  [PASS] Clean short click (250ms hold) detected on release\n");

    // 6. Button Debounce: Long Press (> 1500ms)
    btn.reset();
    btn.update(true, 1000); // Press begins at t=1000
    btn.update(true, 1050); // Debounce passed
    evt = btn.update(true, 2400); // At t=2400 (held 1400ms < 1500ms)
    TEST_ASSERT(evt == BTN_EVENT_NONE);

    evt = btn.update(true, 2550); // At t=2550 (held 1550ms >= 1500ms)
    TEST_ASSERT(evt == BTN_EVENT_LONG_PRESS); // Immediate trigger!

    // Ensure release after long press does NOT trigger a second click
    evt = btn.update(false, 2800);
    evt = btn.update(false, 2850);
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    printf("  [PASS] Long press (1500ms+) triggers immediately and suppresses release click\n");

    printf(">>> test_inputdecoder: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}

