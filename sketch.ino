#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include "Config.h"
#include "EventLog.h"
#include "GameState.h"
#include "AppController.h"
#include "InputDecoder.h"
#include "DisplayManager.h"
#include "StorageManager.h"

// Hardware display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Core business logic, HID controller, persistence & state
EventLog matchLog;
GameState gameState(matchLog);
AppController controller(gameState, matchLog);
StorageManager persistence(gameState, matchLog);
DisplayManager displayMgr(tft, gameState, matchLog);

// Encoder hardware pin mapping
const uint8_t ENCODER_CLK_PINS[NUM_PLAYERS] = { P1_CLK, P2_CLK, P3_CLK, P4_CLK };
const uint8_t ENCODER_DT_PINS[NUM_PLAYERS]  = { P1_DT,  P2_DT,  P3_DT,  P4_DT  };
const uint8_t ENCODER_SW_PINS[NUM_PLAYERS]  = { P1_SW,  P2_SW,  P3_SW,  P4_SW  };

// Input state decoders
RotaryDecoder   rotaryDecoders[NUM_PLAYERS];
ButtonDebouncer rotaryButtons[NUM_PLAYERS];
ButtonDebouncer tableButton;

#if defined(ESP32) || defined(ARDUINO)
static void IRAM_ATTR encoderIsr0() {
  rotaryDecoders[0].update(digitalRead(P1_CLK) == HIGH, digitalRead(P1_DT) == HIGH);
}
static void IRAM_ATTR encoderIsr1() {
  rotaryDecoders[1].update(digitalRead(P2_CLK) == HIGH, digitalRead(P2_DT) == HIGH);
}
static void IRAM_ATTR encoderIsr2() {
  rotaryDecoders[2].update(digitalRead(P3_CLK) == HIGH, digitalRead(P3_DT) == HIGH);
}
static void IRAM_ATTR encoderIsr3() {
  rotaryDecoders[3].update(digitalRead(P4_CLK) == HIGH, digitalRead(P4_DT) == HIGH);
}
#endif

// Helper function to print clean serial logs with automatic CRLF expansion
static void sysLog(const char* format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  // Expand lone \n to \r\n to prevent diagonal stair-stepping in serial terminals
  char outBuf[512];
  size_t j = 0;
  for (size_t i = 0; buf[i] != '\0' && j < sizeof(outBuf) - 2; i++) {
    if (buf[i] == '\n' && (i == 0 || buf[i - 1] != '\r')) {
      outBuf[j++] = '\r';
    }
    outBuf[j++] = buf[i];
  }
  outBuf[j] = '\0';
  Serial.print(outBuf);
}

static void printQuickControlsGuide() {
  sysLog("[SYS]\r\n");
  sysLog("[SYS] --- Quick Controls Guide ---\r\n");
  sysLog("[SYS]   Knob Turn    : Adjust Life / Counter\r\n");
  sysLog("[SYS]   Knob Click   : Cycle Mode (Life/CMDR/Psn/Tax)\r\n");
  sysLog("[SYS]   Knob 2xClick : Instant Return to Life Mode\r\n");
  sysLog("[SYS]   Knob Hold 3s : Reset Player (%d Life)\r\n", STARTING_LIFE);
  sysLog("[SYS]   Red Button   : Roll Starting Player\r\n");
  sysLog("[SYS]   Red 2xClick  : Toggle Match Event Log\r\n");
  sysLog("[SYS]   Red Hold 1.5s: Full Game Reset\r\n");
  sysLog("[SYS]   Red Hold 3s  : Player Count Menu (2-4P)\r\n");
  sysLog("==================================================\r\n\r\n");
}

