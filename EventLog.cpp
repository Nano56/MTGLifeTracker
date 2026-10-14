#include "EventLog.h"
#include <stdio.h>
#include <string.h>

EventLog::EventLog() {
    clear();
}

void EventLog::clear() {
    head = 0;
    count = 0;
    totalLogged = 0;
    memset(buffer, 0, sizeof(buffer));
}

void EventLog::log(uint32_t timestampMs, uint8_t player, uint8_t type, int16_t delta, int16_t newValue,
                   uint8_t source, int16_t secondaryValue) {
    buffer[head].timestampMs = timestampMs;
    buffer[head].player = player;
    buffer[head].type = type;
    buffer[head].source = source;
    buffer[head].delta = delta;
    buffer[head].newValue = newValue;
    buffer[head].secondaryValue = secondaryValue;

    head = (head + 1) % EVENT_LOG_CAPACITY;
    if (count < EVENT_LOG_CAPACITY) {
        count++;
    }
    totalLogged++;
}

uint16_t EventLog::getCount() const {
    return count;
}

uint32_t EventLog::getTotalCount() const {
    return totalLogged;
}

bool EventLog::getEvent(uint16_t index, GameEvent& outEvent) const {
    if (index >= count) {
        return false;
    }
    // Calculate oldest entry index
    uint16_t oldestIndex;
    if (count < EVENT_LOG_CAPACITY) {
        oldestIndex = 0;
    } else {
        oldestIndex = head; // In full circular buffer, head points to the oldest overwritten entry
    }

    uint16_t actualIndex = (oldestIndex + index) % EVENT_LOG_CAPACITY;
    outEvent = buffer[actualIndex];
    return true;
}

bool EventLog::getLatest(GameEvent& outEvent) const {
    if (count == 0) {
        return false;
    }
    return getEvent(count - 1, outEvent);
}

void EventLog::formatEvent(const GameEvent& evt, char* outBuffer, size_t maxLen) {
    if (!outBuffer || maxLen == 0) return;

    uint32_t totalSeconds = evt.timestampMs / 1000;
    uint32_t minutes = totalSeconds / 60;
    uint32_t seconds = totalSeconds % 60;

    char playerStr[16];
    if (evt.player == SYSTEM_PLAYER_ID) {
        snprintf(playerStr, sizeof(playerStr), "SYS");
    } else {
        snprintf(playerStr, sizeof(playerStr), "P%d", evt.player + 1);
    }

    switch (evt.type) {
        case EVT_LIFE_CHANGE:
            snprintf(outBuffer, maxLen, "[%02u:%02u] %s: Life %+d (%d)", 
                     (unsigned)minutes, (unsigned)seconds, playerStr, evt.delta, evt.newValue);
            break;
        case EVT_POISON_CHANGE:
            snprintf(outBuffer, maxLen, "[%02u:%02u] %s: Poison %+d (%d)", 
                     (unsigned)minutes, (unsigned)seconds, playerStr, evt.delta, evt.newValue);
            break;
        case EVT_CMDR_CHANGE:
            if (evt.source != SYSTEM_PLAYER_ID) {
                snprintf(outBuffer, maxLen, "[%02u:%02u] %s: Cmdr P%d %+d (%02d) | Life %d", 
                         (unsigned)minutes, (unsigned)seconds, playerStr, evt.source + 1, evt.delta, evt.newValue, evt.secondaryValue);
            } else {
                snprintf(outBuffer, maxLen, "[%02u:%02u] %s: Cmdr %+d (%d)", 
                         (unsigned)minutes, (unsigned)seconds, playerStr, evt.delta, evt.newValue);
            }
            break;
        case EVT_TAX_CHANGE:
            snprintf(outBuffer, maxLen, "[%02u:%02u] %s: Tax %+d (%d)", 
                     (unsigned)minutes, (unsigned)seconds, playerStr, evt.delta, evt.newValue);
            break;
        case EVT_GAME_RESET:
            snprintf(outBuffer, maxLen, "[%02u:%02u] SYS: Game Reset", 
                     (unsigned)minutes, (unsigned)seconds);
            break;
        case EVT_ROLL_FIRST:
            snprintf(outBuffer, maxLen, "[%02u:%02u] SYS: P%d Goes First", 
                     (unsigned)minutes, (unsigned)seconds, evt.player + 1);
            break;
        case EVT_PLAYER_RESET:
            snprintf(outBuffer, maxLen, "[%02u:%02u] %s: Reset to 40 Life", 
                     (unsigned)minutes, (unsigned)seconds, playerStr);
            break;
        default:
            snprintf(outBuffer, maxLen, "[%02u:%02u] %s: Event", 
                     (unsigned)minutes, (unsigned)seconds, playerStr);
            break;
    }
}
