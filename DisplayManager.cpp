#include "DisplayManager.h"
#include <stdio.h>
#include <string.h>

DisplayManager::DisplayManager(Adafruit_ILI9341& tftRef, GameState& stateRef, EventLog& logRef)
    : tft(tftRef), state(stateRef), log(logRef), currentScreen(SCREEN_GAME),
      rollWinner(0), animStartTime(0), lastAnimTick(0), animCurrentHighlight(0),
      animDelay(50), animSettled(false) {
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        lastRenderedDead[i] = false;
    }
}

void DisplayManager::begin() {
    tft.begin();
    tft.setRotation(1); // Landscape 320 x 240
    returnToGameScreen();
}

void DisplayManager::returnToGameScreen() {
    currentScreen = SCREEN_GAME;
    tft.fillScreen(COLOR_BLACK);
    drawStaticLayout();
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        lastRenderedDead[i] = state.getPlayer(i).isDead;
        renderQuadrant(i, true);
    }
}

void DisplayManager::drawStaticLayout() {
    // Center divider lines
    tft.drawFastVLine(QUAD_WIDTH - 1, 0, SCREEN_HEIGHT, COLOR_DIVIDER);
    tft.drawFastVLine(QUAD_WIDTH, 0, SCREEN_HEIGHT, COLOR_DIVIDER);

    tft.drawFastHLine(0, QUAD_HEIGHT - 1, SCREEN_WIDTH, COLOR_DIVIDER);
    tft.drawFastHLine(0, QUAD_HEIGHT, SCREEN_WIDTH, COLOR_DIVIDER);
}

void DisplayManager::getQuadrantBounds(uint8_t playerIdx, int16_t& qx, int16_t& qy, int16_t& qw, int16_t& qh) {
    qw = QUAD_WIDTH;
    qh = QUAD_HEIGHT;
    switch (playerIdx) {
        case 0: qx = 0;          qy = 0;           break; // P1: Top-Left
        case 1: qx = QUAD_WIDTH; qy = 0;           break; // P2: Top-Right
        case 2: qx = 0;          qy = QUAD_HEIGHT; break; // P3: Bottom-Left
        case 3: qx = QUAD_WIDTH; qy = QUAD_HEIGHT; break; // P4: Bottom-Right
        default: qx = 0; qy = 0; break;
    }
}

uint16_t DisplayManager::getPlayerColor(uint8_t playerIdx) {
    switch (playerIdx) {
        case 0: return COLOR_P1;
        case 1: return COLOR_P2;
        case 2: return COLOR_P3;
        case 3: return COLOR_P4;
        default: return COLOR_WHITE;
    }
}

uint16_t DisplayManager::getQuadrantBg(uint8_t playerIdx) const {
    if (playerIdx >= NUM_PLAYERS) return COLOR_BLACK;
    return state.getPlayer(playerIdx).isDead ? COLOR_DEAD_BG : COLOR_BLACK;
}