void setup() {
  Serial.begin(115200);
  delay(150);
  
  sysLog("\r\n==================================================\r\n");
  sysLog("  MTG COMMANDER LIFE TRACKER - FIRMWARE v1.0\r\n");
  sysLog("==================================================\r\n");

  // Configure rotary encoder pins (active-low)
  for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
    pinMode(ENCODER_CLK_PINS[i], INPUT_PULLUP);
    // Pin 34..39 on ESP32 are input-only (GPI); KY-040 breakout board has on-module pull-up
    if (ENCODER_DT_PINS[i] >= 34) {
      pinMode(ENCODER_DT_PINS[i], INPUT);
    } else {
      pinMode(ENCODER_DT_PINS[i], INPUT_PULLUP);
    }
    pinMode(ENCODER_SW_PINS[i],  INPUT_PULLUP);

    // Player knob hold threshold: 3000 ms to prevent accidental player reset
    rotaryButtons[i].setLongPressMs(PLAYER_RESET_LONG_PRESS_MS);
  }

#if defined(ESP32) || defined(ARDUINO)
  attachInterrupt(digitalPinToInterrupt(P1_CLK), encoderIsr0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(P2_CLK), encoderIsr1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(P3_CLK), encoderIsr2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(P4_CLK), encoderIsr3, CHANGE);
#endif

  // Configure table reset/roll button (1500 ms hold for reset, 3000 ms hold for Player Count menu)
  pinMode(BTN_RESET, INPUT_PULLUP);
  tableButton.setLongPressMs(TABLE_RESET_LONG_PRESS_MS);
  tableButton.setVeryLongPressMs(TABLE_MENU_LONG_PRESS_MS);
  tableButton.setDeferSingleClick(true);

  // Seed random generator with analog noise on unconnected pin
  randomSeed(analogRead(36) + esp_random());

  // Check Non-Volatile Storage (Flash) for saved match recovery
  persistence.begin();
  bool matchRestored = false;
  if (persistence.hasSavedMatch() && persistence.loadMatch()) {
    matchRestored = true;
    sysLog("[SYS] *** Active Match Restored from Flash: %d Players ***\r\n", gameState.getPlayerCount());
    sysLog("[SYS] Startup self-test bypassed (resuming active game).\r\n");
    sysLog("[SYS] Tabletop Ready: %d Players @ %d Life.\r\n", gameState.getPlayerCount(), STARTING_LIFE);
    printQuickControlsGuide();
  }

  // Initialize display and render layout (bypasses self-test if match was restored)
  displayMgr.begin(matchRestored, millis() + persistence.getRestoredTimestampOffset());
}

static uint8_t getPlayerIndexForKnob(uint8_t knobIdx, uint8_t playerCount) {
  if (playerCount == 2) {
    return (knobIdx < 2) ? 0 : 1;
  } else if (playerCount == 3) {
    if (knobIdx < 2) return 0;
    return (knobIdx == 2) ? 1 : 2;
  }
  return knobIdx;
}

