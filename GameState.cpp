#include "GameState.h"

GameState::GameState(EventLog& logRef) : log(logRef) {
    resetAll(0);
}

void GameState::resetAll(uint32_t currentMs) {
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        resetPlayer(i, currentMs);
    }
    log.log(currentMs, SYSTEM_PLAYER_ID, EVT_GAME_RESET, 0, 0);
}

void GameState::resetPlayer(uint8_t playerIdx, uint32_t currentMs) {
    if (playerIdx >= NUM_PLAYERS) return;

    players[playerIdx].life = STARTING_LIFE;
    players[playerIdx].poison = 0;
    players[playerIdx].cmdr = STARTING_CMDR_HEALTH;
    players[playerIdx].tax = 0;
    players[playerIdx].activeMode = MODE_LIFE;
    players[playerIdx].modeTimestamp = currentMs;
    players[playerIdx].isDead = false;
    players[playerIdx].dirty = true;
}

bool GameState::applyDelta(uint8_t playerIdx, int16_t delta, uint32_t currentMs) {
    if (playerIdx >= NUM_PLAYERS || delta == 0) return false;

    PlayerState& p = players[playerIdx];
    p.modeTimestamp = currentMs;
    p.dirty = true;

    int16_t actualDelta = delta;
    int16_t newVal = 0;
    uint8_t evtType = EVT_LIFE_CHANGE;

    switch (p.activeMode) {
        case MODE_LIFE:
            p.life += delta;
            newVal = p.life;
            evtType = EVT_LIFE_CHANGE;
            break;

        case MODE_POISON:
            if (p.poison + delta < 0) {
                actualDelta = -p.poison;
                p.poison = 0;
            } else {
                p.poison += delta;
            }
            newVal = p.poison;
            evtType = EVT_POISON_CHANGE;
            break;

        case MODE_CMDR:
            p.cmdr += delta;
            newVal = p.cmdr;
            evtType = EVT_CMDR_CHANGE;
            break;

        case MODE_TAX:
            actualDelta = delta * CMDR_TAX_STEP;
            if (p.tax + actualDelta < 0) {
                actualDelta = -p.tax;
                p.tax = 0;
            } else {
                p.tax += actualDelta;
            }
            newVal = p.tax;
            evtType = EVT_TAX_CHANGE;
            break;

        default:
            return false;
    }

    updateLethal(playerIdx);
    log.log(currentMs, playerIdx, evtType, actualDelta, newVal);
    return true;
}

void GameState::cycleMode(uint8_t playerIdx, uint32_t currentMs) {
    if (playerIdx >= NUM_PLAYERS) return;

    PlayerState& p = players[playerIdx];
    p.activeMode = (CounterMode)((p.activeMode + 1) % MODE_COUNT);
    p.modeTimestamp = currentMs;
    p.dirty = true;
}

bool GameState::checkAutoReturn(uint32_t currentMs) {
#if AUTO_RETURN_LIFE_MS > 0
    bool anyChanged = false;
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        PlayerState& p = players[i];
        if (p.activeMode != MODE_LIFE) {
            if (currentMs - p.modeTimestamp >= AUTO_RETURN_LIFE_MS) {
                p.activeMode = MODE_LIFE;
                p.dirty = true;
                anyChanged = true;
            }
        }
    }
    return anyChanged;
#else
    (void)currentMs;
    return false;
#endif
}

bool GameState::updateLethal(uint8_t playerIdx) {
    if (playerIdx >= NUM_PLAYERS) return false;

    PlayerState& p = players[playerIdx];
    bool previouslyDead = p.isDead;
    p.isDead = (p.life <= 0 || p.poison >= LETHAL_POISON || p.cmdr <= 0);

    if (p.isDead != previouslyDead) {
        p.dirty = true;
    }
    return p.isDead;
}

void GameState::recordFirstPlayer(uint8_t playerIdx, uint32_t currentMs) {
    if (playerIdx < NUM_PLAYERS) {
        log.log(currentMs, playerIdx, EVT_ROLL_FIRST, 0, 0);
    }
}

int16_t GameState::getActiveValue(uint8_t playerIdx) const {
    if (playerIdx >= NUM_PLAYERS) return 0;

    const PlayerState& p = players[playerIdx];
    switch (p.activeMode) {
        case MODE_LIFE:   return p.life;
        case MODE_POISON: return p.poison;
        case MODE_CMDR:   return p.cmdr;
        case MODE_TAX:    return p.tax;
        default:          return 0;
    }
}

const PlayerState& GameState::getPlayer(uint8_t playerIdx) const {
    return players[playerIdx];
}

void GameState::clearDirty(uint8_t playerIdx) {
    if (playerIdx < NUM_PLAYERS) {
        players[playerIdx].dirty = false;
    }
}

const char* GameState::getModeName(CounterMode mode) {
    switch (mode) {
        case MODE_LIFE:   return "LIFE";
        case MODE_POISON: return "POISON";
        case MODE_CMDR:   return "CMDR";
        case MODE_TAX:    return "TAX";
        default:          return "";
    }
}

