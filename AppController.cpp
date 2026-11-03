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

        case ACT_RETURN_TO_LIFE:
            state.returnToLife(playerIdx, currentMs);
            return true;

        case ACT_RESET_PLAYER:
            state.resetPlayer(playerIdx, currentMs);
            return true;

        case ACT_RESET_GAME:
            state.resetAll(currentMs);
            return true;

        case ACT_ROLL_FIRST: {
            rollStartingPlayer(currentMs, currentMs * 2654435761u);
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
    uint8_t count = state.getPlayerCount();
    if (count == 0) count = NUM_PLAYERS;
    // High-quality integer hash mixing to guarantee fair distribution across all active players
    uint32_t h = seed ^ (currentMs * 0x9E3779B9u);
    h ^= (h >> 16);
    h *= 0x85EBCA6Bu;
    h ^= (h >> 13);
    h *= 0xC2B2AE35u;
    h ^= (h >> 16);
    uint8_t winner = (uint8_t)(h % count);
    state.recordFirstPlayer(winner, currentMs);
    return winner;
}

void AppController::setPlayerCount(uint8_t count, uint32_t currentMs) {
    state.setPlayerCount(count, currentMs);
}
