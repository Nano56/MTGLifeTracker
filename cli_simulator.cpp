#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "GameState.h"
#include "EventLog.h"
#include "AppController.h"

// ANSI Color Codes
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_CYAN    "\033[1;36m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_GREY    "\033[90m"
#define ANSI_BG_RED  "\033[41;1;37m"

const char* getPlayerAnsiColor(uint8_t playerIdx) {
    switch (playerIdx) {
        case 0: return ANSI_RED;
        case 1: return ANSI_CYAN;
        case 2: return ANSI_GREEN;
        case 3: return ANSI_YELLOW;
        default: return ANSI_RESET;
    }
}

void printBoard(const GameState& state, const EventLog& log, uint32_t currentMs) {
    // Clear screen / move cursor to top
    printf("\033[H\033[J");

    printf("%s========================================================================%s\n", ANSI_GREY, ANSI_RESET);
    printf("           %sMTG COMMANDER LIFE TRACKER - DESKTOP SIMULATOR%s\n", ANSI_BOLD, ANSI_RESET);
    printf("                   Uptime: %02u:%02u (HID Agnostic)\n", (unsigned)(currentMs / 60000), (unsigned)((currentMs / 1000) % 60));
    printf("%s========================================================================%s\n", ANSI_GREY, ANSI_RESET);

    // Render Quadrants (Dual Hero: Life & Commander Damage side-by-side)
    const PlayerState& p1 = state.getPlayer(0);
    const PlayerState& p2 = state.getPlayer(1);
    const PlayerState& p3 = state.getPlayer(2);
    const PlayerState& p4 = state.getPlayer(3);

    // Row 1: Headers (P1 & P2)
    printf("+-----------------------------------+-----------------------------------+\n");
    printf("| %s[P1]%s                PSN:%-2d TAX:%-2d | %s[P2]%s                PSN:%-2d TAX:%-2d |\n",
           getPlayerAnsiColor(0), ANSI_RESET, p1.poison, p1.tax,
           getPlayerAnsiColor(1), ANSI_RESET, p2.poison, p2.tax);

    // Row 2: Dual Counters (Life & Cmdr Damage side-by-side)
    printf("| %s%sLIFE:%3d%s     %s%sCMDR:%2d%s          | %s%sLIFE:%3d%s     %s%sCMDR:%2d%s          |\n",
           (p1.activeMode == MODE_LIFE) ? "> " : "  ",
           (p1.activeMode == MODE_LIFE) ? getPlayerAnsiColor(0) : ANSI_RESET, p1.life, ANSI_RESET,
           (p1.activeMode == MODE_CMDR) ? "> " : "  ",
           (p1.cmdr <= 0) ? ANSI_RED : ((p1.activeMode == MODE_CMDR) ? ANSI_YELLOW : ANSI_RESET), p1.cmdr, ANSI_RESET,
           (p2.activeMode == MODE_LIFE) ? "> " : "  ",
           (p2.activeMode == MODE_LIFE) ? getPlayerAnsiColor(1) : ANSI_RESET, p2.life, ANSI_RESET,
           (p2.activeMode == MODE_CMDR) ? "> " : "  ",
           (p2.cmdr <= 0) ? ANSI_RED : ((p2.activeMode == MODE_CMDR) ? ANSI_YELLOW : ANSI_RESET), p2.cmdr, ANSI_RESET);

    // Row 3: Status
    char p1Stat[36], p2Stat[36];
    if (p1.isDead) {
        snprintf(p1Stat, sizeof(p1Stat), "%s[ DEAD: %s ]%s", ANSI_BG_RED, (p1.cmdr <= 0) ? "0 CMDR" : (p1.life <= 0 ? "0 LIFE" : "10 POIS"), ANSI_RESET);
    } else {
        snprintf(p1Stat, sizeof(p1Stat), "Dial adjusts: %s", GameState::getModeName(p1.activeMode));
    }
    if (p2.isDead) {
        snprintf(p2Stat, sizeof(p2Stat), "%s[ DEAD: %s ]%s", ANSI_BG_RED, (p2.cmdr <= 0) ? "0 CMDR" : (p2.life <= 0 ? "0 LIFE" : "10 POIS"), ANSI_RESET);
    } else {
        snprintf(p2Stat, sizeof(p2Stat), "Dial adjusts: %s", GameState::getModeName(p2.activeMode));
    }
    printf("| %-33s | %-33s |\n", p1Stat, p2Stat);
    printf("+-----------------------------------+-----------------------------------+\n");

    // Row 4: Headers (P3 & P4)
    printf("| %s[P3]%s                PSN:%-2d TAX:%-2d | %s[P4]%s                PSN:%-2d TAX:%-2d |\n",
           getPlayerAnsiColor(2), ANSI_RESET, p3.poison, p3.tax,
           getPlayerAnsiColor(3), ANSI_RESET, p4.poison, p4.tax);

    // Row 5: Dual Counters (P3 & P4)
    printf("| %s%sLIFE:%3d%s     %s%sCMDR:%2d%s          | %s%sLIFE:%3d%s     %s%sCMDR:%2d%s          |\n",
           (p3.activeMode == MODE_LIFE) ? "> " : "  ",
           (p3.activeMode == MODE_LIFE) ? getPlayerAnsiColor(2) : ANSI_RESET, p3.life, ANSI_RESET,
           (p3.activeMode == MODE_CMDR) ? "> " : "  ",
           (p3.cmdr <= 0) ? ANSI_RED : ((p3.activeMode == MODE_CMDR) ? ANSI_YELLOW : ANSI_RESET), p3.cmdr, ANSI_RESET,
           (p4.activeMode == MODE_LIFE) ? "> " : "  ",
           (p4.activeMode == MODE_LIFE) ? getPlayerAnsiColor(3) : ANSI_RESET, p4.life, ANSI_RESET,
           (p4.activeMode == MODE_CMDR) ? "> " : "  ",
           (p4.cmdr <= 0) ? ANSI_RED : ((p4.activeMode == MODE_CMDR) ? ANSI_YELLOW : ANSI_RESET), p4.cmdr, ANSI_RESET);

    // Row 6: Status (P3 & P4)
    char p3Stat[36], p4Stat[36];
    if (p3.isDead) {
        snprintf(p3Stat, sizeof(p3Stat), "%s[ DEAD: %s ]%s", ANSI_BG_RED, (p3.cmdr <= 0) ? "0 CMDR" : (p3.life <= 0 ? "0 LIFE" : "10 POIS"), ANSI_RESET);
    } else {
        snprintf(p3Stat, sizeof(p3Stat), "Dial adjusts: %s", GameState::getModeName(p3.activeMode));
    }
    if (p4.isDead) {
        snprintf(p4Stat, sizeof(p4Stat), "%s[ DEAD: %s ]%s", ANSI_BG_RED, (p4.cmdr <= 0) ? "0 CMDR" : (p4.life <= 0 ? "0 LIFE" : "10 POIS"), ANSI_RESET);
    } else {
        snprintf(p4Stat, sizeof(p4Stat), "Dial adjusts: %s", GameState::getModeName(p4.activeMode));
    }
    printf("| %-33s | %-33s |\n", p3Stat, p4Stat);
    printf("+-----------------------------------+-----------------------------------+\n");

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

    printf("\n%sCONTROLS (Any HID Simulation):%s\n", ANSI_BOLD, ANSI_RESET);
    printf("  [q / a] P1 (+/-)   [w / s] P2 (+/-)   [e / d] P3 (+/-)   [r / f] P4 (+/-)\n");
    printf("  [1, 2, 3, 4] Cycle Mode (Life -> Poison -> Cmdr -> Tax)\n");
    printf("  [SPACE] Roll First Player    [X] Reset Match    [ESC] Exit Simulator\n");
    printf("%s========================================================================%s\n", ANSI_GREY, ANSI_RESET);
}

int main(int argc, char* argv[]) {
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
        controller.dispatchAction(1, ACT_DECREMENT, 1100); // P2 -1
        controller.dispatchAction(2, ACT_CYCLE_MODE, 1200); // P3 to Poison
        controller.dispatchAction(2, ACT_INCREMENT, 1300); // P3 +1 Poison
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
