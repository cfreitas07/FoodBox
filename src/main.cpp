#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"
#include "Adafruit_VEML7700.h"
#include <Arduino_GigaDisplay_GFX.h>
#include <Arduino_GigaDisplayTouch.h>

// ——— Color definitions ———
#define GC9A01A_CYAN    0x07FF
#define GC9A01A_RED     0xF800
#define GC9A01A_BLUE    0x001F
#define GC9A01A_GREEN   0x07E0
#define GC9A01A_MAGENTA 0xF81F
#define GC9A01A_WHITE   0xFFFF
#define GC9A01A_BLACK   0x0000
#define GC9A01A_YELLOW  0xFFE0
#define GC9A01A_BROWN   0xA145
#define GC9A01A_DARK_BLUE 0x0008
#define GC9A01A_LIGHT_BLUE 0x07FF

// 100% bigger than before
#define ICON_SCALE 3    // was 2, now doubled
#define TITLE_FONT   6  // was 3, now doubled

GigaDisplay_GFX     tft;
Adafruit_SHT4x      sht4;
Adafruit_VEML7700   veml;
Arduino_GigaDisplayTouch touchDetector;

// Button parameters
#define REFRESH_BTN_W 180   // Increased width
#define REFRESH_BTN_H 70    // Increased height
#define REFRESH_BTN_MARGIN 20

bool refreshBtnPressed = false;
bool lastRefreshBtnPressed = false;
unsigned long refreshBtnLastPress = 0;

// Draw a thermometer icon
void drawThermometer(int16_t x, int16_t y, uint16_t color) {
  int16_t width = 8 * ICON_SCALE;
  int16_t height = 20 * ICON_SCALE;
  int16_t bulb = 12 * ICON_SCALE;
  
  // Draw the bulb
  tft.fillCircle(x, y + height, bulb/2, color);
  // Draw the stem
  tft.fillRect(x - width/2, y, width, height, color);
  // Draw the mercury
  tft.fillRect(x - width/4, y + height/4, width/2, height/2, GC9A01A_RED);
}

// Draw a water drop icon
void drawWaterDrop(int16_t x, int16_t y, uint16_t color) {
  int16_t size = 15 * ICON_SCALE;
  tft.fillTriangle(x, y, x - size/2, y + size, x + size/2, y + size, color);
  tft.fillCircle(x, y + size/2, size/4, color);
}

// Draw a sun icon
void drawSun(int16_t x, int16_t y, uint16_t color) {
  int16_t size = 15 * ICON_SCALE;
  tft.fillCircle(x, y, size/2, color);
  // Draw rays
  for(int i = 0; i < 8; i++) {
    float angle = i * PI / 4;
    int16_t x1 = x + cos(angle) * size;
    int16_t y1 = y + sin(angle) * size;
    tft.drawLine(x, y, x1, y1, color);
  }
}

// Draw a soil/plant icon
void drawSoil(int16_t x, int16_t y, uint16_t color) {
  tft.fillRect(x-10, y+10, 20, 8, GC9A01A_BROWN); // soil
  tft.fillCircle(x, y, 8, color); // plant
  tft.drawLine(x, y+8, x, y+14, GC9A01A_GREEN); // stem
}

void drawSeedling(int16_t x, int16_t y, uint16_t color) {
  int16_t rad     = 5 * ICON_SCALE;
  int16_t stemLen = 20 * ICON_SCALE;
  tft.drawLine(x, y + rad, x, y + rad + stemLen, color);
  tft.fillCircle(x - rad, y + rad + stemLen/2, rad, color);
  tft.fillCircle(x + rad, y + rad + stemLen/2, rad, color);
}

// Draw a gradient background
void drawGradientBackground() {
  for(int y = 0; y < tft.height(); y++) {
    uint16_t color = tft.color565(
      0,
      0,
      map(y, 0, tft.height(), 0, 255)
    );
    tft.drawFastHLine(0, y, tft.width(), color);
  }
}

void drawRefreshButton(bool pressed) {
  int btnX = tft.width() - REFRESH_BTN_W - REFRESH_BTN_MARGIN;
  int btnY = tft.height() - REFRESH_BTN_H - REFRESH_BTN_MARGIN;
  uint16_t btnColor = pressed ? GC9A01A_RED : GC9A01A_GREEN;
  tft.fillRoundRect(btnX, btnY, REFRESH_BTN_W, REFRESH_BTN_H, 12, btnColor);
  tft.drawRoundRect(btnX, btnY, REFRESH_BTN_W, REFRESH_BTN_H, 12, GC9A01A_WHITE);
  tft.setTextColor(GC9A01A_BLACK);
  tft.setTextSize(3);
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds("Refresh", 0, 0, &x1, &y1, &w, &h);
  int textX = btnX + (REFRESH_BTN_W - w) / 2;
  int textY = btnY + (REFRESH_BTN_H - h) / 2;
  tft.setCursor(textX, textY);
  tft.print("Refresh");
}

bool isRefreshButtonTouched(int16_t tx, int16_t ty) {
  return (tx > tft.width()/2); // Button is right half of screen
}

