#include "AppController.h"

AppController::AppController(GameState& stateRef, EventLog& logRef)
    : state(stateRef), log(logRef) {
}

bool AppController::dispatchAction(uint8_t playerIdx, HIDAction action, uint32_t currentMs) {
    switch (action) {
        case ACT_INCREMENT:
            return state.applyDelta(playerIdx, 1, currentMs);

        case ACT_DECREMENT:
            return state.applyDelta(playerIdx, -1, currentMs);

        case ACT_CYCLE_MODE:
            state.cycleMode(playerIdx, currentMs);
            return true;

        case ACT_RESET_PLAYER:
            state.resetPlayer(playerIdx, currentMs);
            return true;

        case ACT_RESET_GAME:
            state.resetAll(currentMs);
            return true;

        case ACT_ROLL_FIRST: {
            // Seeded or simulated roll
            uint8_t winner = currentMs % NUM_PLAYERS;
            state.recordFirstPlayer(winner, currentMs);
            return true;
        }

        default:
            return false;
    }
}

void AppController::tick(uint32_t currentMs) {
    state.checkAutoReturn(currentMs);
}

uint8_t AppController::rollStartingPlayer(uint32_t currentMs, uint32_t seed) {
    uint8_t winner = (seed + currentMs) % NUM_PLAYERS;
    state.recordFirstPlayer(winner, currentMs);
    return winner;
}

