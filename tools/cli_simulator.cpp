#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "../GameState.h"
#include "../EventLog.h"
#include "../AppController.h"

// ANSI Color Codes
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_CYAN    "\033[1;36m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_GREY    "\033[90m"
#define ANSI_BG_RED  "\033[41;1;37m"
#define ANSI_BG_CYAN "\033[46;1;30m"

const char* getPlayerAnsiColor(uint8_t playerIdx) {
    switch (playerIdx) {
        case 0: return ANSI_RED;
        case 1: return ANSI_CYAN;
        case 2: return ANSI_GREEN;
        case 3: return ANSI_YELLOW;
        default: return ANSI_RESET;
    }
}

void formatDeadReason(const PlayerState& p, char* buf, size_t maxLen) {
    for (uint8_t i = 0; i < NUM_PLAYERS; i++) {
        if (p.cmdrDmg[i] >= LETHAL_CMDR_DAMAGE) {
            snprintf(buf, maxLen, "DEAD: 21 CMDR (P%d)", i + 1);
            return;
        }
    }
    if (p.life <= 0) {
        snprintf(buf, maxLen, "DEAD: 0 LIFE");
    } else {
        snprintf(buf, maxLen, "DEAD: 10 POIS");
    }
}

void printBoard(const GameState& state, const EventLog& log, uint32_t currentMs) {
    // Clear screen / move cursor to top
    printf("\033[H\033[J");

    printf("%s====================================================================================%s\n", ANSI_GREY, ANSI_RESET);
    printf("                  %sMTG COMMANDER LIFE TRACKER - DESKTOP SIMULATOR%s\n", ANSI_BOLD, ANSI_RESET);
    printf("                          Uptime: %02u:%02u (MTG EDH Rules Engine)\n", (unsigned)(currentMs / 60000), (unsigned)((currentMs / 1000) % 60));
    printf("%s====================================================================================%s\n", ANSI_GREY, ANSI_RESET);

    const PlayerState& p1 = state.getPlayer(0);
    const PlayerState& p2 = state.getPlayer(1);
    const PlayerState& p3 = state.getPlayer(2);
    const PlayerState& p4 = state.getPlayer(3);

    // Row 1: Headers (P1 & P2)
    printf("+------------------------------------------+------------------------------------------+\n");
    printf("| %s[P1]%s                       PSN:%-2d TAX:%-2d | %s[P2]%s                       PSN:%-2d TAX:%-2d |\n",
           getPlayerAnsiColor(0), ANSI_RESET, p1.poison, p1.tax,
           getPlayerAnsiColor(1), ANSI_RESET, p2.poison, p2.tax);

    // Row 2: Life Total
    printf("| %s%sLIFE:%4d%s                                 | %s%sLIFE:%4d%s                                 |\n",
           (p1.activeMode == MODE_LIFE) ? "> " : "  ",
           (p1.activeMode == MODE_LIFE) ? getPlayerAnsiColor(0) : ANSI_RESET, p1.life, ANSI_RESET,
           (p2.activeMode == MODE_LIFE) ? "> " : "  ",
           (p2.activeMode == MODE_LIFE) ? getPlayerAnsiColor(1) : ANSI_RESET, p2.life, ANSI_RESET);

    // Row 3: Commander Damage from 3 opponents
    printf("| CMDR DMG: [P2:%02d P3:%02d P4:%02d]             | CMDR DMG: [P1:%02d P3:%02d P4:%02d]             |\n",
           p1.cmdrDmg[1], p1.cmdrDmg[2], p1.cmdrDmg[3],
           p2.cmdrDmg[0], p2.cmdrDmg[2], p2.cmdrDmg[3]);

    // Row 4: Status / Mode
    char p1Stat[40], p2Stat[40];
    if (p1.isDead) {
        char reason[32];
        formatDeadReason(p1, reason, sizeof(reason));
        snprintf(p1Stat, sizeof(p1Stat), "%s[ %s ]%s", ANSI_BG_RED, reason, ANSI_RESET);
    } else if (p1.activeMode == MODE_CMDR) {
        snprintf(p1Stat, sizeof(p1Stat), "Dial: CMDR from P%d", p1.cmdrSource + 1);
    } else {
        snprintf(p1Stat, sizeof(p1Stat), "Dial: %s", GameState::getModeName(p1.activeMode));
    }

    if (p2.isDead) {
        char reason[32];
        formatDeadReason(p2, reason, sizeof(reason));
        snprintf(p2Stat, sizeof(p2Stat), "%s[ %s ]%s", ANSI_BG_RED, reason, ANSI_RESET);
    } else if (p2.activeMode == MODE_CMDR) {
        snprintf(p2Stat, sizeof(p2Stat), "Dial: CMDR from P%d", p2.cmdrSource + 1);
    } else {
        snprintf(p2Stat, sizeof(p2Stat), "Dial: %s", GameState::getModeName(p2.activeMode));
    }
    printf("| %-40s | %-40s |\n", p1Stat, p2Stat);
    printf("+------------------------------------------+------------------------------------------+\n");

    // Row 5: Headers (P3 & P4)
    printf("| %s[P3]%s                       PSN:%-2d TAX:%-2d | %s[P4]%s                       PSN:%-2d TAX:%-2d |\n",
           getPlayerAnsiColor(2), ANSI_RESET, p3.poison, p3.tax,
           getPlayerAnsiColor(3), ANSI_RESET, p4.poison, p4.tax);

    // Row 6: Life Total
    printf("| %s%sLIFE:%4d%s                                 | %s%sLIFE:%4d%s                                 |\n",
           (p3.activeMode == MODE_LIFE) ? "> " : "  ",
           (p3.activeMode == MODE_LIFE) ? getPlayerAnsiColor(2) : ANSI_RESET, p3.life, ANSI_RESET,
           (p4.activeMode == MODE_LIFE) ? "> " : "  ",
           (p4.activeMode == MODE_LIFE) ? getPlayerAnsiColor(3) : ANSI_RESET, p4.life, ANSI_RESET);

    // Row 7: Commander Damage from 3 opponents
    printf("| CMDR DMG: [P1:%02d P2:%02d P4:%02d]             | CMDR DMG: [P1:%02d P2:%02d P3:%02d]             |\n",
           p3.cmdrDmg[0], p3.cmdrDmg[1], p3.cmdrDmg[3],
           p4.cmdrDmg[0], p4.cmdrDmg[1], p4.cmdrDmg[2]);

    // Row 8: Status / Mode
    char p3Stat[40], p4Stat[40];
    if (p3.isDead) {
        char reason[32];
        formatDeadReason(p3, reason, sizeof(reason));
        snprintf(p3Stat, sizeof(p3Stat), "%s[ %s ]%s", ANSI_BG_RED, reason, ANSI_RESET);
    } else if (p3.activeMode == MODE_CMDR) {
        snprintf(p3Stat, sizeof(p3Stat), "Dial: CMDR from P%d", p3.cmdrSource + 1);
    } else {
        snprintf(p3Stat, sizeof(p3Stat), "Dial: %s", GameState::getModeName(p3.activeMode));
    }

    if (p4.isDead) {
        char reason[32];
        formatDeadReason(p4, reason, sizeof(reason));
        snprintf(p4Stat, sizeof(p4Stat), "%s[ %s ]%s", ANSI_BG_RED, reason, ANSI_RESET);
    } else if (p4.activeMode == MODE_CMDR) {
        snprintf(p4Stat, sizeof(p4Stat), "Dial: CMDR from P%d", p4.cmdrSource + 1);
    } else {
        snprintf(p4Stat, sizeof(p4Stat), "Dial: %s", GameState::getModeName(p4.activeMode));
    }
    printf("| %-40s | %-40s |\n", p3Stat, p4Stat);
    printf("+------------------------------------------+------------------------------------------+\n");

    // Event Log Section
    printf("\n%s[ RECENT MATCH LOG ]%s\n", ANSI_BOLD, ANSI_RESET);
    uint16_t logCount = log.getCount();
    if (logCount == 0) {
        printf("  %sNo events recorded yet.%s\n", ANSI_GREY, ANSI_RESET);
    } else {
        uint8_t showCount = (logCount > 4) ? 4 : logCount;
        for (int i = (int)logCount - 1; i >= (int)logCount - (int)showCount; i--) {
            GameEvent evt;
            if (log.getEvent(i, evt)) {
                char str[64];
                EventLog::formatEvent(evt, str, sizeof(str));
                const char* c = (evt.player < NUM_PLAYERS) ? getPlayerAnsiColor(evt.player) : ANSI_YELLOW;
                printf("  %s%s%s\n", c, str, ANSI_RESET);
            }
        }
    }

    printf("\n%sCONTROLS:%s\n", ANSI_BOLD, ANSI_RESET);
    printf("  Adjust Counter: [q / a] P1   [w / s] P2   [e / d] P3   [r / f] P4 (+/-)\n");
    printf("  Cycle Mode:     [1, 2, 3, 4] (Life -> Cmdr Opponents -> Poison -> Tax)\n");
    printf("  Return to Life: [!, @, #, $] (Instant jump back to LIFE)\n");
    printf("  Table Actions:  [SPACE] Roll First Player    [X] Reset Match    [ESC] Exit\n");
    printf("%s====================================================================================%s\n", ANSI_GREY, ANSI_RESET);
}

int main(int argc, char* argv[]) {
    srand((unsigned)time(NULL));

    // Enable VT100 ANSI sequences on Windows console
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif

    EventLog log;
    GameState state(log);
    AppController controller(state, log);

    // If run with --demo or --test, run automated simulation sequence
    if (argc > 1 && (strcmp(argv[1], "--demo") == 0 || strcmp(argv[1], "--test") == 0)) {
        printf("Running Automated CLI Demonstration...\n");
        controller.dispatchAction(0, ACT_DECREMENT, 1000); // P1 -1
        controller.dispatchAction(1, ACT_CYCLE_MODE, 1100); // P2 to Cmdr from P1
        controller.dispatchAction(1, ACT_INCREMENT, 1200); // P2 takes 1 from P1 -> Life decreases to 39
        controller.dispatchAction(2, ACT_CYCLE_MODE, 1300); // P3
        printBoard(state, log, 1500);
        printf("\nDemo completed successfully.\n");
        return 0;
    }

    uint32_t simulatedMs = 0;
    bool running = true;
    printBoard(state, log, simulatedMs);

    while (running) {
#ifdef _WIN32
        if (_kbhit()) {
            char ch = _getch();
            simulatedMs += 250;

            switch (ch) {
                // Adjust counters
                case 'q': controller.dispatchAction(0, ACT_INCREMENT, simulatedMs); break;
                case 'a': controller.dispatchAction(0, ACT_DECREMENT, simulatedMs); break;

                case 'w': controller.dispatchAction(1, ACT_INCREMENT, simulatedMs); break;
                case 's': controller.dispatchAction(1, ACT_DECREMENT, simulatedMs); break;

                case 'e': controller.dispatchAction(2, ACT_INCREMENT, simulatedMs); break;
                case 'd': controller.dispatchAction(2, ACT_DECREMENT, simulatedMs); break;

                case 'r': controller.dispatchAction(3, ACT_INCREMENT, simulatedMs); break;
                case 'f': controller.dispatchAction(3, ACT_DECREMENT, simulatedMs); break;

                // Cycle mode
                case '1': controller.dispatchAction(0, ACT_CYCLE_MODE, simulatedMs); break;
                case '2': controller.dispatchAction(1, ACT_CYCLE_MODE, simulatedMs); break;
                case '3': controller.dispatchAction(2, ACT_CYCLE_MODE, simulatedMs); break;
                case '4': controller.dispatchAction(3, ACT_CYCLE_MODE, simulatedMs); break;

                // Return to life (shift + 1..4)
                case '!': controller.dispatchAction(0, ACT_RETURN_TO_LIFE, simulatedMs); break;
                case '@': controller.dispatchAction(1, ACT_RETURN_TO_LIFE, simulatedMs); break;
                case '#': controller.dispatchAction(2, ACT_RETURN_TO_LIFE, simulatedMs); break;
                case '$': controller.dispatchAction(3, ACT_RETURN_TO_LIFE, simulatedMs); break;

                // Roll first player
                case ' ':
                    controller.rollStartingPlayer(simulatedMs, rand());
                    break;

                // Reset match
                case 'X':
                case 'x':
                    controller.dispatchAction(0, ACT_RESET_GAME, simulatedMs);
                    break;

                // Quit
                case 27: // ESC
                    running = false;
                    break;

                default:
                    break;
            }

            controller.tick(simulatedMs);
            printBoard(state, log, simulatedMs);
        }
        Sleep(30);
        simulatedMs += 30;
        controller.tick(simulatedMs);
#else
        running = false;
#endif
    }

    printf("\nSimulator exited.\n");
    return 0;
}
