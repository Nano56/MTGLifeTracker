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
    SCREEN_ROLL_ANIMATION
};

class DisplayManager {
public:
    DisplayManager(Adafruit_ILI9341& tftRef, GameState& stateRef, EventLog& logRef);

    // Initial hardware setup & static frame draw
    void begin();

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

    ScreenMode currentScreen;
    bool lastRenderedDead[NUM_PLAYERS];

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
};

#endif // DISPLAY_MANAGER_H
