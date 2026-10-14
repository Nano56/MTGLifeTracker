#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "Config.h"
#include "EventLog.h"

enum CounterMode {
    MODE_LIFE = 0,
    MODE_CMDR,
    MODE_POISON,
    MODE_TAX,
    MODE_COUNT
};

// Bitflags for death reasons in deadReasonMask:
#define DEAD_REASON_LIFE        0x01
#define DEAD_REASON_POISON      0x02
#define DEAD_REASON_CMDR_P1     0x04
#define DEAD_REASON_CMDR_P2     0x08
#define DEAD_REASON_CMDR_P3     0x10
#define DEAD_REASON_CMDR_P4     0x20

struct PlayerState {
    int16_t     life;
    int8_t      poison;
    uint8_t     cmdrDmg[NUM_PLAYERS]; // Damage taken from each player's commander (own index unused)
    int8_t      tax;
    CounterMode activeMode;
    uint8_t     cmdrSource;           // 0..3: which opponent commander is selected when in MODE_CMDR
    uint32_t    modeTimestamp;
    bool        isDead;
    uint8_t     deadReasonMask;       // Bitmask of lethal conditions
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

    // Cycle through modes: LIFE -> CMDR (opponents) -> POISON -> TAX -> LIFE
    void cycleMode(uint8_t playerIdx, uint32_t currentMs);

    // Immediately return player to LIFE mode
    void returnToLife(uint8_t playerIdx, uint32_t currentMs);

    // Check if any player's temporary mode timed out, returning to LIFE
    bool checkAutoReturn(uint32_t currentMs);

    // Evaluate lethal condition for a player (life <= 0 || poison >= 10 || any cmdrDmg >= 21)
    bool updateLethal(uint8_t playerIdx);

    // Record starting player selection
    void recordFirstPlayer(uint8_t playerIdx, uint32_t currentMs);

    // Get current value of player's currently active counter
    int16_t getActiveValue(uint8_t playerIdx) const;

    // Const access to player state
    const PlayerState& getPlayer(uint8_t playerIdx) const;

    // Clear dirty flag after quadrant has been redrawn
    void clearDirty(uint8_t playerIdx);

    // Name of active mode: "LIFE", "CMDR", "POISON", "TAX"
    static const char* getModeName(CounterMode mode);

    EventLog& log;

private:
    PlayerState players[NUM_PLAYERS];

    uint8_t getFirstOpponent(uint8_t playerIdx) const;
    uint8_t getNextOpponent(uint8_t playerIdx, uint8_t currentSource) const;
};

#endif // GAME_STATE_H
