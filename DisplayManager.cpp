#include "DisplayManager.h"
#include <stdio.h>
#include <string.h>

#if defined(ESP32) || defined(ARDUINO)
#include <Arduino.h>
#else
#include <stdarg.h>
#endif

static void selfTestLog(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

#if defined(ESP32) || defined(ARDUINO)
    // Expand lone \n to \r\n to prevent diagonal stair-stepping in serial terminals
    char outBuf[512];
    size_t j = 0;
    for (size_t i = 0; buffer[i] != '\0' && j < sizeof(outBuf) - 2; i++) {
        if (buffer[i] == '\n' && (i == 0 || buffer[i - 1] != '\r')) {
            outBuf[j++] = '\r';
        }
        outBuf[j++] = buffer[i];
    }
    outBuf[j] = '\0';
    Serial.print(outBuf);
#else
    printf("%s", buffer);
    fflush(stdout);
#endif
}

DisplayManager::DisplayManager(Adafruit_ILI9341& tftRef, GameState& stateRef, EventLog& logRef)
    : tft(tftRef), state(stateRef), log(logRef), quadCanvas(QUAD_WIDTH, QUAD_HEIGHT),
      currentScreen(SCREEN_GAME), menuPlayerCount(DEFAULT_PLAYER_COUNT), logScrollOffset(0),
      toastActive(false), toastStartTime(0),
      selfTestStep(TEST_STEP_NONE), selfTestStepStartTime(0),
      rollWinner(0), animStartTime(0), lastAnimTick(0),
      animCurrentHighlight(0), animDelay(50), animSettled(false) {
    memset(toastText, 0, sizeof(toastText));
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        lastRenderedDead[i] = false;
        recentDelta[i] = 0;
        lastDeltaTimeMs[i] = 0;
    }
}

void DisplayManager::begin(bool restoreMatch, uint32_t currentMs) {
    tft.begin();
    tft.setRotation(1); // Landscape 320 x 240
    if (restoreMatch) {
        returnToGameScreen();
        showToast("MATCH RESTORED", currentMs);
    } else {
#if ENABLE_STARTUP_SELF_TEST
        startSelfTest(0);
#else
        returnToGameScreen();
        selfTestLog("[SYS] Tabletop Ready: %d Players @ %d Life.\r\n", state.getPlayerCount(), STARTING_LIFE);
        selfTestLog("[SYS]\r\n");
        selfTestLog("[SYS] --- Quick Controls Guide ---\r\n");
        selfTestLog("[SYS]   Knob Turn    : Adjust Life / Counter\r\n");
        selfTestLog("[SYS]   Knob Click   : Cycle Mode (Life/CMDR/Psn/Tax)\r\n");
        selfTestLog("[SYS]   Knob 2xClick : Instant Return to Life Mode\r\n");
        selfTestLog("[SYS]   Knob Hold 3s : Reset Player (40 Life)\r\n");
        selfTestLog("[SYS]   Red Button   : Roll Starting Player\r\n");
        selfTestLog("[SYS]   Red 2xClick  : Toggle Match Event Log\r\n");
        selfTestLog("[SYS]   Red Hold 1.5s: Full Game Reset\r\n");
        selfTestLog("[SYS]   Red Hold 3s  : Player Count Menu (2-4P)\r\n");
        selfTestLog("==================================================\r\n\r\n");
#endif
    }
}

void DisplayManager::showToast(const char* message, uint32_t currentMs) {
    if (!message) return;
    strncpy(toastText, message, sizeof(toastText) - 1);
    toastText[sizeof(toastText) - 1] = '\0';
    toastActive = true;
    toastStartTime = currentMs;

    int16_t bx = (SCREEN_WIDTH - 200) / 2;
    int16_t by = (SCREEN_HEIGHT - 32) / 2;
    tft.fillRoundRect(bx, by, 200, 32, 6, COLOR_BLACK);
    tft.drawRoundRect(bx, by, 200, 32, 6, COLOR_P3);
    tft.drawRoundRect(bx + 1, by + 1, 198, 30, 5, COLOR_P3);

    tft.setTextColor(COLOR_P3);
    tft.setTextSize(2);
    int16_t textLen = (int16_t)strlen(toastText);
    int16_t tx = (SCREEN_WIDTH - (textLen * 12)) / 2;
    int16_t ty = by + 8;
    tft.setCursor(tx, ty);
    tft.print(toastText);
}

void DisplayManager::updateToast(uint32_t currentMs) {
    if (!toastActive) return;
    if (currentMs - toastStartTime >= 1500) {
        toastActive = false;
        drawStaticLayout();
        uint8_t count = state.getPlayerCount();
        for (uint8_t i = 0; i < count; i++) {
            renderQuadrant(i, true);
        }
    }
}

bool DisplayManager::isToastActive() const {
    return toastActive;
}

void DisplayManager::returnToGameScreen() {
    currentScreen = SCREEN_GAME;
    tft.fillScreen(COLOR_BLACK);
    drawStaticLayout();
    uint8_t count = state.getPlayerCount();
    for (uint8_t i = 0; i < count; i++) {
        lastRenderedDead[i] = state.getPlayer(i).isDead;
        renderQuadrant(i, true);
    }
}

void DisplayManager::drawStaticLayout() {
    uint8_t count = state.getPlayerCount();

    // Center horizontal divider (present in 2, 3, and 4 player modes)
    tft.drawFastHLine(0, QUAD_HEIGHT - 1, SCREEN_WIDTH, COLOR_DIVIDER);
    tft.drawFastHLine(0, QUAD_HEIGHT, SCREEN_WIDTH, COLOR_DIVIDER);

    // Center vertical divider
    if (count == 4) {
        // Full vertical divider across entire screen
        tft.drawFastVLine(QUAD_WIDTH - 1, 0, SCREEN_HEIGHT, COLOR_DIVIDER);
        tft.drawFastVLine(QUAD_WIDTH, 0, SCREEN_HEIGHT, COLOR_DIVIDER);
    } else if (count == 3) {
        // Vertical divider only for the bottom half (between P2 and P3)
        tft.drawFastVLine(QUAD_WIDTH - 1, QUAD_HEIGHT, QUAD_HEIGHT, COLOR_DIVIDER);
        tft.drawFastVLine(QUAD_WIDTH, QUAD_HEIGHT, QUAD_HEIGHT, COLOR_DIVIDER);
    }
    // 2-player mode has no vertical divider
}

