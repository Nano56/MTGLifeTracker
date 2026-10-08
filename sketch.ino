#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ==========================================
// PIN DEFINITIONS (From HARDWARE_SPEC.md)
// ==========================================
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

#define BTN_RESET 17

// Player 1 Encoder
#define P1_CLK 25
#define P1_DT  26
#define P1_SW  27

// Player 2 Encoder
#define P2_CLK 13
#define P2_DT  14
#define P2_SW  32

// Player 3 Encoder (GPI 34, 35 are input-only)
#define P3_CLK 33
#define P3_DT  34
#define P3_SW  35

// Player 4 Encoder (GPI 36, 39 are input-only)
#define P4_CLK 36
#define P4_DT  39
#define P4_SW  16

// Display dimensions
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Colors (16-bit RGB565)
#define COLOR_BG        ILI9341_BLACK
#define COLOR_GRID      0x4208 // Dark Gray
#define COLOR_P1        ILI9341_RED
#define COLOR_P2        ILI9341_CYAN
#define COLOR_P3        ILI9341_GREEN
#define COLOR_P4        ILI9341_YELLOW
#define COLOR_WHITE     ILI9341_WHITE
#define COLOR_POISON    0xD01F // Magenta/Purple
#define COLOR_CMDR      0xFD20 // Orange

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Counter modes per player
enum CounterMode {
  MODE_LIFE = 0,
  MODE_POISON,
  MODE_CMDR_DMG,
  MODE_COUNT
};

struct Player {
  int life;
  int poison;
  int cmdr_dmg;
  CounterMode current_mode;
  uint16_t theme_color;
  int last_clk;
  int last_sw;
  unsigned long last_sw_debounce;
};

Player players[4];

// Quadrant layout bounds (x, y, w, h)
const int quad_x[4] = { 0, 160, 0, 160 };
const int quad_y[4] = { 0, 0, 120, 120 };
const int quad_w = 160;
const int quad_h = 120;

