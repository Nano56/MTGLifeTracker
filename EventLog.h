#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define EVENT_LOG_CAPACITY 50
#define SYSTEM_PLAYER_ID   255

enum EventType {
    EVT_LIFE_CHANGE = 0,
    EVT_POISON_CHANGE,
    EVT_CMDR_CHANGE,
    EVT_TAX_CHANGE,
    EVT_GAME_RESET,
    EVT_ROLL_FIRST,
    EVT_PLAYER_RESET
};

struct GameEvent {
    uint32_t timestampMs;
    uint8_t  player;          // 0..3 for P1..P4, or SYSTEM_PLAYER_ID
    uint8_t  type;            // EventType
    uint8_t  source;          // 0..3 for opponent commander source, or SYSTEM_PLAYER_ID
    int16_t  delta;           // +1, -1, +2, etc.
    int16_t  newValue;        // Resulting value of primary counter
    int16_t  secondaryValue;  // Resulting life total (for commander damage events)
};

class EventLog {
public:
    EventLog();

    // Log a new event into the circular ring buffer
    void log(uint32_t timestampMs, uint8_t player, uint8_t type, int16_t delta, int16_t newValue,
             uint8_t source = SYSTEM_PLAYER_ID, int16_t secondaryValue = 0);

    // Number of active events currently stored (0..EVENT_LOG_CAPACITY)
    uint16_t getCount() const;

    // Total events ever logged in this match
    uint32_t getTotalCount() const;

    // Get event by chronological relative index (0 = oldest available, getCount()-1 = newest)
    bool getEvent(uint16_t index, GameEvent& outEvent) const;

    // Get the most recent event (index getCount()-1)
    bool getLatest(GameEvent& outEvent) const;

    // Format an event into human-readable string: "[MM:SS] P3: Cmdr P1 +3 (06) | Life 31"
    static void formatEvent(const GameEvent& evt, char* outBuffer, size_t maxLen);

    // Clear buffer (e.g. on full game reset)
    void clear();

private:
    GameEvent buffer[EVENT_LOG_CAPACITY];
    uint16_t head;
    uint16_t count;
    uint32_t totalLogged;
};

#endif // EVENT_LOG_H
