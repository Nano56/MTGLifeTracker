#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Pins for ESP32 hardware VSPI
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

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

