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

    // 6. Button Debounce: Double Click Detection (< 350ms window)
    // First click was released at t=490. Second press begins at t=600 (110ms later)
    btn.update(true, 600);
    btn.update(true, 640);
    btn.update(false, 700);
    evt = btn.update(false, 740); // Released at t=740 (within 350ms of t=490)
    TEST_ASSERT(evt == BTN_EVENT_DOUBLE_CLICK);
    printf("  [PASS] Rapid second click emitted BTN_EVENT_DOUBLE_CLICK\n");

    // 7. Button Debounce: Long Press (> 1500ms default)
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

    // 8. Custom 3000ms Player Reset Hold Threshold
    btn.reset();
    btn.setLongPressMs(PLAYER_RESET_LONG_PRESS_MS); // 3000ms
    btn.update(true, 3000);
    btn.update(true, 3050);
    evt = btn.update(true, 5000); // Held 2000ms: should NOT trigger at 1500ms
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    evt = btn.update(true, 6050); // Held 3050ms >= 3000ms
    TEST_ASSERT(evt == BTN_EVENT_LONG_PRESS);
    printf("  [PASS] Custom 3000ms player reset long press threshold verified\n");

    // 9. Deferred Single Click (Double-Click window timeout)
    btn.reset();
    btn.setDeferSingleClick(true);
    btn.update(true, 7000);
    btn.update(true, 7050);
    btn.update(false, 7150);
    evt = btn.update(false, 7190); // Released at t=7190
    TEST_ASSERT(evt == BTN_EVENT_NONE); // Single click deferred!
    evt = btn.update(false, 7400); // 210ms elapsed: still in window
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    evt = btn.update(false, 7550); // 360ms elapsed: window expired!
    TEST_ASSERT(evt == BTN_EVENT_SHORT_CLICK);
    printf("  [PASS] Deferred single click emits only after double-click window expires\n");

    // 10. Deferred Double Click suppresses Short Click
    btn.reset();
    btn.setDeferSingleClick(true);
    btn.update(true, 8000);
    btn.update(true, 8040);
    btn.update(false, 8100);
    evt = btn.update(false, 8140);
    TEST_ASSERT(evt == BTN_EVENT_NONE); // Deferred
    // Second click within window
    btn.update(true, 8250);
    btn.update(true, 8290);
    btn.update(false, 8350);
    evt = btn.update(false, 8390);
    TEST_ASSERT(evt == BTN_EVENT_DOUBLE_CLICK); // Immediate double click!
    // Verify no stray short click follows
    evt = btn.update(false, 8800);
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    printf("  [PASS] Double click cleanly fires without any preceding short click\n");

    // 11. Multi-tier Hold: 3000ms Very Long Press does not fire Long Press during hold
    btn.reset();
    btn.setLongPressMs(1500);
    btn.setVeryLongPressMs(3000);
    btn.update(true, 9000);
    btn.update(true, 9050);
    evt = btn.update(true, 10700); // 1700ms held (> 1500ms): suppressed to allow 3000ms
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    evt = btn.update(true, 12050); // 3050ms held (>= 3000ms): fires VERY_LONG_PRESS!
    TEST_ASSERT(evt == BTN_EVENT_VERY_LONG_PRESS);
    // Release after very long press emits nothing
    btn.update(false, 12500);
    evt = btn.update(false, 12550);
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    printf("  [PASS] 3000ms hold fires VERY_LONG_PRESS without firing 1500ms LONG_PRESS\n");

    // 12. Multi-tier Hold: Release between 1500ms and 3000ms fires Long Press on release
    btn.reset();
    btn.setLongPressMs(1500);
    btn.setVeryLongPressMs(3000);
    btn.update(true, 13000);
    btn.update(true, 13050);
    evt = btn.update(true, 14800); // 1800ms held: nothing while holding
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    btn.update(false, 14900); // Released at 1900ms (< 3000ms, >= 1500ms)
    evt = btn.update(false, 14950);
    TEST_ASSERT(evt == BTN_EVENT_LONG_PRESS); // Fires on release!
    printf("  [PASS] Releasing after 1500ms but before 3000ms fires LONG_PRESS on release\n");

    // 13. Reset preserves custom configured thresholds (L2 fix)
    btn.setLongPressMs(2500);
    btn.setVeryLongPressMs(5000);
    btn.setDeferSingleClick(true);
    btn.reset();
    btn.update(true, 16000);
    btn.update(true, 16050);
    evt = btn.update(true, 18000); // Held 2000ms (< 2500ms): should NOT fire default 1500ms
    TEST_ASSERT(evt == BTN_EVENT_NONE);
    printf("  [PASS] reset() preserves custom configured timing thresholds\n");

    // 14. Rotary Step Accumulation Buffer (H2 fix)
    rot.reset();
    TEST_ASSERT(rot.getPendingSteps() == 0);
    TEST_ASSERT(rot.drainSteps() == 0);

    // Simulate 3 full CW detents
    for (int d = 0; d < 3; d++) {
        rot.update(false, true);
        rot.update(false, false);
        rot.update(true, false);
        rot.update(true, true);
    }
    TEST_ASSERT(rot.getPendingSteps() == 3);
    TEST_ASSERT(rot.drainSteps() == 3);
    TEST_ASSERT(rot.getPendingSteps() == 0); // Drained!

    // Direct manual step injection (e.g. from ISR / hardware timer)
    rot.addStep(-5);
    TEST_ASSERT(rot.drainSteps() == -5);
    TEST_ASSERT(rot.drainSteps() == 0);
    printf("  [PASS] Rotary decoder step accumulation buffer and atomic draining verified\n");

    // 15. Hold Status & Duration Query (Long Press Countdown Support)
    btn.reset();
    TEST_ASSERT(btn.isPressed() == false);
    TEST_ASSERT(btn.getPressDuration(20000) == 0);

    // Initial press (within debounce delay < 35ms)
    btn.update(true, 20000);
    btn.update(true, 20020);
    TEST_ASSERT(btn.isPressed() == false); // Not debounced yet

    // Debounce settled (>= 35ms)
    btn.update(true, 20040);
    TEST_ASSERT(btn.isPressed() == true);
    TEST_ASSERT(btn.getPressDuration(20040) == 0);
    TEST_ASSERT(btn.getPressDuration(20540) == 500);
    TEST_ASSERT(btn.getPressDuration(21240) == 1200);

    // Button released
    btn.update(false, 21250);
    btn.update(false, 21300); // Debounce settled release
    TEST_ASSERT(btn.isPressed() == false);
    TEST_ASSERT(btn.getPressDuration(21300) == 0);
    printf("  [PASS] ButtonDebouncer isPressed() and getPressDuration() verified\n");

    printf("\n>>> test_inputdecoder: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}
