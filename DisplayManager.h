#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "Config.h"
#include "GameState.h"
#include "EventLog.h"

enum ScreenMode {
    SCREEN_GAME,
    SCREEN_EVENT_LOG,
    SCREEN_ROLL_ANIMATION,
    SCREEN_PLAYER_SELECT,
    SCREEN_SELF_TEST
};

enum SelfTestStep {
    TEST_STEP_NONE = 0,
    TEST_STEP_SPLASH,          // 1. Splash screen & hardware color palette check
    TEST_STEP_4P_LAYOUT,       // 2. 4-Player layout & 180° tabletop mirroring check
    TEST_STEP_COUNTERS,        // 3. Real-time counter animations (Life, CMDR, Poison, Tax)
    TEST_STEP_LETHAL,          // 4. Lethal condition alerts (Poison & CMDR deaths)
    TEST_STEP_EVENT_LOG,       // 5. Match Event Log screen inspection
    TEST_STEP_3P_LAYOUT,       // 6. 3-Player layout (1 top wide, 2 bottom quads)
    TEST_STEP_2P_LAYOUT,       // 7. 2-Player layout (two dynamic wide halves)
    TEST_STEP_PLAYER_SELECT,   // 8. Player Count Setup menu demonstration
    TEST_STEP_ROLL_ANIMATION,  // 9. First-Player Roll roulette animation & banner
    TEST_STEP_DONE             // 10. Self-test complete, ready to play
};

class DisplayManager {
public:
    DisplayManager(Adafruit_ILI9341& tftRef, GameState& stateRef, EventLog& logRef);

    // Initial hardware setup & static frame draw
    void begin(bool restoreMatch = false, uint32_t currentMs = 0);

    // Visual Toast Banner (e.g. "MATCH RESTORED")
    void showToast(const char* message, uint32_t currentMs = 0);
    void updateToast(uint32_t currentMs);
    bool isToastActive() const;

    // Redraw static layout dividers and headers
    void drawStaticLayout();

    // Redraw a specific player's quadrant if marked dirty
    void renderQuadrant(uint8_t playerIdx, bool forceFullRedraw = false);

    // Render all quadrants that have dirty flag set
    void updateDirtyQuadrants();

    // Start the first player random roll animation
    void startRollAnimation(uint8_t winningPlayer, uint32_t currentMs);

    // Update animation frames (non-blocking)
    bool updateAnimation(uint32_t currentMs);

    // Toggle between game screen and event log screen
    void toggleEventLogScreen();
    void scrollEventLog(int8_t step);
    uint16_t getLogScrollOffset() const;

    // Transient Delta Accumulator (combat damage / life gain popup)
    void recordDelta(uint8_t playerIdx, int16_t delta, uint32_t currentMs);
    void clearDelta(uint8_t playerIdx);
    void updateTransientDeltas(uint32_t currentMs);
    int16_t getRecentDelta(uint8_t playerIdx) const;

    // Player Count Setup Menu
    void openPlayerSelectScreen();
    void adjustPlayerSelect(int8_t step);
    void confirmPlayerSelect(uint32_t currentMs = 0);
    uint8_t getSelectedPlayerCount() const;

    // Startup Hardware Self-Test / Demo Sequence
    void startSelfTest(uint32_t currentMs = 0);
    bool updateSelfTest(uint32_t currentMs);
    void skipSelfTest(uint32_t currentMs = 0);
    bool isSelfTestRunning() const;
    SelfTestStep getSelfTestStep() const;

    // Check current screen mode
    ScreenMode getScreenMode() const;

    // Full screen reset / redraw after log or reset
    void returnToGameScreen();

    // Get background color for a player quadrant
    uint16_t getQuadrantBg(uint8_t playerIdx) const;

private:
    Adafruit_ILI9341& tft;
    GameState& state;
    EventLog& log;
    GFXcanvas16 quadCanvas;

    ScreenMode currentScreen;
    uint8_t menuPlayerCount;
    uint16_t logScrollOffset;
    bool lastRenderedDead[NUM_PLAYERS];

    // Transient Delta Accumulator state
    int16_t recentDelta[NUM_PLAYERS];
    uint32_t lastDeltaTimeMs[NUM_PLAYERS];

    // Toast Banner state
    bool toastActive;
    uint32_t toastStartTime;
    char toastText[32];

    // Startup Self-Test state
    SelfTestStep selfTestStep;
    uint32_t selfTestStepStartTime;
    void runSelfTestStep(SelfTestStep step, uint32_t currentMs);
    void drawSelfTestSplash();

    // Roll animation state
    uint8_t rollWinner;
    uint32_t animStartTime;
    uint32_t lastAnimTick;
    uint8_t animCurrentHighlight;
    uint16_t animDelay;
    bool animSettled;

    // Helper functions
    void getQuadrantBounds(uint8_t playerIdx, int16_t& qx, int16_t& qy, int16_t& qw, int16_t& qh);
    uint16_t getPlayerColor(uint8_t playerIdx);
    void drawEventLog();
    void drawPlayerSelectMenu();
    void renderHalf(uint8_t playerIdx);
};

#endif // DISPLAY_MANAGER_H
