#include "StorageManager.h"
#include <string.h>

#if defined(ESP32) || defined(ARDUINO)
#include <Preferences.h>
static Preferences prefs;
#else
// In-memory mock storage for native host unit tests
#include <map>
#include <string>
#include <vector>
static std::map<std::string, std::vector<uint8_t>> mockStorage;
#endif

StorageManager::StorageManager(GameState& stateRef, EventLog& logRef)
    : state(stateRef), log(logRef), dirty(false), lastDirtyMs(0), initialized(false) {
}

bool StorageManager::begin() {
#if defined(ESP32) || defined(ARDUINO)
    initialized = prefs.begin(NVS_DEFAULT_NAMESPACE, false);
    return initialized;
#else
    initialized = true;
    return true;
#endif
}

uint32_t StorageManager::computeChecksum(const uint8_t* data, size_t length) {
    uint32_t c0 = 0xFFFF;
    uint32_t c1 = 0xFFFF;
    const uint16_t* ptr = (const uint16_t*)data;
    size_t words = length / 2;

    while (words) {
        size_t block = (words >= 359) ? 359 : words;
        words -= block;
        do {
            c0 += *ptr++;
            c1 += c0;
        } while (--block);
        c0 = (c0 & 0xFFFF) + (c0 >> 16);
        c1 = (c1 & 0xFFFF) + (c1 >> 16);
    }

    if (length & 1) {
        c0 += *(const uint8_t*)ptr;
        c1 += c0;
        c0 = (c0 & 0xFFFF) + (c0 >> 16);
        c1 = (c1 & 0xFFFF) + (c1 >> 16);
    }

    c0 = (c0 & 0xFFFF) + (c0 >> 16);
    c1 = (c1 & 0xFFFF) + (c1 >> 16);
    return (c1 << 16) | c0;
}

bool StorageManager::hasSavedMatch() {
    if (!initialized && !begin()) {
        return false;
    }

    SerializedGameState savedState;
    size_t actualLen = 0;
    if (!readBlob(NVS_KEY_GAME_STATE, &savedState, sizeof(savedState), actualLen)) {
        return false;
    }

    if (actualLen != sizeof(savedState)) {
        return false;
    }

    if (savedState.magic != NVS_MAGIC_HEADER || savedState.version != NVS_FORMAT_VERSION) {
        return false;
    }

    if (savedState.playerCount < MIN_PLAYERS || savedState.playerCount > MAX_PLAYERS) {
        return false;
    }

    uint32_t expectedCrc = computeChecksum((const uint8_t*)&savedState, offsetof(SerializedGameState, checksum));
    return (savedState.checksum == expectedCrc);
}

bool StorageManager::saveMatch() {
    if (!initialized && !begin()) {
        return false;
    }

    // 1. Serialize GameState
    SerializedGameState savedState;
    memset(&savedState, 0, sizeof(savedState));
    savedState.magic = NVS_MAGIC_HEADER;
    savedState.version = NVS_FORMAT_VERSION;
    savedState.playerCount = state.getPlayerCount();

    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        const PlayerState& p = state.getPlayer(i);
        savedState.players[i].life = p.life;
        savedState.players[i].poison = p.poison;
        savedState.players[i].tax = p.tax;
        for (uint8_t j = 0; j < NUM_PLAYERS; j++) {
            savedState.players[i].cmdrDmg[j] = p.cmdrDmg[j];
        }
    }

    savedState.checksum = computeChecksum((const uint8_t*)&savedState, offsetof(SerializedGameState, checksum));

    if (!writeBlob(NVS_KEY_GAME_STATE, &savedState, sizeof(savedState))) {
        return false;
    }

    // 2. Serialize EventLog
    SerializedEventLog savedLog;
    memset(&savedLog, 0, sizeof(savedLog));
    savedLog.magic = NVS_MAGIC_HEADER;
    savedLog.version = NVS_FORMAT_VERSION;

    uint16_t logCount = log.getCount();
    if (logCount > EVENT_LOG_CAPACITY) {
        logCount = EVENT_LOG_CAPACITY;
    }
    savedLog.eventCount = (uint8_t)logCount;

    for (uint16_t i = 0; i < logCount; i++) {
        GameEvent evt;
        if (log.getEvent(i, evt)) {
            savedLog.events[i].timestampMs = evt.timestampMs;
            savedLog.events[i].player = evt.player;
            savedLog.events[i].type = evt.type;
            savedLog.events[i].source = evt.source;
            savedLog.events[i].delta = evt.delta;
            savedLog.events[i].newValue = evt.newValue;
            savedLog.events[i].secondaryValue = evt.secondaryValue;
        }
    }

    savedLog.checksum = computeChecksum((const uint8_t*)&savedLog, offsetof(SerializedEventLog, checksum));

    writeBlob(NVS_KEY_EVENT_LOG, &savedLog, sizeof(savedLog));

    dirty = false;
    return true;
}

