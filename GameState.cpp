#include "GameState.h"
#include <string.h>

GameState::GameState(EventLog& logRef) : log(logRef) {
    resetAll(0);
}

uint8_t GameState::getFirstOpponent(uint8_t playerIdx) const {
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        if (i != playerIdx) return i;
    }
    return 0;
}

uint8_t GameState::getNextOpponent(uint8_t playerIdx, uint8_t currentSource) const {
    for (uint8_t i = currentSource + 1; i < NUM_PLAYERS; i++) {
        if (i != playerIdx) return i;
    }
    return 255; // Sentinel: no more opponents
}

void GameState::resetAll(uint32_t currentMs) {
    log.clear();
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        if (i >= NUM_PLAYERS) return;

        PlayerState& p = players[i];
        p.life = STARTING_LIFE;
        p.poison = 0;
        for (uint8_t j = 0; j < NUM_PLAYERS; j++) {
            p.cmdrDmg[j] = 0;
        }
        p.tax = 0;
        p.activeMode = MODE_LIFE;
        p.cmdrSource = getFirstOpponent(i);
        p.modeTimestamp = currentMs;
        p.isDead = false;
        p.deadReasonMask = 0;
        p.dirty = true;
    }
    log.log(currentMs, SYSTEM_PLAYER_ID, EVT_GAME_RESET, 0, 0);
}

void GameState::resetPlayer(uint8_t playerIdx, uint32_t currentMs) {
    if (playerIdx >= NUM_PLAYERS) return;

    PlayerState& p = players[playerIdx];
    p.life = STARTING_LIFE;
    p.poison = 0;
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        p.cmdrDmg[i] = 0;
    }
    p.tax = 0;
    p.activeMode = MODE_LIFE;
    p.cmdrSource = getFirstOpponent(playerIdx);
    p.modeTimestamp = currentMs;
    p.isDead = false;
    p.deadReasonMask = 0;
    p.dirty = true;

    log.log(currentMs, playerIdx, EVT_PLAYER_RESET, 0, STARTING_LIFE);
}

bool GameState::applyDelta(uint8_t playerIdx, int16_t delta, uint32_t currentMs) {
    if (playerIdx >= NUM_PLAYERS || delta == 0) return false;

    PlayerState& p = players[playerIdx];
    p.modeTimestamp = currentMs;

    int16_t actualDelta = delta;
    int16_t newVal = 0;
    uint8_t evtType = EVT_LIFE_CHANGE;
    uint8_t source = SYSTEM_PLAYER_ID;
    int16_t secVal = 0;

    switch (p.activeMode) {
        case MODE_LIFE: {
            int32_t calcLife = p.life + delta;
            if (calcLife < MIN_LIFE) calcLife = MIN_LIFE;
            if (calcLife > MAX_LIFE) calcLife = MAX_LIFE;
            actualDelta = (int16_t)(calcLife - p.life);
            if (actualDelta == 0) return false;
            p.life = (int16_t)calcLife;
            newVal = p.life;
            evtType = EVT_LIFE_CHANGE;
            break;
        }

        case MODE_POISON: {
            int16_t calcPoison = p.poison + delta;
            if (calcPoison < 0) calcPoison = 0;
            if (calcPoison > MAX_POISON) calcPoison = MAX_POISON;
            actualDelta = calcPoison - p.poison;
            if (actualDelta == 0) return false;
            p.poison = (int8_t)calcPoison;
            newVal = p.poison;
            evtType = EVT_POISON_CHANGE;
            break;
        }

        case MODE_CMDR: {
            uint8_t src = p.cmdrSource;
            if (src >= NUM_PLAYERS || src == playerIdx) {
                src = getFirstOpponent(playerIdx);
                p.cmdrSource = src;
            }
            int16_t calcCmdr = p.cmdrDmg[src] + delta;
            if (calcCmdr < 0) calcCmdr = 0;
            if (calcCmdr > MAX_CMDR_DAMAGE) calcCmdr = MAX_CMDR_DAMAGE;
            actualDelta = calcCmdr - p.cmdrDmg[src];
            if (actualDelta == 0) return false;

            p.cmdrDmg[src] = (uint8_t)calcCmdr;
            newVal = p.cmdrDmg[src];

            // CR 903.10a / 704.6c: Commander damage is combat damage, reduces life in tandem
            int32_t calcLife = p.life - actualDelta;
            if (calcLife < MIN_LIFE) calcLife = MIN_LIFE;
            if (calcLife > MAX_LIFE) calcLife = MAX_LIFE;
            p.life = (int16_t)calcLife;

            source = src;
            secVal = p.life;
            evtType = EVT_CMDR_CHANGE;
            break;
        }

        case MODE_TAX: {
            actualDelta = delta * CMDR_TAX_STEP;
            int16_t calcTax = p.tax + actualDelta;
            if (calcTax < 0) calcTax = 0;
            if (calcTax > MAX_TAX) calcTax = MAX_TAX;
            actualDelta = calcTax - p.tax;
            if (actualDelta == 0) return false;
            p.tax = (int8_t)calcTax;
            newVal = p.tax;
            evtType = EVT_TAX_CHANGE;
            break;
        }

        default:
            return false;
    }

    p.dirty = true;
    updateLethal(playerIdx);
    log.log(currentMs, playerIdx, evtType, actualDelta, newVal, source, secVal);
    return true;
}

