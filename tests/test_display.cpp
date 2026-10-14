#include <stdio.h>
#include <assert.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "../Config.h"
#include "../GameState.h"
#include "../EventLog.h"
#include "../DisplayManager.h"

static int g_assertCount = 0;
#define TEST_ASSERT(cond) do { \
    g_assertCount++; \
    if (!(cond)) { \
        printf("FAILED ASSERTION: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

int main() {
    printf("--- Running test_display (Native DisplayManager Validation) ---\n");

    Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
    EventLog log;
    GameState state(log);
    DisplayManager display(tft, state, log);

    // 1. Hardware Initialization & Static Layout
    display.begin();
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    printf("  [PASS] DisplayManager initialized in SCREEN_GAME mode\n");

    // 2. Render Quadrants
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        display.renderQuadrant(i, true);
        TEST_ASSERT(!state.getPlayer(i).dirty);
    }
    printf("  [PASS] All 4 quadrants rendered and dirty flags cleared\n");

    // 3. Mode Cycling & Dirty Quadrant Updates
    state.cycleMode(0, 1000); // P1 to CMDR mode
    TEST_ASSERT(state.getPlayer(0).activeMode == MODE_CMDR);
    TEST_ASSERT(state.getPlayer(0).dirty);
    display.updateDirtyQuadrants();
    TEST_ASSERT(!state.getPlayer(0).dirty);
    printf("  [PASS] Dirty quadrant update for CMDR mode rendered cleanly\n");

    // 4. Lethal Death Display Transition
    // Deal 21 commander damage from P2 to P1
    state.applyDelta(0, 21, 2000);
    TEST_ASSERT(state.getPlayer(0).isDead);
    display.updateDirtyQuadrants();
    TEST_ASSERT(!state.getPlayer(0).dirty);
    printf("  [PASS] Lethal quadrant death transition rendered\n");

    // 5. Roll Animation Lifecycle (Stepped frame update)
    display.startRollAnimation(2, 3000);
    TEST_ASSERT(display.getScreenMode() == SCREEN_ROLL_ANIMATION);

    uint32_t simTime = 3050;
    bool animRunning = true;
    while (animRunning && simTime < 10000) {
        animRunning = display.updateAnimation(simTime);
        simTime += 40;
    }
    TEST_ASSERT(!animRunning);
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    printf("  [PASS] Roll animation executed frame-by-frame, settled, and restored game screen\n");

    // 6. Event Log Screen Toggle
    display.toggleEventLogScreen();
    TEST_ASSERT(display.getScreenMode() == SCREEN_EVENT_LOG);
    display.toggleEventLogScreen();
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    printf("  [PASS] Event log screen toggle verified\n");

    printf(">>> test_display: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}