void DisplayManager::renderQuadrant(uint8_t playerIdx, bool forceFullRedraw) {
    if (playerIdx >= NUM_PLAYERS) return;

    int16_t qx, qy, qw, qh;
    getQuadrantBounds(playerIdx, qx, qy, qw, qh);
    const PlayerState& p = state.getPlayer(playerIdx);
    uint16_t pColor = getPlayerColor(playerIdx);
    uint16_t bgColor = getQuadrantBg(playerIdx);

    if (forceFullRedraw) {
        // Clear quadrant interior (leaving 1px border for divider)
        tft.fillRect(qx + 1, qy + 1, qw - 2, qh - 2, bgColor);
    }

    // 1. Header Bar: Player Badge (Left) + Secondary Poison/Tax (Right)
    tft.fillRect(qx + 4, qy + 4, qw - 8, 16, bgColor);

    // Player Badge (filled rounded rectangle)
    tft.fillRoundRect(qx + 6, qy + 4, 28, 14, 3, pColor);
    tft.setTextColor(COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(qx + 13, qy + 7);
    tft.printf("P%d", playerIdx + 1);

    // Secondary Stats in Header (Poison & Tax)
    tft.setTextColor(COLOR_LIGHTGREY, bgColor);
    tft.setTextSize(1);
    tft.setCursor(qx + 45, qy + 7);
    tft.printf("PSN:%d  TAX:%d", p.poison, p.tax);

    // Vertical column divider between Life and Commander Damage
    tft.drawFastVLine(qx + 82, qy + 22, 66, COLOR_DIVIDER);

    // 2. Dual Counter Display (Life on Left, 3 Opponent Commander Damages on Right)
    // Clear left and right counter areas
    tft.fillRect(qx + 4, qy + 22, 76, 66, bgColor);
    tft.fillRect(qx + 84, qy + 22, 72, 66, bgColor);

    // --- LEFT COLUMN: LIFE ---
    bool isLifeActive = (p.activeMode == MODE_LIFE);
    tft.setTextSize(1);
    tft.setTextColor(isLifeActive ? pColor : COLOR_LIGHTGREY, bgColor);
    tft.setCursor(qx + 10, qy + 24);
    tft.printf("%sLIFE", isLifeActive ? "> " : "  ");

    // Big Life Number (Text Size 4, or 3 for 4+ digits)
    char lifeStr[12];
    snprintf(lifeStr, sizeof(lifeStr), "%d", p.life);
    uint8_t lifeLen = strlen(lifeStr);
    uint8_t lifeTextSize = (lifeLen > 3) ? 3 : 4;
    uint8_t lifeCharW = (lifeTextSize == 4) ? 24 : 18;
    int16_t lifeTotalW = lifeLen * lifeCharW;
    int16_t lifeX = qx + 4 + (76 - lifeTotalW) / 2;
    int16_t lifeY = qy + ((lifeTextSize == 4) ? 42 : 46);

    tft.setTextColor(isLifeActive ? COLOR_WHITE : pColor, bgColor);
    tft.setTextSize(lifeTextSize);
    tft.setCursor(lifeX, lifeY);
    tft.print(lifeStr);

    // --- RIGHT COLUMN: 3 OPPONENT COMMANDER DAMAGES ---
    bool isCmdrActive = (p.activeMode == MODE_CMDR);
    tft.setTextSize(1);
    tft.setTextColor(isCmdrActive ? pColor : COLOR_LIGHTGREY, bgColor);
    tft.setCursor(qx + 88, qy + 24);
    tft.print("CMDR DMG");

    uint8_t rowIdx = 0;
    for (uint8_t opp = 0; opp < NUM_PLAYERS; opp++) {
        if (opp == playerIdx) continue;

        int16_t rowY = qy + 36 + (rowIdx * 16);
        bool isSelected = (isCmdrActive && p.cmdrSource == opp);
        uint8_t dmg = p.cmdrDmg[opp];
        uint16_t oppColor = getPlayerColor(opp);

        if (isSelected) {
            // Highlight active selected opponent
            tft.fillRoundRect(qx + 86, rowY - 1, 68, 14, 2, oppColor);
            tft.setTextColor(COLOR_BLACK, oppColor);
        } else {
            uint16_t labelCol = (dmg >= LETHAL_CMDR_DAMAGE) ? COLOR_DEAD_TEXT : oppColor;
            tft.setTextColor(labelCol, bgColor);
        }

        tft.setTextSize(1);
        tft.setCursor(qx + 89, rowY + 2);
        tft.printf("P%d", opp + 1);

        // Value
        if (isSelected) {
            tft.setTextColor(COLOR_BLACK, oppColor);
        } else {
            uint16_t valCol = (dmg >= LETHAL_CMDR_DAMAGE) ? COLOR_DEAD_TEXT : COLOR_WHITE;
            tft.setTextColor(valCol, bgColor);
        }
        tft.setCursor(qx + 120, rowY + 2);
        tft.printf("%2d", dmg);

        if (dmg >= LETHAL_CMDR_DAMAGE && !isSelected) {
            tft.setCursor(qx + 138, rowY + 2);
            tft.print("!");
        }

        rowIdx++;
    }

    // 3. Footer Status Bar / Lethal Alert
    tft.fillRect(qx + 4, qy + 92, qw - 8, 22, bgColor);
    if (p.isDead) {
        tft.fillRoundRect(qx + 6, qy + 92, qw - 12, 20, 3, COLOR_DEAD_TEXT);
        tft.setTextColor(COLOR_WHITE);
        tft.setTextSize(1);
        tft.setCursor(qx + 10, qy + 98);

        // Determine specific cause of death
        int8_t killerOpp = -1;
        for (uint8_t opp = 0; opp < NUM_PLAYERS; opp++) {
            if (opp != playerIdx && p.cmdrDmg[opp] >= LETHAL_CMDR_DAMAGE) {
                killerOpp = opp;
                break;
            }
        }

        if (killerOpp >= 0) {
            tft.printf("*** DEAD: 21 CMDR (P%d) ***", killerOpp + 1);
        } else if (p.life <= 0) {
            tft.print("*** DEAD: 0 LIFE ***");
        } else {
            tft.print("*** DEAD: 10 POIS ***");
        }
    } else {
        tft.setTextSize(1);
        if (p.activeMode == MODE_POISON) {
            tft.setTextColor(COLOR_WHITE, COLOR_DARKGREY);
            tft.setCursor(qx + 8, qy + 98);
            tft.printf(" [ADJUSTING POISON: %d] ", p.poison);
        } else if (p.activeMode == MODE_TAX) {
            tft.setTextColor(COLOR_WHITE, COLOR_DARKGREY);
            tft.setCursor(qx + 8, qy + 98);
            tft.printf(" [ADJUSTING TAX: +%d] ", p.tax);
        } else if (p.activeMode == MODE_CMDR) {
            tft.setTextColor(COLOR_LIGHTGREY, bgColor);
            tft.setCursor(qx + 8, qy + 98);
            tft.printf("Dial: CMDR P%d (+/-)", p.cmdrSource + 1);
        } else {
            tft.setTextColor(COLOR_LIGHTGREY, bgColor);
            tft.setCursor(qx + 12, qy + 98);
            tft.print("Dial: LIFE (+/-)");
        }
    }

    // Mark quadrant as clean
    state.clearDirty(playerIdx);
}

void DisplayManager::updateDirtyQuadrants() {
    if (currentScreen != SCREEN_GAME) return;

    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        bool deadChanged = (state.getPlayer(i).isDead != lastRenderedDead[i]);
        if (state.getPlayer(i).dirty || deadChanged) {
            lastRenderedDead[i] = state.getPlayer(i).isDead;
            renderQuadrant(i, deadChanged);
        }
    }
}

