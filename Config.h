#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ==========================================
// PIN ALLOCATIONS (ESP32 DevKit & KY-040)
// Matching HARDWARE_SPEC.md & diagram.json
// ==========================================

// Display (ILI9341 SPI)
#define TFT_CS      21
#define TFT_DC      22
#define TFT_RST     -1
#define TFT_SCK     18
#define TFT_MOSI    23
#define TFT_MISO    19

// Global Table Reset & Roll Button
#define BTN_RESET   13

// Player 1 Controls (Top-Left)
#define P1_CLK      25
#define P1_DT       26
#define P1_SW       27

// Player 2 Controls (Top-Right)
#define P2_CLK      5
#define P2_DT       17 // TX2
#define P2_SW       16 // RX2

// Player 3 Controls (Bottom-Left)
#define P3_CLK      33
#define P3_DT       14
#define P3_SW       12

// Player 4 Controls (Bottom-Right)
#define P4_CLK      4
#define P4_DT       2
#define P4_SW       15

// ==========================================
// GAME RULES CONSTANTS
// ==========================================
#define NUM_PLAYERS          4
#define STARTING_LIFE        40
#define STARTING_CMDR_HEALTH 21
#define LETHAL_POISON        10
#define CMDR_TAX_STEP        2

// ==========================================
// TIMING & INPUT THRESHOLDS
// ==========================================
#define DEBOUNCE_DELAY_MS      35
#define LONG_PRESS_MS          1500
#define DOUBLE_CLICK_WINDOW_MS 350
#define AUTO_RETURN_LIFE_MS    0 // Set to 0 to disable auto-return; modes stay until explicitly cycled

// ==========================================
// 16-BIT RGB565 DISPLAY PALETTE
// ==========================================
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_DARKGREY      0x3186
#define COLOR_LIGHTGREY     0xCE59
#define COLOR_DIVIDER       0x2104
#define COLOR_DEAD_BG       0x3000
#define COLOR_DEAD_TEXT     0xF800

// Player Colors
#define COLOR_P1            0xF986 // Bright Red
#define COLOR_P2            0x067F // Vibrant Cyan / Azure
#define COLOR_P3            0x2666 // Emerald Green
#define COLOR_P4            0xFDE0 // Warm Amber / Gold

// Display Geometry
#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       240
#define QUAD_WIDTH          160
#define QUAD_HEIGHT         120

#endif // CONFIG_H

