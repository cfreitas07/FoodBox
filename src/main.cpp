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

bool sht4_connected = false;
bool veml_connected = false;

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

void tryInitSensors() {
  sht4_connected = sht4.begin(&Wire1);
  veml_connected = veml.begin(&Wire1);
}

void drawStaticUI() {
  drawGradientBackground();
  tft.drawRect(0, 0, tft.width(), tft.height(), GC9A01A_WHITE);
  tft.drawRect(1, 1, tft.width()-2, tft.height()-2, GC9A01A_WHITE);

  // Header
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(TITLE_FONT);
  int16_t x0, y0; uint16_t w, h;
  tft.getTextBounds("FoodBox", 0, 0, &x0, &y0, &w, &h);
  int16_t textX = tft.width() - w - 20;
  int16_t textY = 20;
  tft.setCursor(textX, textY);
  tft.print("FoodBox");
  int16_t iconX = textX + w/2;
  int16_t iconY = textY + h + 10;
  drawSeedling(iconX, iconY, GC9A01A_GREEN);

  // Calculate y-coordinates for value bands
  int bandHeight = 40;
  int extraSpacing = 65;
  int firstBandY = iconY + 20;
  int yTemp = firstBandY;
  int yHum = yTemp + bandHeight + extraSpacing;
  int yLight = yHum + bandHeight + extraSpacing;
  int ySoil = yLight + bandHeight + extraSpacing;
  int iconColX = 30;
  int labelColX = 70;

  // Temperature
  drawThermometer(iconColX, yTemp, GC9A01A_CYAN);
  tft.setTextColor(GC9A01A_CYAN);
  tft.setTextSize(3);
  tft.setCursor(labelColX, yTemp);
  tft.print("Temp");

  // Humidity
  drawWaterDrop(iconColX, yHum, GC9A01A_GREEN);
  tft.setTextColor(GC9A01A_GREEN);
  tft.setTextSize(3);
  tft.setCursor(labelColX, yHum);
  tft.print("Humidity");

  // Light
  drawSun(iconColX, yLight, GC9A01A_YELLOW);
  tft.setTextColor(GC9A01A_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(labelColX, yLight);
  tft.print("Light");

  // Soil (placeholder)
  drawSoil(iconColX, ySoil, GC9A01A_GREEN);
  tft.setTextColor(GC9A01A_MAGENTA);
  tft.setTextSize(3);
  tft.setCursor(labelColX, ySoil);
  tft.print("Soil");
}

void updateValues(float temp, float hum, float lux, bool sht4_connected, bool veml_connected, int yTemp, int yHum, int yLight, int ySoil) {
  int valueColX = 260;

  // Temperature
  tft.fillRect(valueColX, yTemp, 120, 50, GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_CYAN);
  tft.setTextSize(6);
  tft.setCursor(valueColX, yTemp);
  if (sht4_connected && !isnan(temp)) {
    tft.print(temp, 1); tft.print(" C");
  } else {
    tft.print("--");
  }

  // Humidity
  tft.fillRect(valueColX, yHum, 120, 50, GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_GREEN);
  tft.setTextSize(6);
  tft.setCursor(valueColX, yHum);
  if (sht4_connected && !isnan(hum)) {
    tft.print(hum, 1); tft.print(" %");
  } else {
    tft.print("--");
  }

  // Light
  tft.fillRect(valueColX, yLight, 120, 50, GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_YELLOW);
  tft.setTextSize(6);
  tft.setCursor(valueColX, yLight);
  if (veml_connected && !isnan(lux)) {
    tft.print((int)lux); tft.print(" lx");
  } else {
    tft.print("--");
  }

  // Soil (placeholder)
  tft.fillRect(valueColX, ySoil, 120, 50, GC9A01A_BLACK);
  tft.setTextColor(GC9A01A_MAGENTA);
  tft.setTextSize(6);
  tft.setCursor(valueColX, ySoil);
  tft.print("-- %");
}

void setup() {
  Serial.begin(115200);
  Wire1.begin();

  tryInitSensors();

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
  static bool lastRefreshBtnState = false;
  bool doRefresh = false;
  if (contacts > 0) {
    int16_t tx = points[0].y;
    int16_t ty = points[0].x;
    if (isRefreshButtonTouched(tx, ty)) {
      refreshBtnPressed = true;
      if (!lastRefreshBtnState) doRefresh = true; // Only trigger on new press
    }
  }
  lastRefreshBtnState = refreshBtnPressed;

  // If refresh button pressed, try to re-init sensors
  if (doRefresh) {
    sht4_connected = sht4.begin(&Wire1);
    veml_connected = veml.begin(&Wire1);
  }

  // 1) read sensors
  sensors_event_t h_event, t_event;
  float temp = NAN, hum = NAN, lux = NAN;

  // Only call getEvent if sensor is marked as connected
  if (sht4_connected) {
    if (!sht4.getEvent(&h_event, &t_event)) {
      sht4_connected = false; // Mark as disconnected if call fails
    } else {
      temp = t_event.temperature;
      hum = h_event.relative_humidity;
    }
  }
  if (veml_connected) {
    lux = veml.readLux();
    if (isnan(lux) || lux == 0) {
      // Optionally, try to mark as disconnected if you know the failure mode
      // veml_connected = false;
    }
  }

  static bool needFullRedraw = true;
  if (doRefresh) {
    tryInitSensors();
    needFullRedraw = true;
  }

  // Calculate y-coordinates for value bands
  // Header calculation (must match drawStaticUI)
  tft.setTextSize(TITLE_FONT);
  int16_t x0, y0; uint16_t w, h;
  tft.getTextBounds("FoodBox", 0, 0, &x0, &y0, &w, &h);
  int16_t textX = tft.width() - w - 20;
  int16_t textY = 20;
  int16_t iconX = textX + w/2;
  int16_t iconY = textY + h + 10;
  int bandHeight = 40;
  int extraSpacing = 65;
  int firstBandY = iconY + 20;
  int yTemp = firstBandY;
  int yHum = yTemp + bandHeight + extraSpacing;
  int yLight = yHum + bandHeight + extraSpacing;
  int ySoil = yLight + bandHeight + extraSpacing;

  if (needFullRedraw) {
    drawStaticUI();
    needFullRedraw = false;
  }

  updateValues(temp, hum, lux, sht4_connected, veml_connected, yTemp, yHum, yLight, ySoil);
  drawRefreshButton(refreshBtnPressed);

  delay(100);
}
