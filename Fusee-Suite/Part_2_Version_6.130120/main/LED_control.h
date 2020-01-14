#include <Arduino.h>

extern uint32_t NEW_DOTSTAR_BRIGHTNESS;


#ifdef DOTSTAR_ENABLED
Adafruit_DotStar strip = Adafruit_DotStar(1, INTERNAL_DS_DATA, INTERNAL_DS_CLK, DOTSTAR_BGR);
#endif

void ledInit() {
  #ifdef DOTSTAR_ENABLED
  strip.begin();
  strip.setPixelColor(0, 0, 0, 0);
  strip.show();
  #endif
  #ifdef ONBOARD_LED
  pinMode(ONBOARD_LED, OUTPUT);
  #endif
}

void setLedColor(const char color[]) {
  #ifdef DOTSTAR_ENABLED
  if (color == "red") {
    strip.setPixelColor(0, EEPROM_DOTSTAR_BRIGHTNESS, 0, 0);
  } else if (color == "green") {
    strip.setPixelColor(0, 0, EEPROM_DOTSTAR_BRIGHTNESS, 0);
  } else if (color == "orange") {
    strip.setPixelColor(0, EEPROM_DOTSTAR_BRIGHTNESS, (EEPROM_DOTSTAR_BRIGHTNESS / 2), 0);
  } else if (color == "yellow") {
    strip.setPixelColor(0, EEPROM_DOTSTAR_BRIGHTNESS, EEPROM_DOTSTAR_BRIGHTNESS, 0);
  } else if (color == "black") {
    strip.setPixelColor(0, 0, 0, 0);
  } else if (color == "blue") {
    strip.setPixelColor(0, 0, 0, EEPROM_DOTSTAR_BRIGHTNESS);
  } else if (color == "white") {
    strip.setPixelColor(0, EEPROM_DOTSTAR_BRIGHTNESS, EEPROM_DOTSTAR_BRIGHTNESS, EEPROM_DOTSTAR_BRIGHTNESS);
  } else {
    strip.setPixelColor(0, 255, 255, 255);
  }
  strip.show();
  #endif
  #ifdef ONBOARD_LED
  if (color == "black") digitalWrite(ONBOARD_LED, LOW);
  if (color != "black") digitalWrite(ONBOARD_LED, HIGH);
  #endif
}

void ledBlink(const char color[], uint32_t count, uint32_t durationon, uint32_t durationoff) {
  for (uint32_t counter = 0; counter < count; counter++) {
    for (int onOff = 1; onOff >= 0; onOff--) {
      if (onOff == 1) {
        setLedColor(color);
        delayMicroseconds(durationon);
      } else {
        setLedColor("black");
        delayMicroseconds(durationoff);
      }
    }
  }
  setLedColor("black");
}

void confirmLed(const char color[], uint32_t count)
{
  ledBlink(color, count, 37500, 37500);
}
