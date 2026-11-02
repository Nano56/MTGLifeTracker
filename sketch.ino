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

// Hardware display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Core business logic, HID controller & state
EventLog matchLog;
GameState gameState(matchLog);
AppController controller(gameState, matchLog);
DisplayManager displayMgr(tft, gameState, matchLog);

// Encoder hardware pin mapping
const uint8_t ENCODER_CLK_PINS[NUM_PLAYERS] = { P1_CLK, P2_CLK, P3_CLK, P4_CLK };
const uint8_t ENCODER_DT_PINS[NUM_PLAYERS]  = { P1_DT,  P2_DT,  P3_DT,  P4_DT  };
const uint8_t ENCODER_SW_PINS[NUM_PLAYERS]  = { P1_SW,  P2_SW,  P3_SW,  P4_SW  };

// Input state decoders
RotaryDecoder   rotaryDecoders[NUM_PLAYERS];
ButtonDebouncer rotaryButtons[NUM_PLAYERS];
ButtonDebouncer tableButton;

void setup() {
  Serial.begin(115200);
  delay(150);
  
  Serial.println("\n==================================================");
  Serial.println("  MTG COMMANDER LIFE TRACKER - FIRMWARE v1.0");
  Serial.println("==================================================");

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

  // Configure table reset/roll button (1500 ms hold for reset, 3000 ms hold for Player Count menu)
  pinMode(BTN_RESET, INPUT_PULLUP);
  tableButton.setLongPressMs(TABLE_RESET_LONG_PRESS_MS);
  tableButton.setVeryLongPressMs(TABLE_MENU_LONG_PRESS_MS);

  // Seed random generator with analog noise on unconnected pin
  randomSeed(analogRead(36) + esp_random());

  // Initialize display and render starting 4-player layout
  displayMgr.begin();

  Serial.printf("[SYS] Ready. %d Players @ %d Life.\n", gameState.getPlayerCount(), STARTING_LIFE);
  Serial.println("[SYS] Turn knob: +/- Counter | Click knob: Cycle Mode | Double-click: Return to Life");
  Serial.println("[SYS] Hold knob (3s): Reset Player | Red Button: Click to Roll | Double-click: Event Log");
  Serial.println("[SYS] Hold Red Button (1.5s): Reset Game | Hold Red Button (3s): Player Count Setup (2-4P)\n");
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
  uint32_t currentMs = millis();

  // If a first-player roll animation is running, process its frame
  if (displayMgr.getScreenMode() == SCREEN_ROLL_ANIMATION) {
    displayMgr.updateAnimation(currentMs);
    return;
  }

  uint8_t activePlayerCount = gameState.getPlayerCount();

  // -------------------------------------------------------------
  // 1. Process 4 Rotary Encoders (Rotation & Switch)
  // -------------------------------------------------------------
  for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
    // Read quadrature pin levels
    bool clkLevel = (digitalRead(ENCODER_CLK_PINS[i]) == HIGH);
    bool dtLevel  = (digitalRead(ENCODER_DT_PINS[i])  == HIGH);

    int8_t delta = rotaryDecoders[i].update(clkLevel, dtLevel);
    if (delta != 0) {
      if (displayMgr.getScreenMode() == SCREEN_PLAYER_SELECT) {
        displayMgr.adjustPlayerSelect(delta > 0 ? 1 : -1);
        continue;
      }

      if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
        displayMgr.returnToGameScreen();
      }

      uint8_t targetP = getPlayerIndexForKnob(i, activePlayerCount);

      // Dispatch abstract HID action to unified controller
      HIDAction act = (delta > 0) ? ACT_INCREMENT : ACT_DECREMENT;
      controller.dispatchAction(targetP, act, currentMs);

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
        Serial.printf("[SYS] Confirmed %d Players\n", gameState.getPlayerCount());
        continue;
      }

      if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
        displayMgr.returnToGameScreen();
      }

      uint8_t targetP = getPlayerIndexForKnob(i, activePlayerCount);
      controller.dispatchAction(targetP, ACT_CYCLE_MODE, currentMs);
      const PlayerState& p = gameState.getPlayer(targetP);
      if (p.activeMode == MODE_CMDR) {
        Serial.printf("[SYS] P%d switched mode to CMDR (Source P%d)\n", targetP + 1, p.cmdrSource + 1);
      } else {
        Serial.printf("[SYS] P%d switched mode to %s\n", targetP + 1, GameState::getModeName(p.activeMode));
      }
    } else if (swEvt == BTN_EVENT_DOUBLE_CLICK) {
      if (displayMgr.getScreenMode() == SCREEN_PLAYER_SELECT) {
        displayMgr.confirmPlayerSelect(currentMs);
        continue;
      }
      if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
        displayMgr.returnToGameScreen();
      }
      uint8_t targetP = getPlayerIndexForKnob(i, activePlayerCount);
      controller.dispatchAction(targetP, ACT_RETURN_TO_LIFE, currentMs);
      Serial.printf("[SYS] P%d returned to LIFE mode\n", targetP + 1);
    } else if (swEvt == BTN_EVENT_LONG_PRESS) {
      if (displayMgr.getScreenMode() == SCREEN_PLAYER_SELECT) {
        continue;
      }
      if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
        displayMgr.returnToGameScreen();
      }
      uint8_t targetP = getPlayerIndexForKnob(i, activePlayerCount);
      controller.dispatchAction(targetP, ACT_RESET_PLAYER, currentMs);
      Serial.printf("[SYS] P%d reset to %d life\n", targetP + 1, STARTING_LIFE);
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
      Serial.printf("[SYS] Confirmed %d Players\n", gameState.getPlayerCount());
    } else if (displayMgr.getScreenMode() == SCREEN_EVENT_LOG) {
      // Exit event log view back to game screen
      displayMgr.returnToGameScreen();
    } else {
      // Start Random First Player Roll via controller
      uint8_t winner = controller.rollStartingPlayer(currentMs, random(0, 1000));
      displayMgr.startRollAnimation(winner, currentMs);

      Serial.printf("\n[SYS] *** First Player Roll: P%d Goes First! ***\n\n", winner + 1);
    }
  } else if (rstEvt == BTN_EVENT_DOUBLE_CLICK) {
    if (displayMgr.getScreenMode() != SCREEN_PLAYER_SELECT) {
      // Double click toggles Event Log screen
      displayMgr.toggleEventLogScreen();
      Serial.println("[SYS] Toggled Match Event Log screen");
    }
  } else if (rstEvt == BTN_EVENT_LONG_PRESS) {
    if (displayMgr.getScreenMode() != SCREEN_PLAYER_SELECT) {
      // Full Table Game Reset via controller (1.5s hold)
      controller.dispatchAction(0, ACT_RESET_GAME, currentMs);
      displayMgr.returnToGameScreen();
      Serial.printf("\n[SYS] === FULL GAME RESET: All %d players reset to %d Life ===\n\n",
                    gameState.getPlayerCount(), STARTING_LIFE);
    }
  } else if (rstEvt == BTN_EVENT_VERY_LONG_PRESS) {
    // 3.0s hold enters Player Count Setup screen
    displayMgr.openPlayerSelectScreen();
    Serial.println("[SYS] Entered Player Count Setup menu (2, 3, or 4 Players)");
  }

  // -------------------------------------------------------------
  // 3. Periodic Controller Tick
  // -------------------------------------------------------------
  controller.tick(currentMs);

  // -------------------------------------------------------------
  // 4. Render any dirty quadrants with flicker-free partial update
  // -------------------------------------------------------------
  displayMgr.updateDirtyQuadrants();

  // Small non-blocking yield for ESP32 RTOS watchdog
  delay(1);
}
