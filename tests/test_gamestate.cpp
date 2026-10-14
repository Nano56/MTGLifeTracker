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
        for (uint8_t j = 0; j < NUM_PLAYERS; j++) {
            TEST_ASSERT(p.cmdrDmg[j] == 0);
        }
        TEST_ASSERT(p.tax == 0);
        TEST_ASSERT(p.activeMode == MODE_LIFE);
        TEST_ASSERT(!p.isDead);
        TEST_ASSERT(state.getActiveValue(i) == 40);
    }
    printf("  [PASS] All 4 players initialized to 40 life, 0 counters\n");

    // 2. Life Adjustments
    state.applyDelta(0, -3, 1000); // P1 life: 40 -> 37
    TEST_ASSERT(state.getPlayer(0).life == 37);
    TEST_ASSERT(state.getActiveValue(0) == 37);

    state.applyDelta(0, 5, 2000); // P1 life: 37 -> 42
    TEST_ASSERT(state.getPlayer(0).life == 42);
    printf("  [PASS] Life increment and decrement working\n");

    // 3. Mode Cycling (LIFE -> CMDR P1..P4 skipping self -> POISON -> TAX -> LIFE)
    // Testing on P2 (index 1): opponents are P1 (0), P3 (2), P4 (3)
    state.cycleMode(1, 3000); // CMDR from P1
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_CMDR);
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 0);

    state.cycleMode(1, 3100); // CMDR from P3
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_CMDR);
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 2);

    state.cycleMode(1, 3200); // CMDR from P4
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_CMDR);
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 3);

    state.cycleMode(1, 3300); // POISON
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_POISON);
    TEST_ASSERT(state.getActiveValue(1) == 0);

    state.cycleMode(1, 3400); // TAX
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_TAX);
    TEST_ASSERT(state.getActiveValue(1) == 0);

    state.cycleMode(1, 3500); // Back to LIFE
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_LIFE);
    TEST_ASSERT(state.getActiveValue(1) == 40);
    printf("  [PASS] Option A mode cycling through each opponent commander verified\n");

    // 4. Commander Damage Synchronized Life Reduction (CR 903.10a / 704.6c)
    state.cycleMode(1, 3600); // P2 selects CMDR from P1 (0)
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_CMDR);
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 0);

    state.applyDelta(1, 5, 3700); // P2 takes 5 commander damage from P1
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[0] == 5);
    TEST_ASSERT(state.getPlayer(1).life == 35); // Life automatically reduced by 5!

    state.applyDelta(1, -2, 3800); // Correction: -2 commander damage
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[0] == 3);
    TEST_ASSERT(state.getPlayer(1).life == 37); // Life restored by 2!
    printf("  [PASS] Commander damage adjusts in tandem with life total\n");

    // 5. Per-Opponent Independent Commander Lethal Rule
    // P2 currently has 3 damage from P1. Life is 37.
    state.applyDelta(1, 8, 3900); // P1 commander damage = 11. Life = 29.
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[0] == 11);
    TEST_ASSERT(!state.getPlayer(1).isDead);

    state.cycleMode(1, 4000); // Switch to CMDR from P3 (source 2)
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 2);
    state.applyDelta(1, 10, 4100); // P3 commander damage = 10. Life = 19.
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[2] == 10);

    // Total commander damage is 11 + 10 = 21 across multiple opponents, but NOT lethal!
    TEST_ASSERT(!state.getPlayer(1).isDead);
    printf("  [PASS] Multi-opponent commander damage sum >= 21 does NOT kill player\n");

    // Now P1 hits for 10 more (P1 total = 21) -> LETHAL from single commander!
    state.returnToLife(1, 4200);
    state.cycleMode(1, 4300); // Switch back to CMDR P1 (source 0)
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 0);
    state.applyDelta(1, 10, 4400); // P1 damage reaches 21! Life becomes 9.
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[0] == 21);
    TEST_ASSERT(state.getPlayer(1).life == 9);
    TEST_ASSERT(state.getPlayer(1).isDead); // ELIMINATED!
    TEST_ASSERT(state.getPlayer(1).deadReasonMask & DEAD_REASON_CMDR_P1);
    printf("  [PASS] Lethal elimination triggered when single commander reaches 21 damage\n");

    // Correcting commander damage below 21 removes lethal status (since life is 9 > 0)
    state.applyDelta(1, -1, 4500);
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[0] == 20);
    TEST_ASSERT(state.getPlayer(1).life == 10);
    TEST_ASSERT(!state.getPlayer(1).isDead);
    printf("  [PASS] Correcting commander damage below 21 revives player when life > 0\n");

    // 6. Commander Tax Steps by +/- 2
    state.returnToLife(2, 5000);
    state.cycleMode(2, 5000); // CMDR P1
    state.cycleMode(2, 5000); // CMDR P2
    state.cycleMode(2, 5000); // CMDR P4
    state.cycleMode(2, 5000); // POISON
    state.cycleMode(2, 5000); // TAX
    TEST_ASSERT(state.getPlayer(2).activeMode == MODE_TAX);

    state.applyDelta(2, 1, 5100); // 1 dial click in TAX mode = +2 mana
    TEST_ASSERT(state.getPlayer(2).tax == 2);

    state.applyDelta(2, 2, 5200); // 2 dial clicks = +4 mana (total 6)
    TEST_ASSERT(state.getPlayer(2).tax == 6);

    state.applyDelta(2, -1, 5300); // -1 dial click = -2 mana (total 4)
    TEST_ASSERT(state.getPlayer(2).tax == 4);

    state.applyDelta(2, -5, 5400); // Large decrement clamps at 0
    TEST_ASSERT(state.getPlayer(2).tax == 0);
    printf("  [PASS] Commander Tax steps accurately by +/-2 and clamps at 0\n");

    // 7. Poison Counter Lethal Condition (10 Poison)
    state.returnToLife(3, 6000);
    state.cycleMode(3, 6000); // CMDR P1
    state.cycleMode(3, 6000); // CMDR P2
    state.cycleMode(3, 6000); // CMDR P3
    state.cycleMode(3, 6000); // POISON
    TEST_ASSERT(state.getPlayer(3).activeMode == MODE_POISON);
    TEST_ASSERT(!state.getPlayer(3).isDead);

    state.applyDelta(3, 9, 6100); // 9 poison
    TEST_ASSERT(state.getPlayer(3).poison == 9);
    TEST_ASSERT(!state.getPlayer(3).isDead);

    state.applyDelta(3, 1, 6200); // 10 poison -> LETHAL!
    TEST_ASSERT(state.getPlayer(3).poison == 10);
    TEST_ASSERT(state.getPlayer(3).isDead);
    TEST_ASSERT(state.getPlayer(3).deadReasonMask & DEAD_REASON_POISON);
    printf("  [PASS] Poison counter lethal condition triggered at 10 poison\n");

    // 8. Life <= 0 Lethal Condition
    state.resetPlayer(0, 7000);
    TEST_ASSERT(!state.getPlayer(0).isDead);
    state.applyDelta(0, -40, 7100); // Life reaches 0 -> LETHAL!
    TEST_ASSERT(state.getPlayer(0).life == 0);
    TEST_ASSERT(state.getPlayer(0).isDead);
    TEST_ASSERT(state.getPlayer(0).deadReasonMask & DEAD_REASON_LIFE);
    printf("  [PASS] Life <= 0 lethal condition triggered\n");

    // 9. Instant Return to LIFE
    state.cycleMode(0, 8000);
    TEST_ASSERT(state.getPlayer(0).activeMode != MODE_LIFE);
    state.returnToLife(0, 8100);
    TEST_ASSERT(state.getPlayer(0).activeMode == MODE_LIFE);
    printf("  [PASS] Instant returnToLife verified\n");

    // 10. Full Reset Clears Log & Resets All Counters
    state.resetAll(9000);
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        TEST_ASSERT(state.getPlayer(i).life == 40);
        TEST_ASSERT(state.getPlayer(i).poison == 0);
        for (uint8_t j = 0; j < NUM_PLAYERS; j++) {
            TEST_ASSERT(state.getPlayer(i).cmdrDmg[j] == 0);
        }
        TEST_ASSERT(state.getPlayer(i).tax == 0);
        TEST_ASSERT(!state.getPlayer(i).isDead);
    }
    TEST_ASSERT(log.getCount() == 1); // Only EVT_GAME_RESET in clean log!
    printf("  [PASS] Full game reset verified (all players at 40 life and clean log)\n");

    printf(">>> test_gamestate: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}