void GameState::cycleMode(uint8_t playerIdx, uint32_t currentMs) {
    if (playerIdx >= NUM_PLAYERS) return;

    PlayerState& p = players[playerIdx];
    p.modeTimestamp = currentMs;
    p.dirty = true;

    switch (p.activeMode) {
        case MODE_LIFE:
            p.activeMode = MODE_CMDR;
            p.cmdrSource = getFirstOpponent(playerIdx);
            break;

        case MODE_CMDR: {
            uint8_t nextOpp = getNextOpponent(playerIdx, p.cmdrSource);
            if (nextOpp != 255) {
                p.cmdrSource = nextOpp;
            } else {
                p.activeMode = MODE_POISON;
            }
            break;
        }

        case MODE_POISON:
            p.activeMode = MODE_TAX;
            break;

        case MODE_TAX:
            p.activeMode = MODE_LIFE;
            break;

        default:
            p.activeMode = MODE_LIFE;
            break;
    }
}

void GameState::returnToLife(uint8_t playerIdx, uint32_t currentMs) {
    if (playerIdx >= NUM_PLAYERS) return;

    PlayerState& p = players[playerIdx];
    if (p.activeMode != MODE_LIFE) {
        p.activeMode = MODE_LIFE;
        p.modeTimestamp = currentMs;
        p.dirty = true;
    }
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
    p.deadReasonMask = 0;

    if (p.life <= 0) {
        p.deadReasonMask |= DEAD_REASON_LIFE;
    }
    if (p.poison >= LETHAL_POISON) {
        p.deadReasonMask |= DEAD_REASON_POISON;
    }
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        if (i != playerIdx && p.cmdrDmg[i] >= LETHAL_CMDR_DAMAGE) {
            p.deadReasonMask |= (DEAD_REASON_CMDR_P1 << i);
        }
    }

    p.isDead = (p.deadReasonMask != 0);
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
        case MODE_CMDR: {
            uint8_t src = p.cmdrSource;
            if (src < NUM_PLAYERS) return p.cmdrDmg[src];
            return 0;
        }
        case MODE_TAX:    return p.tax;
        default:          return 0;
    }
}

const PlayerState& GameState::getPlayer(uint8_t playerIdx) const {
    if (playerIdx >= NUM_PLAYERS) {
        return players[0];
    }
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
        case MODE_CMDR:   return "CMDR";
        case MODE_POISON: return "POISON";
        case MODE_TAX:    return "TAX";
        default:          return "";
    }
}