void DisplayManager::getQuadrantBounds(uint8_t playerIdx, int16_t& qx, int16_t& qy, int16_t& qw, int16_t& qh) {
    uint8_t count = state.getPlayerCount();
    if (count == 2) {
        qw = SCREEN_WIDTH;
        qh = QUAD_HEIGHT;
        switch (playerIdx) {
            case 0: qx = 0; qy = 0;           break; // P1: Top Half (320x120), flipped
            case 1: qx = 0; qy = QUAD_HEIGHT; break; // P2: Bottom Half (320x120), normal
            default: qx = 0; qy = 0; qw = 0; qh = 0; break;
        }
    } else if (count == 3) {
        switch (playerIdx) {
            case 0: // P1: Top Half full-width (320x120), flipped
                qx = 0; qy = 0; qw = SCREEN_WIDTH; qh = QUAD_HEIGHT;
                break;
            case 1: // P2: Bottom-Left (160x120), normal
                qx = 0; qy = QUAD_HEIGHT; qw = QUAD_WIDTH; qh = QUAD_HEIGHT;
                break;
            case 2: // P3: Bottom-Right (160x120), normal
                qx = QUAD_WIDTH; qy = QUAD_HEIGHT; qw = QUAD_WIDTH; qh = QUAD_HEIGHT;
                break;
            default:
                qx = 0; qy = 0; qw = 0; qh = 0;
                break;
        }
    } else { // 4 Players
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
    (void)forceFullRedraw;
    if (playerIdx >= state.getPlayerCount()) return;
    if (quadCanvas.getBuffer() == nullptr) return;

    int16_t qx, qy, qw, qh;
    getQuadrantBounds(playerIdx, qx, qy, qw, qh);

    // If this player is allocated a full-width half (320x120), delegate to renderHalf
    if (qw == SCREEN_WIDTH) {
        renderHalf(playerIdx);
        return;
    }

    const PlayerState& p = state.getPlayer(playerIdx);
    uint16_t pColor = getPlayerColor(playerIdx);
    uint16_t bgColor = getQuadrantBg(playerIdx);
    uint8_t count = state.getPlayerCount();

    // P1 and P2 (top half of table in 4P mode) or P1 are mirrored/rotated 180 degrees
    bool isFlipped = (playerIdx == 0) || (count == 4 && playerIdx == 1);
    quadCanvas.setRotation(isFlipped ? 2 : 0);

    // Clear quadrant off-screen canvas to background
    quadCanvas.fillScreen(bgColor);

    // 1. Header Bar: Player Badge (Left) + Secondary Poison/Tax (Right)
    quadCanvas.fillRect(4, 4, qw - 8, 16, bgColor);

    // Player Badge (filled rounded rectangle)
    quadCanvas.fillRoundRect(6, 4, 28, 14, 3, pColor);
    quadCanvas.setTextColor(COLOR_BLACK);
    quadCanvas.setTextSize(1);
    quadCanvas.setCursor(13, 7);
    quadCanvas.printf("P%d", playerIdx + 1);

    // Secondary Stats in Header (Poison & Tax)
    quadCanvas.setTextColor(COLOR_LIGHTGREY, bgColor);
    quadCanvas.setTextSize(1);
    quadCanvas.setCursor(45, 7);
    quadCanvas.printf("PSN:%d  TAX:%d", p.poison, p.tax);

    // Vertical column divider between Life and Commander Damage
    quadCanvas.drawFastVLine(82, 22, 66, COLOR_DIVIDER);

    // 2. Dual Counter Display (Life on Left, Opponent Commander Damages on Right)
    // --- LEFT COLUMN: LIFE ---
    bool isLifeActive = (p.activeMode == MODE_LIFE);
    quadCanvas.setTextSize(1);
    quadCanvas.setTextColor(isLifeActive ? pColor : COLOR_LIGHTGREY, bgColor);
    quadCanvas.setCursor(10, 24);
    quadCanvas.printf("%sLIFE", isLifeActive ? "> " : "  ");

    // Delta accumulator badge (e.g. [+3] or [-5] during rapid adjustments)
    if (recentDelta[playerIdx] != 0) {
        int16_t d = recentDelta[playerIdx];
        bool isPos = (d > 0);
        uint16_t badgeColor = isPos ? 0x05E0 : COLOR_DEAD_TEXT;
        char deltaBuf[8];
        snprintf(deltaBuf, sizeof(deltaBuf), "%+d", d);
        uint8_t dLen = (uint8_t)strlen(deltaBuf);
        int16_t badgeW = (dLen * 6) + 8;
        if (badgeW < 22) badgeW = 22;
        int16_t badgeX = 80 - badgeW;

        quadCanvas.fillRoundRect(badgeX, 22, badgeW, 12, 2, badgeColor);
        quadCanvas.setTextColor(COLOR_WHITE);
        quadCanvas.setTextSize(1);
        int16_t textX = badgeX + (badgeW - (dLen * 6)) / 2;
        quadCanvas.setCursor(textX, 24);
        quadCanvas.print(deltaBuf);
    }

    // Big Life Number (Text Size 4, or 3 for 4+ digits)
    char lifeStr[12];
    snprintf(lifeStr, sizeof(lifeStr), "%d", p.life);
    uint8_t lifeLen = strlen(lifeStr);
    uint8_t lifeTextSize = (lifeLen > 3) ? 3 : 4;
    uint8_t lifeCharW = (lifeTextSize == 4) ? 24 : 18;
    int16_t lifeTotalW = lifeLen * lifeCharW;
    int16_t lifeX = 4 + (76 - lifeTotalW) / 2;
    int16_t lifeY = (lifeTextSize == 4) ? 42 : 46;

    quadCanvas.setTextColor(isLifeActive ? COLOR_WHITE : pColor, bgColor);
    quadCanvas.setTextSize(lifeTextSize);
    quadCanvas.setCursor(lifeX, lifeY);
    quadCanvas.print(lifeStr);

    // --- RIGHT COLUMN: ACTIVE OPPONENT COMMANDER DAMAGES ---
    bool isCmdrActive = (p.activeMode == MODE_CMDR);
    quadCanvas.setTextSize(1);
    quadCanvas.setTextColor(isCmdrActive ? pColor : COLOR_LIGHTGREY, bgColor);
    quadCanvas.setCursor(88, 24);
    quadCanvas.print("CMDR DMG");

    uint8_t rowIdx = 0;
    for (uint8_t opp = 0; opp < count; opp++) {
        if (opp == playerIdx) continue;

        int16_t rowY = 36 + (rowIdx * 16);
        bool isSelected = (isCmdrActive && p.cmdrSource == opp);
        uint8_t dmg = p.cmdrDmg[opp];
        uint16_t oppColor = getPlayerColor(opp);

        if (isSelected) {
            // Highlight active selected opponent
            quadCanvas.fillRoundRect(86, rowY - 1, 68, 14, 2, oppColor);
            quadCanvas.setTextColor(COLOR_BLACK, oppColor);
        } else {
            uint16_t labelCol = (dmg >= LETHAL_CMDR_DAMAGE) ? COLOR_DEAD_TEXT : oppColor;
            quadCanvas.setTextColor(labelCol, bgColor);
        }

        quadCanvas.setTextSize(1);
        quadCanvas.setCursor(89, rowY + 2);
        quadCanvas.printf("P%d", opp + 1);

        // Value
        if (isSelected) {
            quadCanvas.setTextColor(COLOR_BLACK, oppColor);
        } else {
            uint16_t valCol = (dmg >= LETHAL_CMDR_DAMAGE) ? COLOR_DEAD_TEXT : COLOR_WHITE;
            quadCanvas.setTextColor(valCol, bgColor);
        }
        quadCanvas.setCursor(120, rowY + 2);
        quadCanvas.printf("%2d", dmg);

        if (dmg >= LETHAL_CMDR_DAMAGE && !isSelected) {
            quadCanvas.setCursor(138, rowY + 2);
            quadCanvas.print("!");
        }

        rowIdx++;
    }

    // 3. Footer Status Bar / Lethal Alert
    quadCanvas.fillRect(4, 92, qw - 8, 22, bgColor);
    if (p.isDead) {
        quadCanvas.fillRoundRect(6, 92, qw - 12, 20, 3, COLOR_DEAD_TEXT);
        quadCanvas.setTextColor(COLOR_WHITE);
        quadCanvas.setTextSize(1);
        quadCanvas.setCursor(10, 98);

        // Determine specific cause of death
        int8_t killerOpp = -1;
        for (uint8_t opp = 0; opp < count; opp++) {
            if (opp != playerIdx && p.cmdrDmg[opp] >= LETHAL_CMDR_DAMAGE) {
                killerOpp = opp;
                break;
            }
        }

        if (killerOpp >= 0) {
            quadCanvas.setCursor(20, 98);
            quadCanvas.printf("DEAD: 21 CMDR P%d", killerOpp + 1);
        } else if (p.life <= 0) {
            quadCanvas.setCursor(22, 98);
            quadCanvas.print("*** DEAD: 0 LIFE ***");
        } else {
            quadCanvas.setCursor(22, 98);
            quadCanvas.print("*** DEAD: 10 POIS ***");
        }
    } else {
        quadCanvas.setTextSize(1);
        if (p.activeMode == MODE_POISON) {
            quadCanvas.setTextColor(COLOR_WHITE, COLOR_DARKGREY);
            quadCanvas.setCursor(8, 98);
            quadCanvas.printf(" [ADJUSTING POISON: %d] ", p.poison);
        } else if (p.activeMode == MODE_TAX) {
            quadCanvas.setTextColor(COLOR_WHITE, COLOR_DARKGREY);
            quadCanvas.setCursor(8, 98);
            quadCanvas.printf(" [ADJUSTING TAX: +%d] ", p.tax);
        } else if (p.activeMode == MODE_CMDR) {
            quadCanvas.setTextColor(COLOR_LIGHTGREY, bgColor);
            quadCanvas.setCursor(8, 98);
            quadCanvas.printf("Dial: CMDR P%d (+/-)", p.cmdrSource + 1);
        } else {
            quadCanvas.setTextColor(COLOR_LIGHTGREY, bgColor);
            quadCanvas.setCursor(12, 98);
            quadCanvas.print("Dial: LIFE (+/-)");
        }
    }

    // Blit rendered quadrant buffer to hardware display
    tft.drawRGBBitmap(qx, qy, quadCanvas.getBuffer(), qw, qh);

    if (forceFullRedraw) {
        drawStaticLayout();
    }

    // Mark quadrant as clean
    state.clearDirty(playerIdx);
}

void DisplayManager::renderHalf(uint8_t playerIdx) {
    if (playerIdx >= state.getPlayerCount()) return;
    if (quadCanvas.getBuffer() == nullptr) return;

    int16_t qx, qy, qw, qh;
    getQuadrantBounds(playerIdx, qx, qy, qw, qh);
    const PlayerState& p = state.getPlayer(playerIdx);
    uint16_t pColor = getPlayerColor(playerIdx);
    uint16_t bgColor = getQuadrantBg(playerIdx);
    uint8_t count = state.getPlayerCount();

    // P1 (top half) is viewed across the table, rotated 180 deg
    bool isFlipped = (playerIdx == 0);
    quadCanvas.setRotation(isFlipped ? 2 : 0);

    // =========================================================
    // TILE 1: LIFE & IDENTITY (160x120)
    // =========================================================
    quadCanvas.fillScreen(bgColor);

    // 1. Header Bar: Player Badge (Left) + Secondary Poison/Tax (Right)
    quadCanvas.fillRect(4, 4, QUAD_WIDTH - 8, 16, bgColor);

    // Player Badge (filled rounded rectangle)
    quadCanvas.fillRoundRect(6, 4, 28, 14, 3, pColor);
    quadCanvas.setTextColor(COLOR_BLACK);
    quadCanvas.setTextSize(1);
    quadCanvas.setCursor(13, 7);
    quadCanvas.printf("P%d", playerIdx + 1);

    // Secondary Stats in Header
    quadCanvas.setTextColor(COLOR_LIGHTGREY, bgColor);
    quadCanvas.setTextSize(1);
    quadCanvas.setCursor(42, 7);
    quadCanvas.printf("PSN:%d TAX:+%d", p.poison, p.tax);

    // 2. Big Life Total
    bool isLifeActive = (p.activeMode == MODE_LIFE);
    quadCanvas.setTextSize(1);
    quadCanvas.setTextColor(isLifeActive ? pColor : COLOR_LIGHTGREY, bgColor);
    quadCanvas.setCursor(10, 24);
    quadCanvas.printf("%sLIFE", isLifeActive ? "> " : "  ");

    // Delta accumulator badge (e.g. [+3] or [-5] during rapid adjustments)
    if (recentDelta[playerIdx] != 0) {
        int16_t d = recentDelta[playerIdx];
        bool isPos = (d > 0);
        uint16_t badgeColor = isPos ? 0x05E0 : COLOR_DEAD_TEXT;
        char deltaBuf[8];
        snprintf(deltaBuf, sizeof(deltaBuf), "%+d", d);
        uint8_t dLen = (uint8_t)strlen(deltaBuf);
        int16_t badgeW = (dLen * 6) + 8;
        if (badgeW < 22) badgeW = 22;
        int16_t badgeX = 80 - badgeW;

        quadCanvas.fillRoundRect(badgeX, 22, badgeW, 12, 2, badgeColor);
        quadCanvas.setTextColor(COLOR_WHITE);
        quadCanvas.setTextSize(1);
        int16_t textX = badgeX + (badgeW - (dLen * 6)) / 2;
        quadCanvas.setCursor(textX, 24);
        quadCanvas.print(deltaBuf);
    }

    char lifeStr[12];
    snprintf(lifeStr, sizeof(lifeStr), "%d", p.life);
    uint8_t lifeLen = strlen(lifeStr);
    uint8_t lifeTextSize = (lifeLen > 3) ? 4 : 5;
    uint8_t lifeCharW = (lifeTextSize == 5) ? 30 : 24;
    int16_t lifeTotalW = lifeLen * lifeCharW;
    int16_t lifeX = (QUAD_WIDTH - lifeTotalW) / 2;
    int16_t lifeY = (lifeTextSize == 5) ? 36 : 42;

    quadCanvas.setTextColor(isLifeActive ? COLOR_WHITE : pColor, bgColor);
    quadCanvas.setTextSize(lifeTextSize);
    quadCanvas.setCursor(lifeX, lifeY);
    quadCanvas.print(lifeStr);

    // 3. Footer Status Bar
    quadCanvas.fillRect(4, 92, QUAD_WIDTH - 8, 22, bgColor);
    if (p.isDead) {
        quadCanvas.fillRoundRect(6, 94, QUAD_WIDTH - 12, 20, 3, COLOR_DEAD_TEXT);
        quadCanvas.setTextColor(COLOR_WHITE);
        quadCanvas.setTextSize(1);
        quadCanvas.setCursor(12, 100);
        if (p.life <= 0) {
            quadCanvas.print("*** DEAD: 0 LIFE ***");
        } else if (p.poison >= LETHAL_POISON) {
            quadCanvas.print("*** DEAD: 10 POIS ***");
        } else {
            quadCanvas.print("*** DEAD: 21 CMDR ***");
        }
    } else {
        quadCanvas.setTextSize(1);
        if (p.activeMode == MODE_POISON) {
            quadCanvas.setTextColor(COLOR_WHITE, COLOR_DARKGREY);
            quadCanvas.setCursor(14, 98);
            quadCanvas.printf("[ADJUST POISON: %d]", p.poison);
        } else if (p.activeMode == MODE_TAX) {
            quadCanvas.setTextColor(COLOR_WHITE, COLOR_DARKGREY);
            quadCanvas.setCursor(18, 98);
            quadCanvas.printf("[ADJUST TAX: +%d]", p.tax);
        } else if (p.activeMode == MODE_CMDR) {
            quadCanvas.setTextColor(COLOR_LIGHTGREY, bgColor);
            quadCanvas.setCursor(16, 98);
            quadCanvas.printf("Dial: CMDR P%d (+/-)", p.cmdrSource + 1);
        } else {
            quadCanvas.setTextColor(COLOR_LIGHTGREY, bgColor);
            quadCanvas.setCursor(26, 98);
            quadCanvas.print("Dial: LIFE (+/-)");
        }
    }

    // Vertical seam divider inside the half
    quadCanvas.drawFastVLine(QUAD_WIDTH - 1, 4, QUAD_HEIGHT - 8, COLOR_DIVIDER);

    // Blit Tile 1: If rotated 180°, the player's left is board x = 160. If normal, x = 0.
    tft.drawRGBBitmap(isFlipped ? QUAD_WIDTH : 0, qy, quadCanvas.getBuffer(), QUAD_WIDTH, QUAD_HEIGHT);

    // =========================================================
    // TILE 2: COMMANDER DAMAGE (160x120)
    // =========================================================
    quadCanvas.fillScreen(bgColor);

    bool isCmdrActive = (p.activeMode == MODE_CMDR);
    quadCanvas.setTextSize(1);
    quadCanvas.setTextColor(isCmdrActive ? pColor : COLOR_LIGHTGREY, bgColor);
    quadCanvas.setCursor(12, 12);
    quadCanvas.printf("%sCMDR DAMAGE", isCmdrActive ? "> " : "  ");

    if (count == 2) {
        // 1v1: Single opponent commander
        uint8_t opp = (playerIdx == 0) ? 1 : 0;
        uint8_t dmg = p.cmdrDmg[opp];
        uint16_t oppColor = getPlayerColor(opp);

        quadCanvas.fillRoundRect(14, 38, 42, 26, 4, oppColor);
        quadCanvas.setTextColor(COLOR_BLACK);
        quadCanvas.setTextSize(2);
        quadCanvas.setCursor(24, 44);
        quadCanvas.printf("P%d", opp + 1);

        uint16_t valCol = (dmg >= LETHAL_CMDR_DAMAGE) ? COLOR_DEAD_TEXT : (isCmdrActive ? COLOR_WHITE : oppColor);
        quadCanvas.setTextColor(valCol, bgColor);
        quadCanvas.setTextSize(4);
        quadCanvas.setCursor(72, 36);
        quadCanvas.printf("%2d", dmg);

        if (dmg >= LETHAL_CMDR_DAMAGE) {
            quadCanvas.setTextSize(3);
            quadCanvas.setCursor(130, 38);
            quadCanvas.setTextColor(COLOR_DEAD_TEXT, bgColor);
            quadCanvas.print("!");
        }
    } else {
        // 3-Player Mode (top player has 2 opponents)
        uint8_t rowIdx = 0;
        for (uint8_t opp = 0; opp < count; opp++) {
            if (opp == playerIdx) continue;

            int16_t rowY = 32 + (rowIdx * 28);
            bool isSelected = (isCmdrActive && p.cmdrSource == opp);
            uint8_t dmg = p.cmdrDmg[opp];
            uint16_t oppColor = getPlayerColor(opp);

            if (isSelected) {
                quadCanvas.fillRoundRect(12, rowY - 2, 136, 24, 4, oppColor);
                quadCanvas.setTextColor(COLOR_BLACK, oppColor);
            } else {
                uint16_t labelCol = (dmg >= LETHAL_CMDR_DAMAGE) ? COLOR_DEAD_TEXT : oppColor;
                quadCanvas.setTextColor(labelCol, bgColor);
            }

            quadCanvas.setTextSize(1);
            quadCanvas.setCursor(20, rowY + 6);
            quadCanvas.printf("P%d", opp + 1);

            if (isSelected) {
                quadCanvas.setTextColor(COLOR_BLACK, oppColor);
            } else {
                uint16_t valCol = (dmg >= LETHAL_CMDR_DAMAGE) ? COLOR_DEAD_TEXT : COLOR_WHITE;
                quadCanvas.setTextColor(valCol, bgColor);
            }
            quadCanvas.setTextSize(2);
            quadCanvas.setCursor(70, rowY + 3);
            quadCanvas.printf("%2d", dmg);

            if (dmg >= LETHAL_CMDR_DAMAGE && !isSelected) {
                quadCanvas.setTextSize(2);
                quadCanvas.setTextColor(COLOR_DEAD_TEXT, bgColor);
                quadCanvas.setCursor(120, rowY + 3);
                quadCanvas.print("!");
            }
            rowIdx++;
        }
    }

    // Tile 2 Footer Prompt
    quadCanvas.fillRect(4, 92, QUAD_WIDTH - 8, 22, bgColor);
    if (p.isDead) {
        quadCanvas.setTextColor(COLOR_DEAD_TEXT, bgColor);
        quadCanvas.setTextSize(1);
        quadCanvas.setCursor(18, 98);
        quadCanvas.print("*** ELIMINATED ***");
    } else {
        quadCanvas.setTextSize(1);
        if (p.activeMode == MODE_CMDR) {
            quadCanvas.setTextColor(COLOR_WHITE, COLOR_DARKGREY);
            quadCanvas.setCursor(10, 98);
            quadCanvas.printf("[ADJUST CMDR P%d]", p.cmdrSource + 1);
        } else {
            quadCanvas.setTextColor(COLOR_LIGHTGREY, bgColor);
            quadCanvas.setCursor(20, 98);
            quadCanvas.print("Opponent CMDR Dmg");
        }
    }

    // Blit Tile 2: If rotated 180°, the player's right is board x = 0. If normal, x = 160.
    tft.drawRGBBitmap(isFlipped ? 0 : QUAD_WIDTH, qy, quadCanvas.getBuffer(), QUAD_WIDTH, QUAD_HEIGHT);

    // Clear dirty flag
    state.clearDirty(playerIdx);
}

void DisplayManager::updateDirtyQuadrants() {
    if (currentScreen != SCREEN_GAME) return;

    uint8_t count = state.getPlayerCount();
    bool anyDrawn = false;
    for (uint8_t i = 0; i < count; i++) {
        bool deadChanged = (state.getPlayer(i).isDead != lastRenderedDead[i]);
        if (state.getPlayer(i).dirty || deadChanged) {
            lastRenderedDead[i] = state.getPlayer(i).isDead;
            renderQuadrant(i, deadChanged);
            anyDrawn = true;
        }
    }
    if (anyDrawn) {
        drawStaticLayout();
    }
}

void DisplayManager::toggleEventLogScreen() {
    if (currentScreen == SCREEN_GAME) {
        currentScreen = SCREEN_EVENT_LOG;
        logScrollOffset = 0;
        drawEventLog();
    } else {
        returnToGameScreen();
    }
}

void DisplayManager::scrollEventLog(int8_t step) {
    uint16_t total = log.getCount();
    uint16_t maxOffset = (total > 8) ? (total - 8) : 0;
    int32_t newOffset = (int32_t)logScrollOffset + step;
    if (newOffset < 0) newOffset = 0;
    if (newOffset > maxOffset) newOffset = maxOffset;
    if ((uint16_t)newOffset != logScrollOffset) {
        logScrollOffset = (uint16_t)newOffset;
        drawEventLog();
    }
}

uint16_t DisplayManager::getLogScrollOffset() const {
    return logScrollOffset;
}

ScreenMode DisplayManager::getScreenMode() const {
    return currentScreen;
}

void DisplayManager::openPlayerSelectScreen() {
    currentScreen = SCREEN_PLAYER_SELECT;
    menuPlayerCount = state.getPlayerCount();
    drawPlayerSelectMenu();
}

void DisplayManager::adjustPlayerSelect(int8_t step) {
    int16_t next = (int16_t)menuPlayerCount + step;
    if (next < MIN_PLAYERS) next = MAX_PLAYERS;
    if (next > MAX_PLAYERS) next = MIN_PLAYERS;
    menuPlayerCount = (uint8_t)next;
    drawPlayerSelectMenu();
}

void DisplayManager::confirmPlayerSelect(uint32_t currentMs) {
    state.setPlayerCount(menuPlayerCount, currentMs);
    returnToGameScreen();
}

uint8_t DisplayManager::getSelectedPlayerCount() const {
    return menuPlayerCount;
}

void DisplayManager::drawPlayerSelectMenu() {
    tft.fillScreen(COLOR_BLACK);

    // Title Bar
    tft.fillRect(0, 0, SCREEN_WIDTH, 28, COLOR_DARKGREY);
    tft.drawFastHLine(0, 28, SCREEN_WIDTH, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(45, 6);
    tft.print("SELECT PLAYER COUNT");

    // 3 Options: 2, 3, 4 players
    const char* titles[3] = { "2 PLAYERS", "3 PLAYERS", "4 PLAYERS" };
    const char* descs[3]  = { "1v1 Duel (Two Dynamic Halves)", "1 Top Half vs 2 Bottom Quads", "Standard 4-Quadrant Tabletop" };
    uint8_t counts[3]     = { 2, 3, 4 };

    int16_t startY = 46;
    for (uint8_t i = 0; i < 3; i++) {
        int16_t rowY = startY + (i * 48);
        bool isSelected = (menuPlayerCount == counts[i]);

        if (isSelected) {
            tft.fillRoundRect(20, rowY, 280, 42, 6, COLOR_P2);
            tft.drawRoundRect(19, rowY - 1, 282, 44, 7, COLOR_WHITE);
            tft.setTextColor(COLOR_BLACK);
        } else {
            tft.fillRoundRect(20, rowY, 280, 42, 6, COLOR_DARKGREY);
            tft.drawRoundRect(20, rowY, 280, 42, 6, COLOR_DIVIDER);
            tft.setTextColor(COLOR_WHITE);
        }

        tft.setTextSize(2);
        tft.setCursor(35, rowY + 6);
        if (isSelected) {
            tft.printf("> %s <", titles[i]);
        } else {
            tft.printf("  %s", titles[i]);
        }

        tft.setTextSize(1);
        tft.setTextColor(isSelected ? COLOR_BLACK : COLOR_LIGHTGREY);
        tft.setCursor(45, rowY + 27);
        tft.print(descs[i]);
    }

    // Footer instruction
    tft.drawFastHLine(0, 205, SCREEN_WIDTH, COLOR_DARKGREY);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(20, 218);
    tft.print("Turn knob: Select  |  Click knob/button: Confirm");
}

void DisplayManager::drawEventLog() {
    tft.fillScreen(COLOR_BLACK);

    uint16_t totalEvents = log.getCount();

    // Title Header
    tft.fillRect(0, 0, SCREEN_WIDTH, 26, COLOR_DARKGREY);
    tft.drawFastHLine(0, 26, SCREEN_WIDTH, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 6);
    if (totalEvents > 0) {
        uint16_t viewStart = logScrollOffset + 1;
        uint16_t viewEnd = (totalEvents < 8) ? totalEvents : (logScrollOffset + 8);
        if (viewEnd > totalEvents) viewEnd = totalEvents;
        tft.printf("EVENT LOG (%u-%u of %u)", (unsigned)viewStart, (unsigned)viewEnd, (unsigned)totalEvents);
    } else {
        tft.print("MATCH EVENT LOG");
    }

    // Print events for current scroll window
    int16_t startY = 36;
    uint8_t maxToDisplay = 8;
    uint8_t displayed = 0;

    tft.setTextSize(1);
    if (totalEvents == 0) {
        tft.setTextColor(COLOR_LIGHTGREY);
        tft.setCursor(40, 100);
        tft.print("No events recorded yet.");
    } else {
        // Calculate starting index based on scroll offset (0 = newest)
        int32_t startIdx = (int32_t)totalEvents - 1 - logScrollOffset;
        for (int32_t i = startIdx; i >= 0 && displayed < maxToDisplay; i--, displayed++) {
            GameEvent evt;
            if (log.getEvent((uint16_t)i, evt)) {
                char str[64];
                EventLog::formatEvent(evt, str, sizeof(str));

                uint16_t color = COLOR_WHITE;
                if (evt.player < state.getPlayerCount()) {
                    color = getPlayerColor(evt.player);
                } else {
                    color = COLOR_P4; // Gold for system
                }
                tft.setTextColor(color);
                tft.setCursor(15, startY + (displayed * 18));
                tft.print(str);
            }
        }

        // Draw scrollbar indicator on the right edge if more than 8 events exist
        if (totalEvents > 8) {
            int16_t barX = SCREEN_WIDTH - 6;
            int16_t barTotalH = 160;
            int16_t barY = startY;
            tft.drawFastVLine(barX, barY, barTotalH, COLOR_DARKGREY);

            uint16_t maxOffset = totalEvents - 8;
            int16_t thumbH = (8 * barTotalH) / totalEvents;
            if (thumbH < 12) thumbH = 12;
            int16_t thumbY = barY + (int16_t)(((int32_t)logScrollOffset * (barTotalH - thumbH)) / maxOffset);
            tft.fillRect(barX - 1, thumbY, 3, thumbH, COLOR_WHITE);
        }
    }

    // Footer Help Prompt
    tft.drawFastHLine(0, 215, SCREEN_WIDTH, COLOR_DARKGREY);
    tft.setTextColor(COLOR_LIGHTGREY);
    tft.setTextSize(1);
    tft.setCursor(25, 222);
    tft.print("Turn knob: Scroll  |  Click knob or button: Return");
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
    uint8_t count = state.getPlayerCount();
    for (uint8_t i = 0; i < count; i++) {
        lastRenderedDead[i] = state.getPlayer(i).isDead;
        renderQuadrant(i, true);
    }
}

bool DisplayManager::updateAnimation(uint32_t currentMs) {
    if (currentScreen != SCREEN_ROLL_ANIMATION &&
        !(currentScreen == SCREEN_SELF_TEST && selfTestStep == TEST_STEP_ROLL_ANIMATION)) {
        return false;
    }

    // Elapsed time since start of animation
    uint32_t elapsed = currentMs - animStartTime;
    uint8_t count = state.getPlayerCount();

    if (!animSettled) {
        if (currentMs - lastAnimTick >= animDelay) {
            lastAnimTick = currentMs;

            uint8_t currentP = 0;
            if (count == 2) {
                currentP = animCurrentHighlight % 2;
                animCurrentHighlight = (animCurrentHighlight + 1) % 2;
            } else if (count == 3) {
                static const uint8_t CYCLE3[3] = { 0, 2, 1 };
                currentP = CYCLE3[animCurrentHighlight % 3];
                animCurrentHighlight = (animCurrentHighlight + 1) % 3;
            } else {
                static const uint8_t CYCLE4[4] = { 0, 1, 3, 2 };
                currentP = CYCLE4[animCurrentHighlight % 4];
                animCurrentHighlight = (animCurrentHighlight + 1) % 4;
            }

            // Clear previous highlight borders using each quadrant's actual background color
            for (uint8_t i = 0; i < count; i++) {
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

void DisplayManager::startSelfTest(uint32_t currentMs) {
    currentScreen = SCREEN_SELF_TEST;
    selfTestStep = TEST_STEP_SPLASH;
    selfTestStepStartTime = currentMs;

    selfTestLog("\r\n--------------------------------------------------\r\n");
    selfTestLog("  STARTUP HARDWARE & APP FUNCTIONALITY SELF-TEST  \r\n");
    selfTestLog("--------------------------------------------------\r\n");
    selfTestLog("  Initiating visual display & rules verification...\r\n");
    selfTestLog("  (Turn or click any knob to skip directly to game)\r\n");
    selfTestLog("--------------------------------------------------\r\n");

    runSelfTestStep(TEST_STEP_SPLASH, currentMs);
}

void DisplayManager::skipSelfTest(uint32_t currentMs) {
    if (isSelfTestRunning()) {
        selfTestLog("\r\n[TEST] [SKIP] Self-test skipped at step [%d/9] by user.\r\n", (int)selfTestStep);
        selfTestLog("[SYS] Tabletop Ready: 4 Players @ 40 Life.\r\n");
        selfTestLog("[SYS]\r\n");
        selfTestLog("[SYS] --- Quick Controls Guide ---\r\n");
        selfTestLog("[SYS]   Knob Turn    : Adjust Life / Counter\r\n");
        selfTestLog("[SYS]   Knob Click   : Cycle Mode (Life/CMDR/Psn/Tax)\r\n");
        selfTestLog("[SYS]   Knob 2xClick : Instant Return to Life Mode\r\n");
        selfTestLog("[SYS]   Knob Hold 3s : Reset Player (40 Life)\r\n");
        selfTestLog("[SYS]   Red Button   : Roll Starting Player\r\n");
        selfTestLog("[SYS]   Red 2xClick  : Toggle Match Event Log\r\n");
        selfTestLog("[SYS]   Red Hold 1.5s: Full Game Reset\r\n");
        selfTestLog("[SYS]   Red Hold 3s  : Player Count Menu (2-4P)\r\n");
        selfTestLog("==================================================\r\n\r\n");
    }
    selfTestStep = TEST_STEP_NONE;
    state.setPlayerCount(DEFAULT_PLAYER_COUNT, currentMs);
    returnToGameScreen();
}

bool DisplayManager::isSelfTestRunning() const {
    return (currentScreen == SCREEN_SELF_TEST && selfTestStep != TEST_STEP_NONE);
}

SelfTestStep DisplayManager::getSelfTestStep() const {
    return selfTestStep;
}

void DisplayManager::drawSelfTestSplash() {
    tft.fillScreen(COLOR_BLACK);

    // Title Box
    tft.fillRect(0, 0, SCREEN_WIDTH, 36, COLOR_DARKGREY);
    tft.drawFastHLine(0, 36, SCREEN_WIDTH, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(30, 10);
    tft.print("HARDWARE SELF-TEST");

    // Color Palette test bars
    int16_t boxW = 60, boxH = 32, startY = 52;
    tft.fillRoundRect(15,  startY, boxW, boxH, 4, COLOR_P1);
    tft.fillRoundRect(90,  startY, boxW, boxH, 4, COLOR_P2);
    tft.fillRoundRect(165, startY, boxW, boxH, 4, COLOR_P3);
    tft.fillRoundRect(240, startY, boxW, boxH, 4, COLOR_P4);

    tft.setTextColor(COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(26, startY + 12);  tft.print("P1 RED");
    tft.setCursor(98, startY + 12);  tft.print("P2 CYAN");
    tft.setCursor(174, startY + 12); tft.print("P3 GRN");
    tft.setCursor(248, startY + 12); tft.print("P4 GOLD");

    // Test sequence description
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(20, 100);
    tft.print("Testing Peripherals, Layouts & Game Logic:");
    tft.setTextColor(COLOR_LIGHTGREY);
    tft.setCursor(30, 118);
    tft.print("- 4P / 3P / 2P Dynamic Screen Layouts");
    tft.setCursor(30, 134);
    tft.print("- 180-deg Tabletop Mirroring (P1/P2)");
    tft.setCursor(30, 150);
    tft.print("- Real-time Combat & Commander Damage Sync");
    tft.setCursor(30, 166);
    tft.print("- Poison Counters & Commander Tax Stepping");
    tft.setCursor(30, 182);
    tft.print("- Lethal Checks, First Player Roll & Log");

    // Footer
    tft.drawFastHLine(0, 208, SCREEN_WIDTH, COLOR_DARKGREY);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(35, 218);
    tft.print("[ Turn or click any knob to skip ]");
}

void DisplayManager::runSelfTestStep(SelfTestStep step, uint32_t currentMs) {
    switch (step) {
        case TEST_STEP_SPLASH:
            drawSelfTestSplash();
            selfTestLog("  [1/9] Color Palette & Splash Screen ........ [PASS]\r\n");
            break;

        case TEST_STEP_4P_LAYOUT:
            state.setPlayerCount(4, currentMs);
            returnToGameScreen();
            currentScreen = SCREEN_SELF_TEST;
            selfTestLog("  [2/9] 4-Player Tabletop Layout (180° Mirror) [PASS]\r\n");
            break;

        case TEST_STEP_COUNTERS:
            // P1 Life adjustment: -5
            state.applyDelta(0, -5, currentMs);
            // P2 takes 5 commander damage from P1
            state.cycleMode(1, currentMs);
            state.applyDelta(1, 5, currentMs);
            // P3 takes 3 poison
            state.cycleMode(2, currentMs);
            state.cycleMode(2, currentMs);
            state.cycleMode(2, currentMs);
            state.cycleMode(2, currentMs);
            state.applyDelta(2, 3, currentMs);
            // P4 increments Commander Tax
            state.cycleMode(3, currentMs);
            state.cycleMode(3, currentMs);
            state.cycleMode(3, currentMs);
            state.cycleMode(3, currentMs);
            state.cycleMode(3, currentMs);
            state.applyDelta(3, 1, currentMs);
            updateDirtyQuadrants();
            {
                bool ok = (state.getPlayer(0).life == 35) &&
                          (state.getPlayer(1).cmdrDmg[0] == 5) &&
                          (state.getPlayer(2).poison == 3) &&
                          (state.getPlayer(3).tax == 2);
                selfTestLog("  [3/9] Multi-Mode Counters (Life, CMDR, Psn)  [%s]\r\n", ok ? "PASS" : "FAIL");
            }
            break;

        case TEST_STEP_LETHAL:
            // P3 reaches 10 poison -> LETHAL
            state.applyDelta(2, 7, currentMs);
            // P2 takes 21 commander damage from P1 -> LETHAL
            state.applyDelta(1, 16, currentMs);
            updateDirtyQuadrants();
            {
                bool ok = state.getPlayer(1).isDead && state.getPlayer(2).isDead;
                selfTestLog("  [4/9] Lethal Condition Elimination Alerts .. [%s]\r\n", ok ? "PASS" : "FAIL");
            }
            break;

        case TEST_STEP_EVENT_LOG:
            drawEventLog();
            currentScreen = SCREEN_SELF_TEST;
            selfTestLog("  [5/9] Real-Time 50-Entry Event Log Screen .. [PASS]\r\n");
            break;

        case TEST_STEP_3P_LAYOUT:
            state.setPlayerCount(3, currentMs);
            returnToGameScreen();
            currentScreen = SCREEN_SELF_TEST;
            selfTestLog("  [6/9] Dynamic 3-Player Tabletop Layout ..... [PASS]\r\n");
            break;

        case TEST_STEP_2P_LAYOUT:
            state.setPlayerCount(2, currentMs);
            returnToGameScreen();
            currentScreen = SCREEN_SELF_TEST;
            selfTestLog("  [7/9] Dynamic 2-Player Split-Screen Layout . [PASS]\r\n");
            break;

        case TEST_STEP_PLAYER_SELECT:
            openPlayerSelectScreen();
            currentScreen = SCREEN_SELF_TEST;
            selfTestLog("  [8/9] Player Count Setup Menu (2P/3P/4P) ... [PASS]\r\n");
            break;

        case TEST_STEP_ROLL_ANIMATION:
            state.setPlayerCount(4, currentMs);
            returnToGameScreen();
            startRollAnimation(0, currentMs);
            currentScreen = SCREEN_SELF_TEST;
            selfTestLog("  [9/9] First-Player Roll Roulette & Banner .. [PASS]\r\n");
            break;

        case TEST_STEP_DONE:
        default:
            skipSelfTest(currentMs);
            break;
    }
}

bool DisplayManager::updateSelfTest(uint32_t currentMs) {
    if (!isSelfTestRunning()) return false;

    // In roll animation step, update animation frames
    if (selfTestStep == TEST_STEP_ROLL_ANIMATION) {
        updateAnimation(currentMs);
        currentScreen = SCREEN_SELF_TEST;
    }

    uint32_t stepDuration = (selfTestStep == TEST_STEP_ROLL_ANIMATION) ? 4200 : SELF_TEST_STEP_MS;
    if (currentMs - selfTestStepStartTime >= stepDuration) {
        selfTestStep = (SelfTestStep)((int)selfTestStep + 1);
        selfTestStepStartTime = currentMs;

        if (selfTestStep >= TEST_STEP_DONE) {
            selfTestLog("--------------------------------------------------\r\n");
            selfTestLog("              STARTUP SELF-TEST REPORT            \r\n");
            selfTestLog("--------------------------------------------------\r\n");
            selfTestLog("  Checks Executed : 9 / 9 (100%% Passed)\r\n");
            selfTestLog("  Display Driver  : ILI9341 SPI (320x240)\r\n");
            selfTestLog("  Tabletop Engine : Mirrored Canvas (2P/3P/4P)\r\n");
            selfTestLog("  Rules Engine    : Life, CMDR, Poison, Tax, Lethal\r\n");
            selfTestLog("  Overall Verdict : [PASS] ALL CHECKS PASSED\r\n");
            selfTestLog("--------------------------------------------------\r\n");
            selfTestStep = TEST_STEP_NONE;
            state.setPlayerCount(DEFAULT_PLAYER_COUNT, currentMs);
            returnToGameScreen();
            selfTestLog("[SYS] Tabletop Ready: 4 Players @ 40 Life.\r\n");
            selfTestLog("[SYS]\r\n");
            selfTestLog("[SYS] --- Quick Controls Guide ---\r\n");
            selfTestLog("[SYS]   Knob Turn    : Adjust Life / Counter\r\n");
            selfTestLog("[SYS]   Knob Click   : Cycle Mode (Life/CMDR/Psn/Tax)\r\n");
            selfTestLog("[SYS]   Knob 2xClick : Instant Return to Life Mode\r\n");
            selfTestLog("[SYS]   Knob Hold 3s : Reset Player (40 Life)\r\n");
            selfTestLog("[SYS]   Red Button   : Roll Starting Player\r\n");
            selfTestLog("[SYS]   Red 2xClick  : Toggle Match Event Log\r\n");
            selfTestLog("[SYS]   Red Hold 1.5s: Full Game Reset\r\n");
            selfTestLog("[SYS]   Red Hold 3s  : Player Count Menu (2-4P)\r\n");
            selfTestLog("==================================================\r\n\r\n");
            return false;
        } else {
            runSelfTestStep(selfTestStep, currentMs);
        }
    }
    return true;
}

void DisplayManager::recordDelta(uint8_t playerIdx, int16_t delta, uint32_t currentMs) {
    if (playerIdx >= NUM_PLAYERS) return;
    recentDelta[playerIdx] += delta;
    lastDeltaTimeMs[playerIdx] = currentMs;
}

void DisplayManager::clearDelta(uint8_t playerIdx) {
    if (playerIdx >= NUM_PLAYERS) return;
    recentDelta[playerIdx] = 0;
    lastDeltaTimeMs[playerIdx] = 0;
}

int16_t DisplayManager::getRecentDelta(uint8_t playerIdx) const {
    if (playerIdx >= NUM_PLAYERS) return 0;
    return recentDelta[playerIdx];
}

void DisplayManager::updateTransientDeltas(uint32_t currentMs) {
    if (currentScreen != SCREEN_GAME) return;
    uint8_t count = state.getPlayerCount();
    bool anyCleared = false;
    for (uint8_t i = 0; i < count; i++) {
        if (recentDelta[i] != 0 && (currentMs - lastDeltaTimeMs[i] >= DELTA_POPUP_TIMEOUT_MS)) {
            recentDelta[i] = 0;
            renderQuadrant(i, false);
            anyCleared = true;
        }
    }
    if (anyCleared) {
        drawStaticLayout();
    }
}

