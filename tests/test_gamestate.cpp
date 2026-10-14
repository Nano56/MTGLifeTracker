#include <stdio.h>
#include <assert.h>
#include "../GameState.h"
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
    printf("--- Running test_gamestate ---\n");
    EventLog log;
    GameState state(log);

    // 1. Initial State Verification
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        const PlayerState& p = state.getPlayer(i);
        TEST_ASSERT(p.life == 40);
        TEST_ASSERT(p.poison == 0);
        TEST_ASSERT(p.cmdr == 21);
        TEST_ASSERT(p.tax == 0);
        TEST_ASSERT(p.activeMode == MODE_LIFE);
        TEST_ASSERT(!p.isDead);
        TEST_ASSERT(state.getActiveValue(i) == 40);
    }
    printf("  [PASS] All 4 players initialized to 40 life, 21 commander health\n");

    // 2. Life Adjustments
    state.applyDelta(0, -3, 1000); // P1 life: 40 -> 37
    TEST_ASSERT(state.getPlayer(0).life == 37);
    TEST_ASSERT(state.getActiveValue(0) == 37);

    state.applyDelta(0, 5, 2000); // P1 life: 37 -> 42
    TEST_ASSERT(state.getPlayer(0).life == 42);
    printf("  [PASS] Life increment and decrement working\n");

    // 3. Mode Cycling (LIFE -> POISON -> CMDR -> TAX -> LIFE)
    state.cycleMode(1, 3000); // P2 to POISON
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_POISON);
    TEST_ASSERT(state.getActiveValue(1) == 0);

    state.cycleMode(1, 3100); // P2 to CMDR
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_CMDR);
    TEST_ASSERT(state.getActiveValue(1) == 21);

    state.cycleMode(1, 3200); // P2 to TAX
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_TAX);
    TEST_ASSERT(state.getActiveValue(1) == 0);

    state.cycleMode(1, 3300); // P2 back to LIFE
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_LIFE);
    TEST_ASSERT(state.getActiveValue(1) == 40);
    printf("  [PASS] Mode cycling across all 4 modes verified\n");

    // 4. Commander Tax Steps by +/- 2
    state.cycleMode(2, 4000); // P3: LIFE -> POISON
    state.cycleMode(2, 4000); // P3: POISON -> CMDR
    state.cycleMode(2, 4000); // P3: CMDR -> TAX
    TEST_ASSERT(state.getPlayer(2).activeMode == MODE_TAX);

    state.applyDelta(2, 1, 4100); // 1 dial click in TAX mode = +2 mana
    TEST_ASSERT(state.getPlayer(2).tax == 2);
    TEST_ASSERT(state.getActiveValue(2) == 2);

    state.applyDelta(2, 2, 4200); // 2 dial clicks = +4 mana (total 6)
    TEST_ASSERT(state.getPlayer(2).tax == 6);

    state.applyDelta(2, -1, 4300); // -1 dial click = -2 mana (total 4)
    TEST_ASSERT(state.getPlayer(2).tax == 4);

    state.applyDelta(2, -5, 4400); // Large decrement clamps at 0
    TEST_ASSERT(state.getPlayer(2).tax == 0);
    printf("  [PASS] Commander Tax steps accurately by +/-2 and clamps at 0\n");

    // 5. Poison Counter Lethal Condition (10 Poison)
    state.cycleMode(3, 5000); // P4: LIFE -> POISON
    TEST_ASSERT(state.getPlayer(3).activeMode == MODE_POISON);
    TEST_ASSERT(!state.getPlayer(3).isDead);

    state.applyDelta(3, 9, 5100); // 9 poison
    TEST_ASSERT(state.getPlayer(3).poison == 9);
    TEST_ASSERT(!state.getPlayer(3).isDead);

    state.applyDelta(3, 1, 5200); // 10 poison -> LETHAL!
    TEST_ASSERT(state.getPlayer(3).poison == 10);
    TEST_ASSERT(state.getPlayer(3).isDead);
    printf("  [PASS] Poison counter lethal condition triggered at 10 poison\n");

    // 6. Commander Health Lethal Condition (starts at 21, lethal at <= 0)
    state.resetPlayer(3, 6000);
    TEST_ASSERT(!state.getPlayer(3).isDead);
    TEST_ASSERT(state.getPlayer(3).life == 40);
    TEST_ASSERT(state.getPlayer(3).cmdr == 21);

    state.cycleMode(3, 6100); // POISON
    state.cycleMode(3, 6100); // CMDR
    TEST_ASSERT(state.getActiveValue(3) == 21);

    // Taking 20 commander damage reduces commander health to 1
    state.applyDelta(3, -20, 6200);
    TEST_ASSERT(!state.getPlayer(3).isDead);
    TEST_ASSERT(state.getPlayer(3).life == 40); // Life remains 40 (separate from commander health!)
    TEST_ASSERT(state.getPlayer(3).cmdr == 1);

    // Taking 1 more commander damage reduces commander health to 0 -> LETHAL!
    state.applyDelta(3, -1, 6300);
    TEST_ASSERT(state.getPlayer(3).cmdr == 0);
    TEST_ASSERT(state.getPlayer(3).life == 40); // Life still 40, but player loses due to 0 CMDR health!
    TEST_ASSERT(state.getPlayer(3).isDead);
    printf("  [PASS] Commander health starts at 21, decrements independently of life, and triggers lethal at <= 0\n");

    // 7. Life <= 0 Lethal Condition
    state.resetPlayer(0, 7000);
    TEST_ASSERT(!state.getPlayer(0).isDead);
    state.applyDelta(0, -40, 7100); // Life reaches 0 -> LETHAL!
    TEST_ASSERT(state.getPlayer(0).life == 0);
    TEST_ASSERT(state.getPlayer(0).isDead);
    printf("  [PASS] Life <= 0 lethal condition triggered\n");

    // 8. Mode Persistence (Modes stay active until explicitly cycled)
    state.resetAll(8000);
    state.cycleMode(1, 8000); // P2 to POISON
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_POISON);

    bool returned = state.checkAutoReturn(20000); // After 12s, mode remains POISON
    TEST_ASSERT(!returned);
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_POISON);
    printf("  [PASS] Mode selection persists without unexpected auto-revert\n");

    // 9. Full Reset
    state.resetAll(14000);
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        TEST_ASSERT(state.getPlayer(i).life == 40);
        TEST_ASSERT(state.getPlayer(i).poison == 0);
        TEST_ASSERT(state.getPlayer(i).cmdr == 21);
        TEST_ASSERT(state.getPlayer(i).tax == 0);
        TEST_ASSERT(!state.getPlayer(i).isDead);
    }
    printf("  [PASS] Full game reset verified (all players at 40 life and 21 commander health)\n");

    printf(">>> test_gamestate: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}