void setup() {
  Serial.begin(115200);
  Wire1.begin();

  sht4.begin(&Wire1);
  veml.begin(&Wire1);

  tft.begin();
  tft.setRotation(1);
  touchDetector.begin();

  int btnX = tft.width() - REFRESH_BTN_W - REFRESH_BTN_MARGIN;
  int btnY = tft.height() - REFRESH_BTN_H - REFRESH_BTN_MARGIN;
  Serial.print("Button X: "); Serial.print(btnX); Serial.print(" to "); Serial.println(btnX + REFRESH_BTN_W);
  Serial.print("Button Y: "); Serial.print(btnY); Serial.print(" to "); Serial.println(btnY + REFRESH_BTN_H);
}

void loop() {
  // Touch handling
  uint8_t contacts;
  GDTpoint_t points[5];
  contacts = touchDetector.getTouchPoints(points);
  refreshBtnPressed = false; // Reset at start of loop
  if (contacts > 0) {
    int16_t tx = points[0].y;
    int16_t ty = points[0].x;
    // Debug: print touch coordinates
    Serial.print("Touch: ");
    Serial.print(tx);
    Serial.print(", ");
    Serial.println(ty);
    if (isRefreshButtonTouched(tx, ty)) {
      refreshBtnPressed = true;
    }
  }

  // 1) read sensors
  sensors_event_t h_event, t_event;
  sht4.getEvent(&h_event, &t_event);
  float lux = veml.readLux();

  // 2) clear the screen and draw gradient
  drawGradientBackground();

  // 3) Draw border
  tft.drawRect(0, 0, tft.width(), tft.height(), GC9A01A_WHITE);
  tft.drawRect(1, 1, tft.width()-2, tft.height()-2, GC9A01A_WHITE);

  // 4) header: "FoodBox" and seedling in top right
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(TITLE_FONT);
  int16_t x0, y0; uint16_t w, h;
  tft.getTextBounds("FoodBox", 0, 0, &x0, &y0, &w, &h);

  // position at top-right with margin
  int16_t textX = tft.width() - w - 20;
  int16_t textY = 20;
  tft.setCursor(textX, textY);
  tft.print("FoodBox");

  // seedling centered under the text
  int16_t iconX = textX + w/2;
  int16_t iconY = textY + h + 10;  // 10px padding
  drawSeedling(iconX, iconY, GC9A01A_GREEN);

  // 5) Calculate available space for data bands
  uint16_t headerHeight = iconY + 40; // Total height of header section
  uint16_t availableHeight = tft.height() - headerHeight - 20; // 20px bottom margin
  uint16_t band = availableHeight / 4;
  uint16_t startY = headerHeight + 10; // 10px padding after header

  // --- Data bands: start just below header/logo, icons left, values right ---
  const int bandCount = 4;
  int bandHeight = 40; // Fixed height for each band
  int extraSpacing = 65; // Extra space between bands
  int iconColX = 30;
  int labelColX = 70;
  int valueColX = 260;
  int firstBandY = iconY + 20; // Start just below the logo, with a small margin

  // Temperature
  int yTemp = firstBandY;
  drawThermometer(iconColX, yTemp, GC9A01A_CYAN);
  tft.setTextColor(GC9A01A_CYAN);
  tft.setTextSize(3);
  tft.setCursor(labelColX, yTemp);
  tft.print("Temp");
  tft.setTextSize(6);
  tft.setCursor(valueColX, yTemp);
  tft.print(t_event.temperature, 1);
  tft.print(" C");

  // Humidity
  int yHum = yTemp + bandHeight + extraSpacing;
  drawWaterDrop(iconColX, yHum, GC9A01A_GREEN);
  tft.setTextColor(GC9A01A_GREEN);
  tft.setTextSize(3);
  tft.setCursor(labelColX, yHum);
  tft.print("Humidity");
  tft.setTextSize(6);
  tft.setCursor(valueColX, yHum);
  tft.print(h_event.relative_humidity, 1);
  tft.print(" %");

  // Light
  int yLight = yHum + bandHeight + extraSpacing;
  drawSun(iconColX, yLight, GC9A01A_YELLOW);
  tft.setTextColor(GC9A01A_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(labelColX, yLight);
  tft.print("Light");
  tft.setTextSize(6);
  tft.setCursor(valueColX, yLight);
  tft.print((int)lux);
  tft.print(" lx");

  // Soil Moisture (placeholder)
  int ySoil = yLight + bandHeight + extraSpacing;
  drawSoil(iconColX, ySoil, GC9A01A_GREEN);
  tft.setTextColor(GC9A01A_MAGENTA);
  tft.setTextSize(3);
  tft.setCursor(labelColX, ySoil);
  tft.print("Soil");
  tft.setTextSize(6);
  tft.setCursor(valueColX, ySoil);
  tft.print("-- %"); // Placeholder for future sensor

  // Always draw the button last so it appears on top
  drawRefreshButton(refreshBtnPressed);

  delay(100); // Optionally, increase for less flicker
}
