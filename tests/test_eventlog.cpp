#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../EventLog.h"

static int g_assertCount = 0;
#define TEST_ASSERT(cond) do { \
    g_assertCount++; \
    if (!(cond)) { \
        printf("FAILED ASSERTION: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

int main() {
    printf("--- Running test_eventlog ---\n");
    EventLog log;

    // 1. Initial State
    TEST_ASSERT(log.getCount() == 0);
    TEST_ASSERT(log.getTotalCount() == 0);

    GameEvent evt;
    TEST_ASSERT(!log.getLatest(evt));
    printf("  [PASS] Empty log initialization verified\n");

    // 2. Logging and Formatting
    log.log(65000, 0, EVT_LIFE_CHANGE, -3, 37); // t=65s = 01:05
    TEST_ASSERT(log.getCount() == 1);
    TEST_ASSERT(log.getTotalCount() == 1);
    TEST_ASSERT(log.getLatest(evt));
    TEST_ASSERT(evt.player == 0);
    TEST_ASSERT(evt.delta == -3);
    TEST_ASSERT(evt.newValue == 37);

    char strBuf[64];
    EventLog::formatEvent(evt, strBuf, sizeof(strBuf));
    TEST_ASSERT(strcmp(strBuf, "[01:05] P1: Life -3 (37)") == 0);
    printf("  [PASS] Event formatting verified: %s\n", strBuf);

    // 3. Different Event Types
    log.log(75000, 1, EVT_POISON_CHANGE, 1, 1);
    TEST_ASSERT(log.getLatest(evt));
    EventLog::formatEvent(evt, strBuf, sizeof(strBuf));
    TEST_ASSERT(strcmp(strBuf, "[01:15] P2: Poison +1 (1)") == 0);

    // Commander damage with opponent source and resulting life
    log.log(80000, 2, EVT_CMDR_CHANGE, 7, 7, 0, 33);
    TEST_ASSERT(log.getLatest(evt));
    EventLog::formatEvent(evt, strBuf, sizeof(strBuf));
    TEST_ASSERT(strcmp(strBuf, "[01:20] P3: Cmdr P1 +7 (07) | Life 33") == 0);

    log.log(85000, 2, EVT_PLAYER_RESET, 0, 40);
    TEST_ASSERT(log.getLatest(evt));
    EventLog::formatEvent(evt, strBuf, sizeof(strBuf));
    TEST_ASSERT(strcmp(strBuf, "[01:25] P3: Reset to 40 Life") == 0);

    log.log(90000, 3, EVT_TAX_CHANGE, 2, 2);
    TEST_ASSERT(log.getLatest(evt));
    EventLog::formatEvent(evt, strBuf, sizeof(strBuf));
    TEST_ASSERT(strcmp(strBuf, "[01:30] P4: Tax +2 (2)") == 0);

    log.log(95000, SYSTEM_PLAYER_ID, EVT_GAME_RESET, 0, 0);
    TEST_ASSERT(log.getLatest(evt));
    EventLog::formatEvent(evt, strBuf, sizeof(strBuf));
    TEST_ASSERT(strcmp(strBuf, "[01:35] SYS: Game Reset") == 0);

    log.log(100000, 2, EVT_ROLL_FIRST, 0, 0);
    TEST_ASSERT(log.getLatest(evt));
    EventLog::formatEvent(evt, strBuf, sizeof(strBuf));
    TEST_ASSERT(strcmp(strBuf, "[01:40] SYS: P3 Goes First") == 0);
    printf("  [PASS] All event types formatting verified\n");

    // 4. Ring Buffer Wrap-Around (Capacity = 50)
    log.clear();
    for (int i = 0; i < 75; i++) {
        log.log(i * 1000, 0, EVT_LIFE_CHANGE, 1, i + 1);
    }
    TEST_ASSERT(log.getCount() == 50);
    TEST_ASSERT(log.getTotalCount() == 75);

    // Oldest event should be i = 25 (newValue = 26)
    TEST_ASSERT(log.getEvent(0, evt));
    TEST_ASSERT(evt.newValue == 26);

    // Newest event should be i = 74 (newValue = 75)
    TEST_ASSERT(log.getEvent(49, evt));
    TEST_ASSERT(evt.newValue == 75);
    printf("  [PASS] Circular ring buffer wrap-around (75 events pushed to 50 cap) verified\n");

    printf(">>> test_eventlog: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}