void loop() {
  uint32_t currentMs = millis() + persistence.getRestoredTimestampOffset();

  // If a startup self-test is running, advance it or skip on any user input
  if (displayMgr.isSelfTestRunning()) {
    bool hasInput = false;

    // Check rotary encoders
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
      bool clkLevel = (digitalRead(ENCODER_CLK_PINS[i]) == HIGH);
      bool dtLevel  = (digitalRead(ENCODER_DT_PINS[i])  == HIGH);
      int8_t delta = rotaryDecoders[i].update(clkLevel, dtLevel);
      if (delta != 0) {
        hasInput = true;
      }

      bool isSwPressed = (digitalRead(ENCODER_SW_PINS[i]) == LOW);
      ButtonEvent swEvt = rotaryButtons[i].update(isSwPressed, currentMs);
      if (swEvt != BTN_EVENT_NONE) {
        hasInput = true;
      }
    }

    // Check table button
    bool isResetPressed = (digitalRead(BTN_RESET) == LOW);
    ButtonEvent rstEvt = tableButton.update(isResetPressed, currentMs);
    if (rstEvt != BTN_EVENT_NONE) {
      hasInput = true;
    }

    if (hasInput) {
      displayMgr.skipSelfTest(currentMs);
    } else {
      displayMgr.updateSelfTest(currentMs);
    }
    delay(1);
    return;
  }

  // If a first-player roll animation is running, process its frame (skip on table button tap)
  if (displayMgr.getScreenMode() == SCREEN_ROLL_ANIMATION) {
    bool isResetPressed = (digitalRead(BTN_RESET) == LOW);
    ButtonEvent rstEvt = tableButton.update(isResetPressed, currentMs);
    if (rstEvt != BTN_EVENT_NONE) {
      displayMgr.returnToGameScreen();
      return;
    }
    displayMgr.updateAnimation(currentMs);
    return;
  }

  // Update visual recovery toast if active
  displayMgr.updateToast(currentMs);

  uint8_t activePlayerCount = gameState.getPlayerCount();

  // -------------------------------------------------------------
  // 1. Process 4 Rotary Encoders (Rotation & Switch)
  // -------------------------------------------------------------
  for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
    // Check interrupt-accumulated steps first; fallback to pin polling if zero
    int16_t delta = rotaryDecoders[i].drainSteps();
    if (delta == 0) {
      bool clkLevel = (digitalRead(ENCODER_CLK_PINS[i]) == HIGH);
      bool dtLevel  = (digitalRead(ENCODER_DT_PINS[i])  == HIGH);
      delta = rotaryDecoders[i].update(clkLevel, dtLevel);
      if (delta != 0) {
        rotaryDecoders[i].drainSteps();
      }
    }

    if (delta != 0) {
      if (displayMgr.getScreenMode() == SCREEN_PLAYER_SELECT) {
        displayMgr.adjustPlayerSelect(delta > 0 ? 1 : -1);
        continue;
      }

      if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
        displayMgr.scrollEventLog(delta > 0 ? 1 : -1);
        continue;
      }

      uint8_t targetP = getPlayerIndexForKnob(i, activePlayerCount);

      // Dispatch abstract HID action to unified controller for all accumulated steps
      int16_t steps = (delta > 0) ? delta : -delta;
      HIDAction act = (delta > 0) ? ACT_INCREMENT : ACT_DECREMENT;
      for (int16_t s = 0; s < steps; s++) {
        controller.dispatchAction(targetP, act, currentMs);
      }
      persistence.markDirty(currentMs);

      // Print live event to Serial
      GameEvent latestEvt;
      if (matchLog.getLatest(latestEvt)) {
        char logStr[64];
        EventLog::formatEvent(latestEvt, logStr, sizeof(logStr));
        Serial.println(logStr);
      }
    }

    // Read encoder switch (active-low)
    bool isSwPressed = (digitalRead(ENCODER_SW_PINS[i]) == LOW);
    ButtonEvent swEvt = rotaryButtons[i].update(isSwPressed, currentMs);

    if (swEvt == BTN_EVENT_SHORT_CLICK) {
      if (displayMgr.getScreenMode() == SCREEN_PLAYER_SELECT) {
        displayMgr.confirmPlayerSelect(currentMs);
        persistence.markDirty(currentMs);
        sysLog("[SYS] Confirmed %d Players\r\n", gameState.getPlayerCount());
        continue;
      }

      if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
        displayMgr.returnToGameScreen();
        continue;
      }

      uint8_t targetP = getPlayerIndexForKnob(i, activePlayerCount);
      controller.dispatchAction(targetP, ACT_CYCLE_MODE, currentMs);
      const PlayerState& p = gameState.getPlayer(targetP);
      if (p.activeMode == MODE_CMDR) {
        sysLog("[SYS] P%d switched mode to CMDR (Source P%d)\r\n", targetP + 1, p.cmdrSource + 1);
      } else {
        sysLog("[SYS] P%d switched mode to %s\r\n", targetP + 1, GameState::getModeName(p.activeMode));
      }
    } else if (swEvt == BTN_EVENT_DOUBLE_CLICK) {
      if (displayMgr.getScreenMode() == SCREEN_PLAYER_SELECT) {
        displayMgr.confirmPlayerSelect(currentMs);
        persistence.markDirty(currentMs);
        continue;
      }
      if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
        displayMgr.returnToGameScreen();
        continue;
      }
      uint8_t targetP = getPlayerIndexForKnob(i, activePlayerCount);
      controller.dispatchAction(targetP, ACT_RETURN_TO_LIFE, currentMs);
      sysLog("[SYS] P%d returned to LIFE mode\r\n", targetP + 1);
    } else if (swEvt == BTN_EVENT_LONG_PRESS) {
      if (displayMgr.getScreenMode() == SCREEN_PLAYER_SELECT) {
        continue;
      }
      if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
        displayMgr.returnToGameScreen();
        continue;
      }
      uint8_t targetP = getPlayerIndexForKnob(i, activePlayerCount);
      controller.dispatchAction(targetP, ACT_RESET_PLAYER, currentMs);
      persistence.markDirty(currentMs);
      sysLog("[SYS] P%d reset to %d life\r\n", targetP + 1, STARTING_LIFE);
    }
  }

  // -------------------------------------------------------------
  // 2. Process Global Table Button (Reset / Roll First / Event Log / Player Select)
  // -------------------------------------------------------------
  bool isResetPressed = (digitalRead(BTN_RESET) == LOW);
  ButtonEvent rstEvt = tableButton.update(isResetPressed, currentMs);

  if (rstEvt == BTN_EVENT_SHORT_CLICK) {
    if (displayMgr.getScreenMode() == SCREEN_PLAYER_SELECT) {
      displayMgr.confirmPlayerSelect(currentMs);
      persistence.markDirty(currentMs);
      sysLog("[SYS] Confirmed %d Players\r\n", gameState.getPlayerCount());
    } else if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
      // Exit event log view back to game screen
      displayMgr.returnToGameScreen();
    } else {
      // Start Random First Player Roll via controller
      uint8_t winner = controller.rollStartingPlayer(currentMs, random(0, 1000));
      displayMgr.startRollAnimation(winner, currentMs);

      sysLog("\r\n[SYS] *** First Player Roll: P%d Goes First! ***\r\n\r\n", winner + 1);
    }
  } else if (rstEvt == BTN_EVENT_DOUBLE_CLICK) {
    if (displayMgr.getScreenMode() != SCREEN_PLAYER_SELECT) {
      // Double click toggles Event Log screen
      displayMgr.toggleEventLogScreen();
      sysLog("[SYS] Toggled Match Event Log screen\r\n");
    }
  } else if (rstEvt == BTN_EVENT_LONG_PRESS) {
    if (displayMgr.getScreenMode() != SCREEN_PLAYER_SELECT) {
      // Full Table Game Reset via controller (1.5s hold)
      controller.dispatchAction(0, ACT_RESET_GAME, currentMs);
      displayMgr.returnToGameScreen();
      persistence.clearStorage();
      sysLog("\r\n[SYS] === FULL GAME RESET: All %d players reset to %d Life ===\r\n\r\n",
             gameState.getPlayerCount(), STARTING_LIFE);
    }
  } else if (rstEvt == BTN_EVENT_VERY_LONG_PRESS) {
    // 3.0s hold enters Player Count Setup screen
    displayMgr.openPlayerSelectScreen();
    sysLog("[SYS] Entered Player Count Setup menu (2, 3, or 4 Players)\r\n");
  }

  // -------------------------------------------------------------
  // 3. Periodic Controller & Persistence Ticks
  // -------------------------------------------------------------
  controller.tick(currentMs);
  if (persistence.tick(currentMs)) {
    sysLog("[SYS] Match state auto-saved to Flash (NVS).\r\n");
  }

  // -------------------------------------------------------------
  // 4. Render any dirty quadrants with flicker-free partial update
  // -------------------------------------------------------------
  displayMgr.updateDirtyQuadrants();

  // Small non-blocking yield for ESP32 RTOS watchdog
  delay(1);
}
