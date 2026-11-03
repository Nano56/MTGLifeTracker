#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "Config.h"
#include "GameState.h"
#include "EventLog.h"

#define NVS_MAGIC_HEADER        0x4D544731 // "MTG1"
#define NVS_FORMAT_VERSION      1
#define NVS_DEFAULT_NAMESPACE   "mtg_tracker"
#define NVS_KEY_GAME_STATE      "state"
#define NVS_KEY_EVENT_LOG       "log"
#define NVS_DEBOUNCE_DELAY_MS   2500

#pragma pack(push, 1)
struct SerializedPlayer {
    int16_t  life;
    int8_t   poison;
    int8_t   tax;
    uint8_t  cmdrDmg[NUM_PLAYERS];
};

struct SerializedGameState {
    uint32_t magic;
    uint8_t  version;
    uint8_t  playerCount;
    SerializedPlayer players[NUM_PLAYERS];
    uint32_t checksum;
};

struct SerializedEvent {
    uint32_t timestampMs;
    uint8_t  player;
    uint8_t  type;
    uint8_t  source;
    int16_t  delta;
    int16_t  newValue;
    int16_t  secondaryValue;
};

struct SerializedEventLog {
    uint32_t magic;
    uint8_t  version;
    uint8_t  eventCount;
    SerializedEvent events[EVENT_LOG_CAPACITY];
    uint32_t checksum;
};
#pragma pack(pop)

class StorageManager {
public:
    StorageManager(GameState& stateRef, EventLog& logRef);

    // Initialize NVS storage subsystem
    bool begin();

    // Check if an active match is saved in storage
    bool hasSavedMatch();

    // Load saved match from storage. Returns true if valid active match was restored.
    bool loadMatch();

    // Immediately commit current state to storage
    bool saveMatch();

    // Clear saved match (called on full game reset)
    void clearStorage();

    // Mark storage dirty with timestamp to trigger a debounced commit
    void markDirty(uint32_t currentMs);

    // Periodic tick in main loop: writes to storage when debounce timer elapses
    bool tick(uint32_t currentMs);

    // Status queries
    bool isDirty() const { return dirty; }
    uint32_t getLastDirtyMs() const { return lastDirtyMs; }

    // Checksum calculation helper (Fletcher-32)
    static uint32_t computeChecksum(const uint8_t* data, size_t length);

private:
    GameState& state;
    EventLog& log;
    bool dirty;
    uint32_t lastDirtyMs;
    bool initialized;

    // Platform-specific storage backend methods
    bool readBlob(const char* key, void* dest, size_t maxLen, size_t& actualLen);
    bool writeBlob(const char* key, const void* src, size_t len);
    bool eraseKey(const char* key);
};

#endif // STORAGE_MANAGER_H