void DisplayManager::toggleEventLogScreen() {
    if (currentScreen == SCREEN_GAME) {
        currentScreen = SCREEN_EVENT_LOG;
        drawEventLog();
    } else {
        returnToGameScreen();
    }
}

ScreenMode DisplayManager::getScreenMode() const {
    return currentScreen;
}

void DisplayManager::drawEventLog() {
    tft.fillScreen(COLOR_BLACK);

    // Title Header
    tft.fillRect(0, 0, SCREEN_WIDTH, 26, COLOR_DARKGREY);
    tft.drawFastHLine(0, 26, SCREEN_WIDTH, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(55, 6);
    tft.print("MATCH EVENT LOG");

    // Print recent events
    uint16_t count = log.getCount();
    int16_t startY = 36;
    uint8_t maxToDisplay = 8;
    uint8_t displayed = 0;

    tft.setTextSize(1);
    if (count == 0) {
        tft.setTextColor(COLOR_LIGHTGREY);
        tft.setCursor(40, 100);
        tft.print("No events recorded yet.");
    } else {
        // Display newest events first
        for (int i = (int)count - 1; i >= 0 && displayed < maxToDisplay; i--, displayed++) {
            GameEvent evt;
            if (log.getEvent(i, evt)) {
                char str[64];
                EventLog::formatEvent(evt, str, sizeof(str));

                uint16_t color = COLOR_WHITE;
                if (evt.player < NUM_PLAYERS) {
                    color = getPlayerColor(evt.player);
                } else {
                    color = COLOR_P4; // Yellow for system
                }
                tft.setTextColor(color);
                tft.setCursor(15, startY + (displayed * 18));
                tft.print(str);
            }
        }
    }

    // Footer Help Prompt
    tft.drawFastHLine(0, 215, SCREEN_WIDTH, COLOR_DARKGREY);
    tft.setTextColor(COLOR_LIGHTGREY);
    tft.setTextSize(1);
    tft.setCursor(50, 222);
    tft.print("[ Click Reset button to return ]");
}

void DisplayManager::startRollAnimation(uint8_t winningPlayer, uint32_t currentMs) {
    currentScreen = SCREEN_ROLL_ANIMATION;
    rollWinner = winningPlayer;
    animStartTime = currentMs;
    lastAnimTick = currentMs;
    animCurrentHighlight = 0;
    animDelay = 40;
    animSettled = false;

    tft.fillScreen(COLOR_BLACK);
    drawStaticLayout();
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        lastRenderedDead[i] = state.getPlayer(i).isDead;
        renderQuadrant(i, true);
    }
}

