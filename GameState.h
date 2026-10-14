#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "Config.h"
#include "EventLog.h"

enum CounterMode {
    MODE_LIFE = 0,
    MODE_POISON,
    MODE_CMDR,
    MODE_TAX,
    MODE_COUNT
};

struct PlayerState {
    int16_t     life;
    int8_t      poison;
    int8_t      cmdr;
    int8_t      tax;
    CounterMode activeMode;
    uint32_t    modeTimestamp;
    bool        isDead;
    bool        dirty;
};

class GameState {
public:
    GameState(EventLog& logRef);

    // Initialize/Reset all players to default 40 life, 0 counters
    void resetAll(uint32_t currentMs);

    // Reset a single player to default state
    void resetPlayer(uint8_t playerIdx, uint32_t currentMs);

    // Apply incremental tick (+1, -1, etc.) to active counter
    bool applyDelta(uint8_t playerIdx, int16_t delta, uint32_t currentMs);

    // Cycle through modes: LIFE -> POISON -> CMDR -> TAX -> LIFE
    void cycleMode(uint8_t playerIdx, uint32_t currentMs);

    // Check if any player's temporary mode timed out, returning to LIFE
    bool checkAutoReturn(uint32_t currentMs);

    // Evaluate lethal condition for a player (life <= 0 || poison >= 10 || cmdr >= 21)
    bool updateLethal(uint8_t playerIdx);

    // Record starting player selection
    void recordFirstPlayer(uint8_t playerIdx, uint32_t currentMs);

    // Get current value of player's currently active counter
    int16_t getActiveValue(uint8_t playerIdx) const;

    // Const access to player state
    const PlayerState& getPlayer(uint8_t playerIdx) const;

    // Clear dirty flag after quadrant has been redrawn
    void clearDirty(uint8_t playerIdx);

    // Name of active mode: "LIFE", "POISON", "CMDR", "TAX"
    static const char* getModeName(CounterMode mode);

    EventLog& log;

private:
    PlayerState players[NUM_PLAYERS];
};

#endif // GAME_STATE_H