bool StorageManager::loadMatch() {
    if (!hasSavedMatch()) {
        return false;
    }

    SerializedGameState savedState;
    size_t actualLen = 0;
    if (!readBlob(NVS_KEY_GAME_STATE, &savedState, sizeof(savedState), actualLen) || actualLen != sizeof(savedState)) {
        return false;
    }

    // Apply Player Count first
    state.setPlayerCount(savedState.playerCount, 0);

    // Apply Player State
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        state.restorePlayerState(i,
                                 savedState.players[i].life,
                                 savedState.players[i].poison,
                                 savedState.players[i].tax,
                                 savedState.players[i].cmdrDmg);
    }

    // Restore EventLog if present and valid
    SerializedEventLog savedLog;
    if (readBlob(NVS_KEY_EVENT_LOG, &savedLog, sizeof(savedLog), actualLen) && actualLen == sizeof(savedLog)) {
        if (savedLog.magic == NVS_MAGIC_HEADER && savedLog.version == NVS_FORMAT_VERSION) {
            uint32_t expectedCrc = computeChecksum((const uint8_t*)&savedLog, offsetof(SerializedEventLog, checksum));
            if (savedLog.checksum == expectedCrc) {
                log.clear();
                for (uint8_t i = 0; i < savedLog.eventCount; i++) {
                    const SerializedEvent& e = savedLog.events[i];
                    log.log(e.timestampMs, e.player, e.type, e.delta, e.newValue, e.source, e.secondaryValue);
                }
            }
        }
    }

    dirty = false;
    return true;
}

void StorageManager::clearStorage() {
    if (!initialized && !begin()) {
        return;
    }
    eraseKey(NVS_KEY_GAME_STATE);
    eraseKey(NVS_KEY_EVENT_LOG);
    dirty = false;
}

void StorageManager::markDirty(uint32_t currentMs) {
    dirty = true;
    lastDirtyMs = currentMs;
}

bool StorageManager::tick(uint32_t currentMs) {
    if (!dirty) {
        return false;
    }

    if (currentMs - lastDirtyMs >= NVS_DEBOUNCE_DELAY_MS) {
        return saveMatch();
    }
    return false;
}

// -------------------------------------------------------------
// Low-Level Storage Backend Drivers
// -------------------------------------------------------------

bool StorageManager::readBlob(const char* key, void* dest, size_t maxLen, size_t& actualLen) {
#if defined(ESP32) || defined(ARDUINO)
    actualLen = prefs.getBytes(key, dest, maxLen);
    return (actualLen > 0);
#else
    auto it = mockStorage.find(key);
    if (it == mockStorage.end()) {
        actualLen = 0;
        return false;
    }
    actualLen = it->second.size();
    if (actualLen > maxLen) {
        actualLen = maxLen;
    }
    memcpy(dest, it->second.data(), actualLen);
    return true;
#endif
}

bool StorageManager::writeBlob(const char* key, const void* src, size_t len) {
#if defined(ESP32) || defined(ARDUINO)
    size_t written = prefs.putBytes(key, src, len);
    return (written == len);
#else
    std::vector<uint8_t> buffer((const uint8_t*)src, (const uint8_t*)src + len);
    mockStorage[key] = buffer;
    return true;
#endif
}

bool StorageManager::eraseKey(const char* key) {
#if defined(ESP32) || defined(ARDUINO)
    return prefs.remove(key);
#else
    mockStorage.erase(key);
    return true;
#endif
}