bool DisplayManager::updateAnimation(uint32_t currentMs) {
    if (currentScreen != SCREEN_ROLL_ANIMATION) return false;

    // Elapsed time since start of animation
    uint32_t elapsed = currentMs - animStartTime;

    if (!animSettled) {
        if (currentMs - lastAnimTick >= animDelay) {
            lastAnimTick = currentMs;

            // Highlight cycle order: P1(0) -> P2(1) -> P4(3) -> P3(2) (clockwise)
            static const uint8_t CYCLE[4] = { 0, 1, 3, 2 };
            uint8_t currentP = CYCLE[animCurrentHighlight];

            // Clear previous highlight borders using each quadrant's actual background color
            for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
                int16_t qx, qy, qw, qh;
                getQuadrantBounds(i, qx, qy, qw, qh);
                uint16_t bg = getQuadrantBg(i);
                tft.drawRect(qx + 2, qy + 2, qw - 4, qh - 4, bg);
                tft.drawRect(qx + 3, qy + 3, qw - 6, qh - 6, bg);
            }

            // Draw bright highlight on current quadrant
            int16_t hx, hy, hw, hh;
            getQuadrantBounds(currentP, hx, hy, hw, hh);
            tft.drawRect(hx + 2, hy + 2, hw - 4, hh - 4, COLOR_WHITE);
            tft.drawRect(hx + 3, hy + 3, hw - 6, hh - 6, getPlayerColor(currentP));

            animCurrentHighlight = (animCurrentHighlight + 1) % 4;

            // Gradually slow down
            if (elapsed > 600)  animDelay = 80;
            if (elapsed > 1100) animDelay = 140;
            if (elapsed > 1500) animDelay = 220;

            // Settle on winner after 2.0s
            if (elapsed > 2000 && currentP == rollWinner) {
                animSettled = true;

                // Center Celebration Banner
                int16_t bx = 30, by = 90, bw = 260, bh = 55;
                tft.fillRoundRect(bx, by, bw, bh, 6, COLOR_DARKGREY);
                tft.drawRoundRect(bx, by, bw, bh, 6, getPlayerColor(rollWinner));
                tft.drawRoundRect(bx + 1, by + 1, bw - 2, bh - 2, 5, COLOR_WHITE);

                tft.setTextColor(COLOR_WHITE);
                tft.setTextSize(2);
                tft.setCursor(bx + 20, by + 12);
                tft.printf("P%d GOES FIRST!", rollWinner + 1);

                tft.setTextSize(1);
                tft.setTextColor(COLOR_LIGHTGREY);
                tft.setCursor(bx + 45, by + 36);
                tft.print("Starting match...");
            }
        }
    } else {
        // Hold celebration banner for 2 seconds then restore normal screen
        if (elapsed > 4000) {
            returnToGameScreen();
            return false;
        }
    }
    return true;
}
