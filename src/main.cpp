#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"
#include "Adafruit_VEML7700.h"
#include <Arduino_GigaDisplay_GFX.h>

// ——— Color definitions ———
#define GC9A01A_CYAN    0x07FF
#define GC9A01A_RED     0xF800
#define GC9A01A_BLUE    0x001F
#define GC9A01A_GREEN   0x07E0
#define GC9A01A_MAGENTA 0xF81F
#define GC9A01A_WHITE   0xFFFF
#define GC9A01A_BLACK   0x0000
#define GC9A01A_YELLOW  0xFFE0

// 100% bigger than before
#define ICON_SCALE 4    // was 2, now doubled
#define TITLE_FONT   6  // was 3, now doubled

GigaDisplay_GFX     tft;
Adafruit_SHT4x      sht4;
Adafruit_VEML7700   veml;

void drawSeedling(int16_t x, int16_t y, uint16_t color) {
  int16_t rad     = 5 * ICON_SCALE;
  int16_t stemLen = 20 * ICON_SCALE;
  tft.drawLine(x, y + rad, x, y + rad + stemLen, color);
  tft.fillCircle(x - rad, y + rad + stemLen/2, rad, color);
  tft.fillCircle(x + rad, y + rad + stemLen/2, rad, color);
}

void setup() {
  Serial.begin(115200);
  Wire1.begin();

  sht4.begin(&Wire1);
  veml.begin(&Wire1);

  tft.begin();
  tft.setRotation(1);
}

void loop() {
  // 1) read sensors
  sensors_event_t h_event, t_event;
  sht4.getEvent(&h_event, &t_event);
  float lux = veml.readLux();

  // 2) clear the screen
  tft.fillScreen(GC9A01A_BLACK);

  // 3) header: "FoodBox" above, seedling below
  // measure "FoodBox"
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(TITLE_FONT);
  int16_t x0, y0; uint16_t w, h;
  tft.getTextBounds("FoodBox", 0, 0, &x0, &y0, &w, &h);

  // position at top-right with 10px margin
  int16_t textX = tft.width() - w - 10;
  int16_t textY = 5;
  tft.setCursor(textX, textY);
  tft.print("FoodBox");

  // seedling centered under the text
  int16_t iconX = textX + w/2;
  int16_t iconY = textY + h + 5;  // 5px padding
  drawSeedling(iconX, iconY, GC9A01A_GREEN);

  // 4) three big data bands
  uint16_t band = tft.height() / 3;

  // — Temp band —
  tft.setTextColor(GC9A01A_CYAN);
  tft.setTextSize(3);
  tft.setCursor(10, band/4);
  tft.print("Temp");
  tft.setTextSize(6);
  tft.setCursor(10, band/2);
  tft.print(t_event.temperature, 1);
  tft.print(" C");

  // — Humidity band —
  tft.setTextColor(GC9A01A_GREEN);
  tft.setTextSize(3);
  tft.setCursor(10, band + band/4);
  tft.print("Hum");
  tft.setTextSize(6);
  tft.setCursor(10, band + band/2);
  tft.print(h_event.relative_humidity, 1);
  tft.print(" %");

  // — Light band —
  tft.setTextColor(GC9A01A_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(10, 2*band + band/4);
  tft.print("Light");
  tft.setTextSize(6);
  tft.setCursor(10, 2*band + band/2);
  tft.print((int)lux);
  tft.print(" lx");

  delay(2000);
}
