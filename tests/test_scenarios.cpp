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
    // SCENARIO 1: Typical Multi-Turn Combat Match
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
    // P4 cycles mode to POISON, then increments 3 times
    controller.dispatchAction(3, ACT_CYCLE_MODE, 5000); // LIFE -> POISON
    TEST_ASSERT(state.getPlayer(3).activeMode == MODE_POISON);
    for (int i = 0; i < 3; i++) {
        controller.dispatchAction(3, ACT_INCREMENT, 5100 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(3).poison == 3);
    TEST_ASSERT(!state.getPlayer(3).isDead);
    printf("  [PASS] P4 took 3 poison counters, poison = 3\n");

    // Cycle P4 back to LIFE mode
    controller.dispatchAction(3, ACT_CYCLE_MODE, 10000); // POISON -> CMDR
    controller.dispatchAction(3, ACT_CYCLE_MODE, 10100); // CMDR -> TAX
    controller.dispatchAction(3, ACT_CYCLE_MODE, 10200); // TAX -> LIFE
    TEST_ASSERT(state.getPlayer(3).activeMode == MODE_LIFE);
    printf("  [PASS] P4 mode cycled cleanly back to LIFE\n");

    // Turn 4: P1 commander is destroyed and recast!
    // P1 cycles mode to TAX, then increments once (+2 mana tax)
    controller.dispatchAction(0, ACT_CYCLE_MODE, 12000); // LIFE -> POISON
    controller.dispatchAction(0, ACT_CYCLE_MODE, 12100); // POISON -> CMDR
    controller.dispatchAction(0, ACT_CYCLE_MODE, 12200); // CMDR -> TAX
    TEST_ASSERT(state.getPlayer(0).activeMode == MODE_TAX);
    controller.dispatchAction(0, ACT_INCREMENT, 12300); // 1 dial click in TAX mode = +2
    TEST_ASSERT(state.getPlayer(0).tax == 2);
    printf("  [PASS] P1 commander tax incremented to +2 mana\n");

    // Turn 5: P4 takes 7 more poison (total 10) -> Lethal Elimination!
    controller.dispatchAction(3, ACT_CYCLE_MODE, 15000); // LIFE -> POISON
    for (int i = 0; i < 7; i++) {
        controller.dispatchAction(3, ACT_INCREMENT, 15100 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(3).poison == 10);
    TEST_ASSERT(state.getPlayer(3).isDead); // ELIMINATED!
    printf("  [PASS] P4 took lethal poison (10 poison) and was eliminated\n");

    // Turn 6: P1 attacks P2 with Commander. P2 starts with 21 Commander Health.
    // P2 takes 21 Commander Damage -> Commander Health reaches 0 -> Lethal Elimination!
    controller.dispatchAction(1, ACT_CYCLE_MODE, 20000); // LIFE -> POISON
    controller.dispatchAction(1, ACT_CYCLE_MODE, 20100); // POISON -> CMDR
    TEST_ASSERT(state.getPlayer(1).activeMode == MODE_CMDR);
    TEST_ASSERT(state.getPlayer(1).cmdr == 21);
    for (int i = 0; i < 21; i++) {
        controller.dispatchAction(1, ACT_DECREMENT, 20200 + i * 50);
    }
    TEST_ASSERT(state.getPlayer(1).cmdr == 0);
    TEST_ASSERT(state.getPlayer(1).life == 35); // Main life points untouched!
    TEST_ASSERT(state.getPlayer(1).isDead); // ELIMINATED by commander health reaching 0!
    printf("  [PASS] P2 commander health reached 0 and was eliminated (life remained separate at 35)\n");

    // Verify Event Log contains complete historical record
    TEST_ASSERT(log.getCount() > 10);
    GameEvent latest;
    TEST_ASSERT(log.getLatest(latest));
    TEST_ASSERT(latest.player == 1);
    TEST_ASSERT(latest.type == EVT_CMDR_CHANGE);
    TEST_ASSERT(latest.newValue == 0);
    printf("  [PASS] Complete event history recorded accurately in ring buffer\n");

    // Reset game
    controller.dispatchAction(0, ACT_RESET_GAME, 30000);
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        TEST_ASSERT(state.getPlayer(i).life == 40);
        TEST_ASSERT(state.getPlayer(i).poison == 0);
        TEST_ASSERT(state.getPlayer(i).cmdr == 21);
        TEST_ASSERT(!state.getPlayer(i).isDead);
    }
    printf("  [PASS] Full game reset restored all players to 40 life and 21 commander health\n");

    printf("\n>>> test_scenarios: ALL %d ASSERTIONS PASSED <<<\n\n", g_assertCount);
    return 0;
}

