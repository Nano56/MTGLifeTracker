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

    // 1. Startup Self-Test Execution & Completion
    display.begin();
#if ENABLE_STARTUP_SELF_TEST
    TEST_ASSERT(display.isSelfTestRunning());
    TEST_ASSERT(display.getScreenMode() == SCREEN_SELF_TEST);
    TEST_ASSERT(display.getSelfTestStep() == TEST_STEP_SPLASH);
    printf("  [PASS] DisplayManager entered Startup Self-Test sequence\n");

    // Advance self-test step-by-step through all visual demonstrations
    uint32_t simTime = 0;
    while (display.isSelfTestRunning() && simTime < 30000) {
        simTime += 200;
        display.updateSelfTest(simTime);
    }
    TEST_ASSERT(!display.isSelfTestRunning());
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    TEST_ASSERT(state.getPlayerCount() == DEFAULT_PLAYER_COUNT);
    TEST_ASSERT(state.getPlayer(0).life == STARTING_LIFE);
    printf("  [PASS] Startup Self-Test completed all steps and restored clean game screen\n");

    // Test instant skip functionality
    display.startSelfTest(simTime);
    TEST_ASSERT(display.isSelfTestRunning());
    display.skipSelfTest(simTime + 50);
    TEST_ASSERT(!display.isSelfTestRunning());
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    printf("  [PASS] Startup Self-Test skip abort confirmed\n");
#else
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    printf("  [PASS] DisplayManager initialized in SCREEN_GAME mode\n");
#endif

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

    simTime = 3050;
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

    // 7. Player Select Screen (2, 3, 4 Players)
    display.openPlayerSelectScreen();
    TEST_ASSERT(display.getScreenMode() == SCREEN_PLAYER_SELECT);
    TEST_ASSERT(display.getSelectedPlayerCount() == 4);

    display.adjustPlayerSelect(1); // 4 -> wraps to 2
    TEST_ASSERT(display.getSelectedPlayerCount() == 2);

    display.confirmPlayerSelect(11000);
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    TEST_ASSERT(state.getPlayerCount() == 2);
    printf("  [PASS] Player select menu set game to 2 players\n");

    // 8. Render 2-Player Dynamic Halves
    for (uint8_t i = 0; i < 2; i++) {
        display.renderQuadrant(i, true);
        TEST_ASSERT(!state.getPlayer(i).dirty);
    }
    printf("  [PASS] 2-Player dynamic halves rendered\n");

    // 9. Render 3-Player Dynamic Halves & Quads
    display.openPlayerSelectScreen();
    display.adjustPlayerSelect(1); // 2 -> 3
    TEST_ASSERT(display.getSelectedPlayerCount() == 3);
    display.confirmPlayerSelect(12000);
    TEST_ASSERT(state.getPlayerCount() == 3);
    for (uint8_t i = 0; i < 3; i++) {
        display.renderQuadrant(i, true);
        TEST_ASSERT(!state.getPlayer(i).dirty);
    }
    printf("  [PASS] 3-Player layout rendered\n");

    // 10. Table Button UX Flow Simulation
    // Modify P1 life to 27
    state.applyDelta(0, -13, 13000);
    TEST_ASSERT(state.getPlayer(0).life == 27);

    // Double-click red button enters event log
    display.toggleEventLogScreen();
    TEST_ASSERT(display.getScreenMode() == SCREEN_EVENT_LOG);
    // Any button click while on event log screen returns to game screen
    display.returnToGameScreen();
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    TEST_ASSERT(state.getPlayer(0).life == 27); // Life preserved!

    // 3.0s hold enters player select menu without resetting player life
    display.openPlayerSelectScreen();
    TEST_ASSERT(display.getScreenMode() == SCREEN_PLAYER_SELECT);
    TEST_ASSERT(state.getPlayer(0).life == 27); // Life NOT reset upon entering menu!
    display.returnToGameScreen(); // Cancel / back to game
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    TEST_ASSERT(state.getPlayer(0).life == 27);
    printf("  [PASS] Table button event log toggle and non-destructive menu entry verified\n");

    // 11. Toast Banner Lifecycle
    display.showToast("MATCH RESTORED", 20000);
    TEST_ASSERT(display.isToastActive() == true);
    display.updateToast(21000); // 1000ms < 1500ms
    TEST_ASSERT(display.isToastActive() == true);
    display.updateToast(21500); // 1500ms >= 1500ms -> expired!
    TEST_ASSERT(display.isToastActive() == false);
    printf("  [PASS] Toast banner lifecycle and 1500ms duration verified\n");

    // 12. Event Log Scrolling (L11 fix)
    for (int e = 0; e < 20; e++) {
        log.log(25000 + e * 100, 0, EVT_LIFE_CHANGE, 1, 28 + e);
    }
    TEST_ASSERT(log.getCount() >= 20);

    display.toggleEventLogScreen();
    TEST_ASSERT(display.getScreenMode() == SCREEN_EVENT_LOG);
    TEST_ASSERT(display.getLogScrollOffset() == 0); // Starts at newest events

    // Scroll older by 4 events
    display.scrollEventLog(4);
    TEST_ASSERT(display.getLogScrollOffset() == 4);

    // Scroll past maximum offset (total - 8)
    uint16_t totalEvents = log.getCount();
    uint16_t maxExpectedOffset = totalEvents - 8;
    display.scrollEventLog(100);
    TEST_ASSERT(display.getLogScrollOffset() == maxExpectedOffset);

    // Scroll back past newest (0)
    display.scrollEventLog(-100);
    TEST_ASSERT(display.getLogScrollOffset() == 0);

    display.returnToGameScreen();
    TEST_ASSERT(display.getScreenMode() == SCREEN_GAME);
    printf("  [PASS] Event log scrolling bounds and window offset verified\n");

    // 13. Transient Delta Accumulator Lifecycle
    display.recordDelta(0, -3, 30000);
    TEST_ASSERT(display.getRecentDelta(0) == -3);
    TEST_ASSERT(display.getRecentDelta(1) == 0);

    // Rapid successive turns accumulate
    display.recordDelta(0, -2, 30500);
    TEST_ASSERT(display.getRecentDelta(0) == -5);

    // Life gain in same window partially cancels/accumulates
    display.recordDelta(0, 1, 30800);
    TEST_ASSERT(display.getRecentDelta(0) == -4);

    // Before timeout (<1500ms from last adjustment at 30800)
    display.updateTransientDeltas(32000); // 1200ms elapsed
    TEST_ASSERT(display.getRecentDelta(0) == -4);

    // After timeout (>= 1500ms elapsed)
    display.updateTransientDeltas(32301); // 1501ms elapsed
    TEST_ASSERT(display.getRecentDelta(0) == 0);

    // Positive delta and explicit clearDelta
    display.recordDelta(1, 4, 33000);
    TEST_ASSERT(display.getRecentDelta(1) == 4);
    display.clearDelta(1);
    TEST_ASSERT(display.getRecentDelta(1) == 0);

    // Boundary conditions
    display.recordDelta(NUM_PLAYERS, 5, 34000);
    TEST_ASSERT(display.getRecentDelta(NUM_PLAYERS) == 0);
    display.clearDelta(NUM_PLAYERS);
    printf("  [PASS] Transient delta accumulator recording, accumulation, timeout and clear verified\n");

    // 14. Reset Hold Countdown & Confirmation Toasts
    display.showTableResetProgress(600, 1500, 3000, 35000);
    TEST_ASSERT(display.isTableResetProgressActive() == true);

    // Cancel early (release before threshold)
    display.clearTableResetProgress();
    TEST_ASSERT(display.isTableResetProgressActive() == false);

    // Table reset confirmation toast
    display.showToast("GAME RESET: 40 LIFE", 35500);
    TEST_ASSERT(display.isToastActive() == true);
    display.updateToast(37000); // 1500ms -> expired
    TEST_ASSERT(display.isToastActive() == false);

    // Player reset progress hold
    display.showPlayerResetProgress(0, 1200, 3000, 37500);
    TEST_ASSERT(display.isPlayerResetProgressActive(0) == true);
    TEST_ASSERT(display.isPlayerResetProgressActive(1) == false);

    // Clear player reset progress
    display.clearPlayerResetProgress(0);
    TEST_ASSERT(display.isPlayerResetProgressActive(0) == false);

    // Player reset confirmation toast
    display.showToast("P1 RESET: 40 LIFE", 38000);
    TEST_ASSERT(display.isToastActive() == true);
    display.updateToast(39500); // 1500ms -> expired
    TEST_ASSERT(display.isToastActive() == false);
    printf("  [PASS] Table and player reset countdown progress and confirmation toasts verified\n");

    printf(">>> test_display: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}