void drawQuadrant(int p_idx, bool full_redraw = false) {
  int x = quad_x[p_idx];
  int y = quad_y[p_idx];
  Player &p = players[p_idx];

  if (full_redraw) {
    // Fill quadrant background
    tft.fillRect(x + 1, y + 1, quad_w - 2, quad_h - 2, COLOR_BG);

    // Player header badge
    tft.fillRect(x + 4, y + 4, quad_w - 8, 20, p.theme_color);
    tft.setTextColor(COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(x + 10, y + 10);
    tft.print("PLAYER ");
    tft.print(p_idx + 1);
  }

  // Clear value area
  tft.fillRect(x + 10, y + 30, quad_w - 20, 50, COLOR_BG);

  // Counter mode label
  tft.setTextSize(1);
  tft.setCursor(x + 10, y + 32);
  switch (p.current_mode) {
    case MODE_LIFE:
      tft.setTextColor(COLOR_WHITE);
      tft.print("LIFE TOTAL");
      break;
    case MODE_POISON:
      tft.setTextColor(COLOR_POISON);
      tft.print("POISON (Max 10)");
      break;
    case MODE_CMDR_DMG:
      tft.setTextColor(COLOR_CMDR);
      tft.print("CMDR DMG (Max 21)");
      break;
    default:
      break;
  }

  // Display the active value
  int val = 0;
  uint16_t val_color = COLOR_WHITE;
  if (p.current_mode == MODE_LIFE) {
    val = p.life;
    val_color = (p.life <= 0) ? ILI9341_DARKGREY : p.theme_color;
  } else if (p.current_mode == MODE_POISON) {
    val = p.poison;
    val_color = COLOR_POISON;
  } else {
    val = p.cmdr_dmg;
    val_color = COLOR_CMDR;
  }

  // Draw large number
  tft.setTextColor(val_color);
  tft.setTextSize(4);
  tft.setCursor(x + 35, y + 48);
  if (val < 10 && val >= 0) tft.print(" ");
  tft.print(val);

  // Status footer
  tft.setTextSize(1);
  tft.setCursor(x + 10, y + 100);
  tft.setTextColor(ILI9341_DARKGREY);
  tft.print("P");
  tft.print(p.poison);
  tft.print(" | CD");
  tft.print(p.cmdr_dmg);
}

void drawGrid() {
  tft.fillScreen(COLOR_BG);
  // Horizontal dividing line
  tft.drawFastHLine(0, 120, SCREEN_WIDTH, COLOR_GRID);
  // Vertical dividing line
  tft.drawFastVLine(160, 0, SCREEN_HEIGHT, COLOR_GRID);

  for (int i = 0; i < 4; i++) {
    drawQuadrant(i, true);
  }
}

void resetGame() {
  for (int i = 0; i < 4; i++) {
    players[i].life = 40;
    players[i].poison = 0;
    players[i].cmdr_dmg = 0;
    players[i].current_mode = MODE_LIFE;
  }
  drawGrid();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting MTG Commander Life Tracker...");

  // Display initialization
  tft.begin();
  tft.setRotation(1); // Landscape mode: 320x240

  // Configure Reset Button
  pinMode(BTN_RESET, INPUT_PULLUP);

  // Player themes
  players[0].theme_color = COLOR_P1;
  players[1].theme_color = COLOR_P2;
  players[2].theme_color = COLOR_P3;
  players[3].theme_color = COLOR_P4;

  // Pin setup for Player 1
  pinMode(P1_CLK, INPUT_PULLUP);
  pinMode(P1_DT,  INPUT_PULLUP);
  pinMode(P1_SW,  INPUT_PULLUP);

  // Pin setup for Player 2
  pinMode(P2_CLK, INPUT_PULLUP);
  pinMode(P2_DT,  INPUT_PULLUP);
  pinMode(P2_SW,  INPUT_PULLUP);

  // Pin setup for Player 3 (34, 35 are input-only without internal pullups)
  pinMode(P3_CLK, INPUT_PULLUP);
  pinMode(P3_DT,  INPUT);
  pinMode(P3_SW,  INPUT);

  // Pin setup for Player 4 (36, 39 are input-only without internal pullups)
  pinMode(P4_CLK, INPUT);
  pinMode(P4_DT,  INPUT);
  pinMode(P4_SW,  INPUT_PULLUP);

  // Initialize player encoder states
  players[0].last_clk = digitalRead(P1_CLK);
  players[0].last_sw  = digitalRead(P1_SW);

  players[1].last_clk = digitalRead(P2_CLK);
  players[1].last_sw  = digitalRead(P2_SW);

  players[2].last_clk = digitalRead(P3_CLK);
  players[2].last_sw  = digitalRead(P3_SW);

  players[3].last_clk = digitalRead(P4_CLK);
  players[3].last_sw  = digitalRead(P4_SW);

  resetGame();
}

void handleEncoder(int p_idx, int clk_pin, int dt_pin, int sw_pin) {
  Player &p = players[p_idx];

  // 1. Read Rotation (CLK & DT)
  int current_clk = digitalRead(clk_pin);
  if (current_clk != p.last_clk && current_clk == LOW) {
    int dt_val = digitalRead(dt_pin);
    int delta = (dt_val != current_clk) ? 1 : -1;

    if (p.current_mode == MODE_LIFE) {
      p.life += delta;
    } else if (p.current_mode == MODE_POISON) {
      p.poison += delta;
      if (p.poison < 0) p.poison = 0;
    } else if (p.current_mode == MODE_CMDR_DMG) {
      p.cmdr_dmg += delta;
      if (p.cmdr_dmg < 0) p.cmdr_dmg = 0;
    }

    drawQuadrant(p_idx, false);
  }
  p.last_clk = current_clk;

  // 2. Read Switch Click (SW with debounce)
  int current_sw = digitalRead(sw_pin);
  if (current_sw != p.last_sw) {
    if (current_sw == LOW && (millis() - p.last_sw_debounce > 200)) {
      // Cycle mode: LIFE -> POISON -> CMDR -> LIFE
      p.current_mode = (CounterMode)((p.current_mode + 1) % MODE_COUNT);
      p.last_sw_debounce = millis();
      drawQuadrant(p_idx, false);
    }
    p.last_sw = current_sw;
  }
}

void loop() {
  // Check global Reset Button
  if (digitalRead(BTN_RESET) == LOW) {
    delay(50); // Simple debounce
    if (digitalRead(BTN_RESET) == LOW) {
      resetGame();
      while (digitalRead(BTN_RESET) == LOW) {
        delay(10);
      }
    }
  }

  // Poll all 4 player encoders
  handleEncoder(0, P1_CLK, P1_DT, P1_SW);
  handleEncoder(1, P2_CLK, P2_DT, P2_SW);
  handleEncoder(2, P3_CLK, P3_DT, P3_SW);
  handleEncoder(3, P4_CLK, P4_DT, P4_SW);
}

