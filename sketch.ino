#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Pins for ESP32 hardware VSPI
#define TFT_CS   21
#define TFT_DC   22
#define TFT_RST  -1

#define BTN_RESET 13

// Player 1 Encoder (Left)
#define P1_CLK 25
#define P1_DT  26
#define P1_SW  27

// Player 3 Encoder (Left)
#define P3_CLK 33
#define P3_DT  14
#define P3_SW  12

// Player 2 Encoder (Right)
#define P2_CLK  5
#define P2_DT  17 // TX2
#define P2_SW  16 // RX2

// Player 4 Encoder (Right)
#define P4_CLK  4
#define P4_DT   2
#define P4_SW  15

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n==================================");
  Serial.println("  ESP32 ILI9341 HELLO WORLD TEST  ");
  Serial.println("==================================");

  // Initialize display
  tft.begin();
  tft.setRotation(1); // Landscape: 320 x 240

  // 1. Fill background with dark blue
  tft.fillScreen(ILI9341_NAVY);

  // 2. Draw border
  tft.drawRect(5, 5, 310, 230, ILI9341_YELLOW);
  tft.drawRect(7, 7, 306, 226, ILI9341_WHITE);

  // 3. Header text
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(45, 30);
  tft.println("HELLO WORLD!");

  // 4. Subtitle
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(35, 75);
  tft.println("MTG Life Tracker");

  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.setCursor(65, 110);
  tft.println("ESP32 Display OK");

  // 5. Instruction line
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(30, 155);
  tft.println("Simulation running smoothly!");

  Serial.println("Display initialized and test pattern drawn.");
}

void loop() {
  // Update a live counter to prove the simulation loop is ticking
  counter++;

  // Clear previous counter box
  tft.fillRect(50, 185, 220, 30, ILI9341_BLACK);
  tft.drawRect(50, 185, 220, 30, ILI9341_DARKGREY);

  tft.setTextColor(ILI9341_MAGENTA);
  tft.setTextSize(2);
  tft.setCursor(65, 192);
  tft.print("Uptime: ");
  tft.print(counter);
  tft.print("s");

  Serial.printf("Heartbeat: %d seconds\n", counter);
  delay(1000);
}

