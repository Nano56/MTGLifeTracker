#include <stdio.h>
#include <assert.h>
#include "../GameState.h"
#include "../EventLog.h"
#include "../AppController.h"

static int g_assertCount = 0;
#define TEST_ASSERT(cond) do { \
    g_assertCount++; \
    if (!(cond)) { \
        printf("FAILED SCENARIO ASSERTION: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
        return 1; \
    } \
} while (0)

int main() {
    printf("--- Running test_scenarios (Full-Game HID-Agnostic Simulation) ---\n");
    EventLog log;
    GameState state(log);
    AppController controller(state, log);

    // =========================================================
    // SCENARIO 1: Typical Multi-Turn Commander Match Simulation
    // =========================================================
    printf("\n[Scenario 1] Multi-Turn Commander Match Simulation\n");

    // Turn 1: P1 casts Commander. Commander Tax remains 0.
    TEST_ASSERT(state.getPlayer(0).tax == 0);

    // Turn 2: P1 attacks P2 for 5 regular combat damage.
    // P2 decrements life 5 times via HID (e.g. 5 dial ticks or 5 button presses)
    for (int i = 0; i < 5; i++) {
        controller.dispatchAction(1, ACT_DECREMENT, 1000 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(1).life == 35);
    TEST_ASSERT(!state.getPlayer(1).isDead);
    printf("  [PASS] P2 took 5 combat damage, life = 35\n");

    // Turn 3: P3 attacks P4 with an Infect creature for 3 Poison.
    // P4 (index 3): opponents are P1, P2, P3. Cycle to CMDR(P1), CMDR(P2), CMDR(P3), POISON
    controller.dispatchAction(3, ACT_CYCLE_MODE, 5000); // CMDR P1
    controller.dispatchAction(3, ACT_CYCLE_MODE, 5050); // CMDR P2
    controller.dispatchAction(3, ACT_CYCLE_MODE, 5100); // CMDR P3
    controller.dispatchAction(3, ACT_CYCLE_MODE, 5150); // POISON
    TEST_ASSERT(state.getPlayer(3).activeMode == MODE_POISON);
    for (int i = 0; i < 3; i++) {
        controller.dispatchAction(3, ACT_INCREMENT, 5200 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(3).poison == 3);
    TEST_ASSERT(!state.getPlayer(3).isDead);
    printf("  [PASS] P4 took 3 poison counters, poison = 3\n");

    // Test instant double-click return to LIFE mode
    controller.dispatchAction(3, ACT_RETURN_TO_LIFE, 10000);
    TEST_ASSERT(state.getPlayer(3).activeMode == MODE_LIFE);
    printf("  [PASS] P4 double-clicked knob to return instantly to LIFE mode\n");

    // Turn 4: P1 commander is destroyed and recast!
    // P1 cycles to TAX: CMDR P2 -> CMDR P3 -> CMDR P4 -> POISON -> TAX
    controller.dispatchAction(0, ACT_CYCLE_MODE, 12000); // CMDR P2
    controller.dispatchAction(0, ACT_CYCLE_MODE, 12050); // CMDR P3
    controller.dispatchAction(0, ACT_CYCLE_MODE, 12100); // CMDR P4
    controller.dispatchAction(0, ACT_CYCLE_MODE, 12150); // POISON
    controller.dispatchAction(0, ACT_CYCLE_MODE, 12200); // TAX
    TEST_ASSERT(state.getPlayer(0).activeMode == MODE_TAX);
    controller.dispatchAction(0, ACT_INCREMENT, 12300); // 1 dial click in TAX mode = +2 mana
    TEST_ASSERT(state.getPlayer(0).tax == 2);
    controller.dispatchAction(0, ACT_RETURN_TO_LIFE, 12400); // Return to LIFE
    printf("  [PASS] P1 commander tax incremented to +2 mana\n");

    // Turn 5: P4 takes 7 more poison (total 10) -> Lethal Elimination!
    controller.dispatchAction(3, ACT_CYCLE_MODE, 15000); // CMDR P1
    controller.dispatchAction(3, ACT_CYCLE_MODE, 15050); // CMDR P2
    controller.dispatchAction(3, ACT_CYCLE_MODE, 15100); // CMDR P3
    controller.dispatchAction(3, ACT_CYCLE_MODE, 15150); // POISON
    for (int i = 0; i < 7; i++) {
        controller.dispatchAction(3, ACT_INCREMENT, 15200 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(3).poison == 10);
    TEST_ASSERT(state.getPlayer(3).isDead); // ELIMINATED!
    printf("  [PASS] P4 took lethal poison (10 poison) and was eliminated\n");

    // Turn 6: Commander Combat Rules (CR 903.10a / 704.6c)
    // P2 takes 10 commander damage from P1:
    // P2 starts at 35 life.
    controller.dispatchAction(1, ACT_CYCLE_MODE, 20000); // CMDR P1 (source 0)
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_CMDR);
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 0);
    for (int i = 0; i < 10; i++) {
        controller.dispatchAction(1, ACT_INCREMENT, 20100 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[0] == 10);
    TEST_ASSERT(state.getPlayer(1).life == 25); // Life synchronized: 35 - 10 = 25!
    TEST_ASSERT(!state.getPlayer(1).isDead);

    // P2 also takes 10 commander damage from P3:
    controller.dispatchAction(1, ACT_CYCLE_MODE, 21000); // CMDR P3 (source 2)
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 2);
    for (int i = 0; i < 10; i++) {
        controller.dispatchAction(1, ACT_INCREMENT, 21100 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[2] == 10);
    TEST_ASSERT(state.getPlayer(1).life == 15); // Life synchronized: 25 - 10 = 15!
    // Total commander damage taken = 20, but not 21 from any single opponent
    TEST_ASSERT(!state.getPlayer(1).isDead);

    // Turn 7: P1 attacks P2 for 11 more commander damage (P1 commander damage reaches 21)
    controller.dispatchAction(1, ACT_RETURN_TO_LIFE, 22000);
    controller.dispatchAction(1, ACT_CYCLE_MODE, 22100); // CMDR P1
    TEST_ASSERT(state.getPlayer(1).cmdrSource == 0);
    for (int i = 0; i < 11; i++) {
        controller.dispatchAction(1, ACT_INCREMENT, 22200 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(1).cmdrDmg[0] == 21);
    TEST_ASSERT(state.getPlayer(1).life == 4); // Life is 15 - 11 = 4 (> 0)
    TEST_ASSERT(state.getPlayer(1).isDead); // But ELIMINATED by 21 damage from P1's commander!
    printf("  [PASS] P2 eliminated by 21 damage from single commander while life remained positive at 4\n");

    // Verify Event Log contains complete historical record
    TEST_ASSERT(log.getCount() > 10);
    GameEvent latest;
    TEST_ASSERT(log.getLatest(latest));
    TEST_ASSERT(latest.player == 1);
    TEST_ASSERT(latest.type == EVT_CMDR_CHANGE);
    TEST_ASSERT(latest.source == 0); // P1
    TEST_ASSERT(latest.newValue == 21);
    TEST_ASSERT(latest.secondaryValue == 4); // Resulting life = 4
    printf("  [PASS] Complete event history recorded accurately in ring buffer\n");

    // Reset game
    controller.dispatchAction(0, ACT_RESET_GAME, 30000);
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        TEST_ASSERT(state.getPlayer(i).life == 40);
        TEST_ASSERT(state.getPlayer(i).poison == 0);
        for (uint8_t j = 0; j < NUM_PLAYERS; j++) {
            TEST_ASSERT(state.getPlayer(i).cmdrDmg[j] == 0);
        }
        TEST_ASSERT(!state.getPlayer(i).isDead);
    }
    printf("  [PASS] Full game reset restored all players to 40 life and 0 commander damage\n");

    printf("\n>>> test_scenarios: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}
