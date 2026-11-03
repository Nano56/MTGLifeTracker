#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Config.h"
#include "../EventLog.h"
#include "../GameState.h"
#include "../StorageManager.h"

static int g_assertCount = 0;
#define TEST_ASSERT(cond) do { \
    g_assertCount++; \
    if (!(cond)) { \
        printf("FAILED STORAGE ASSERTION: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

int main() {
    printf("--- Running test_storage (Power Loss Recovery & Flash Persistence) ---\n");

    EventLog log;
    GameState state(log);
    StorageManager storage(state, log);

    // 1. Initial State
    TEST_ASSERT(storage.begin() == true);
    storage.clearStorage();
    TEST_ASSERT(storage.hasSavedMatch() == false);
    TEST_ASSERT(storage.loadMatch() == false);
    printf("  [PASS] Empty storage initialization & absence check verified\n");

    // 2. Modify state and save match
    state.resetAll(0);
    state.applyDelta(0, -5, 100);  // P1: Life 35
    state.cycleMode(0, 200);       // P1: CMDR Mode
    state.applyDelta(0, 1, 300);   // P1: CMDR dmg from P2 +1
    state.cycleMode(0, 400);       // P1: CMDR P3
    state.cycleMode(0, 500);       // P1: CMDR P4
    state.cycleMode(0, 600);       // P1: Poison Mode
    state.applyDelta(0, 2, 700);   // P1: 2 Poison
    state.cycleMode(0, 800);       // P1: Tax Mode
    state.applyDelta(0, 1, 900);   // P1: Tax +2

    // P2 takes lethal commander damage from P1 (21)
    state.cycleMode(1, 1000);
    state.applyDelta(1, 21, 1100); // P2: takes 21 cmdr damage from P1 -> Dead
    TEST_ASSERT(state.getPlayer(1).isDead == true);

    // P3 takes lethal poison (10)
    state.cycleMode(2, 1200);      // CMDR
    state.cycleMode(2, 1300);      // CMDR
    state.cycleMode(2, 1400);      // CMDR
    state.cycleMode(2, 1500);      // Poison
    state.applyDelta(2, 10, 1600); // P3: 10 Poison -> Dead
    TEST_ASSERT(state.getPlayer(2).isDead == true);

    uint16_t origLogCount = log.getCount();
    TEST_ASSERT(origLogCount > 5);

    // Save state
    TEST_ASSERT(storage.saveMatch() == true);
    TEST_ASSERT(storage.hasSavedMatch() == true);
    TEST_ASSERT(storage.isDirty() == false);
    printf("  [PASS] Complex match state (Life, CMDR, Poison, Tax, Lethal Deaths) saved\n");

    // 3. Reset game state to clean 40 life
    state.resetAll(2000);
    TEST_ASSERT(state.getPlayer(0).life == 40);
    TEST_ASSERT(state.getPlayer(0).poison == 0);
    TEST_ASSERT(state.getPlayer(0).tax == 0);
    TEST_ASSERT(state.getPlayer(1).isDead == false);
    TEST_ASSERT(state.getPlayer(2).isDead == false);
    TEST_ASSERT(log.getCount() == 1); // Only reset event
    printf("  [PASS] Game reset cleared live state to baseline\n");

    // 4. Restore state from StorageManager
    TEST_ASSERT(storage.loadMatch() == true);

    // Verify P1
    const PlayerState& p1 = state.getPlayer(0);
    TEST_ASSERT(p1.life == 34); // 40 - 5 life - 1 cmdr damage = 34
    TEST_ASSERT(p1.poison == 2);
    TEST_ASSERT(p1.tax == 2);
    TEST_ASSERT(p1.cmdrDmg[1] == 1);
    TEST_ASSERT(p1.isDead == false);

    // Verify P2 (Dead from 21 Commander damage)
    const PlayerState& p2 = state.getPlayer(1);
    TEST_ASSERT(p2.cmdrDmg[0] == 21);
    TEST_ASSERT(p2.isDead == true);

    // Verify P3 (Dead from 10 Poison counters)
    const PlayerState& p3 = state.getPlayer(2);
    TEST_ASSERT(p3.poison == 10);
    TEST_ASSERT(p3.isDead == true);

    // Verify EventLog restoration
    TEST_ASSERT(log.getCount() == origLogCount);
    GameEvent latestEvt;
    TEST_ASSERT(log.getLatest(latestEvt) == true);
    TEST_ASSERT(latestEvt.player == 2); // P3 was latest action
    printf("  [PASS] Round-trip serialization restored all player stats and event log\n");

    // 5. Dynamic Player Count Persistence (2-Player and 3-Player modes)
    state.setPlayerCount(2, 3000);
    state.applyDelta(0, -10, 3100);
    TEST_ASSERT(storage.saveMatch() == true);

    state.resetAll(3200);
    state.setPlayerCount(4, 3300);
    TEST_ASSERT(state.getPlayerCount() == 4);

    TEST_ASSERT(storage.loadMatch() == true);
    TEST_ASSERT(state.getPlayerCount() == 2);
    TEST_ASSERT(state.getPlayer(0).life == 30);
    printf("  [PASS] Dynamic 2-Player configuration and layout restored accurately\n");

    state.setPlayerCount(3, 4000);
    TEST_ASSERT(storage.saveMatch() == true);
    state.resetAll(4100);
    TEST_ASSERT(storage.loadMatch() == true);
    TEST_ASSERT(state.getPlayerCount() == 3);
    printf("  [PASS] Dynamic 3-Player configuration restored accurately\n");

    // 6. Debounced auto-save commit timing
    storage.clearStorage();
    TEST_ASSERT(storage.hasSavedMatch() == false);

    storage.markDirty(10000);
    TEST_ASSERT(storage.isDirty() == true);

    // Check before timeout (500ms elapsed)
    TEST_ASSERT(storage.tick(10500) == false);
    TEST_ASSERT(storage.isDirty() == true);
    TEST_ASSERT(storage.hasSavedMatch() == false);

    // Check at 2000ms elapsed
    TEST_ASSERT(storage.tick(12000) == false);
    TEST_ASSERT(storage.isDirty() == true);
    TEST_ASSERT(storage.hasSavedMatch() == false);

    // Check at 2500ms elapsed -> triggers commit!
    TEST_ASSERT(storage.tick(12500) == true);
    TEST_ASSERT(storage.isDirty() == false);
    TEST_ASSERT(storage.hasSavedMatch() == true);
    printf("  [PASS] Debounced NVS write timing (2500ms threshold) verified\n");

    // 7. Clear Storage
    storage.clearStorage();
    TEST_ASSERT(storage.hasSavedMatch() == false);
    TEST_ASSERT(storage.loadMatch() == false);
    printf("  [PASS] clearStorage() purged all data cleanly\n");

    // 8. Checksum calculation validation
    uint8_t sampleData[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint32_t c1 = StorageManager::computeChecksum(sampleData, sizeof(sampleData));
    TEST_ASSERT(c1 != 0);
    sampleData[0] ^= 0xFF;
    uint32_t c2 = StorageManager::computeChecksum(sampleData, sizeof(sampleData));
    TEST_ASSERT(c1 != c2);
    printf("  [PASS] Checksum calculation and bit-flip detection verified\n");

    printf(">>> test_storage: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}
