#include <Arduino.h>

extern void confirm_led(int confirmledlength, int colorvalR, int colorvalG, int colorvalB);
extern void setLedColor(const char color[]);
extern void all_leds_off();
extern void setPayloadColor(int payloadcolornumber);
extern void full_eeprom_reset();
extern void partial_eeprom_reset();
extern void set_dotstar_brightness();
extern void disable_chip();
extern void wakeup();

#define MASTER_TIMER 13000
#define TRIGGER1 6000
#define TRIGGER2 7000
#define TRIGGER3 8000
#define TRIGGER4 9000
#define TRIGGER5 10000
#define TRIGGER6 11000
#define TRIGGER7 12000


#define WAITON 25000
#define WAITOFF 175000
#define OFF3 500
#define RAPID_BLINK_SPEED_OFF 50
#define RAPID_BLINK_SPEED_ON 10

int pauseInterrupt(const uint32_t uS)
{
  uint32_t uSTimer = 0;
  int res = 0;
  while (uSTimer < uS)
  {
    if (digitalRead(VOLUP_STRAP_PIN) == LOW)
    {
      res = 1;
    }
    if (res) break;
    delayMicroseconds(1);
    ++uSTimer;
  }
  return res;
}

void set_dotstar_brightness() {
#ifdef DOTSTAR_ENABLED
  delayMicroseconds(1000000);

  int currentfade = 0; int i = 0;
  while (currentfade < 3) {
    for (i = 0; i < 251; ++i) {
      if (digitalRead(VOLUP_STRAP_PIN) == LOW) {
        EEPROM_DOTSTAR_BRIGHTNESS = i;
        writeSettings();
        confirmLed("white", 20);
        NVIC_SystemReset();
      }
      strip.setPixelColor(0, i, i, i);
      strip.show();
      delayMicroseconds(20000);
    }
    for (i = 250; i > 0; --i) {
      if (digitalRead(VOLUP_STRAP_PIN) == LOW) {
        EEPROM_DOTSTAR_BRIGHTNESS = i;
        writeSettings();
        confirmLed("white", 20);
        NVIC_SystemReset();
      }
      strip.setPixelColor(0, i, i, i);
      strip.show();
      delayMicroseconds(20000);
    }
    ++currentfade;
  }
  NVIC_SystemReset();
#endif
}

void full_reset() {
  confirmLed("red", 50);
  confirmLed("blue", 50);
EEPROM_EMPTY = 0;
writeSettings();
NVIC_SystemReset();
}



void cycle_bootlogo() {
    delayMicroseconds(1000000);
    
    int ITEM;
    for ( ITEM = 1; ITEM < (LOGOS_AVAILABLE + 1); ++ITEM) {
      bool check_option = false;
      for (int CURRENT_FLASH = 0; CURRENT_FLASH < (ITEM) ; ++CURRENT_FLASH)
      {
        setLedColor("blue");
        if (pauseInterrupt(WAITON) == 1)
        {
          check_option = true;
          break;
        }

        setLedColor("black");
        if (pauseInterrupt(WAITOFF) == 1)
        {
          check_option = true;
          break;
        }
      }
      if (pauseInterrupt(WAITOFF * 2) == 1) check_option = true;
      if (check_option) break;

    }
    EEPROM_BOOTLOGO = ITEM;
    writeSettings();
    confirmLed("white", 20);
    NVIC_SystemReset();
  }

  void cycle_payloads() {
    delayMicroseconds(1000000);
    
    int ITEM;
    for (ITEM = 1; ITEM < (AMOUNT_OF_PAYLOADS + 1); ++ITEM) {
    bool check_option = false;
      for (int CURRENT_FLASH = 0; CURRENT_FLASH < (ITEM) ; ++CURRENT_FLASH) {

        setLedColor("green");
        if (pauseInterrupt(WAITON) == 1)
        {
          check_option = true;
          break;
        }

        setLedColor("black");
        if (pauseInterrupt(WAITOFF) == 1)
        {
          check_option = true;
          break;
        }
      }
      if (pauseInterrupt(WAITOFF * 2) == 1) check_option = true;
      if (check_option) break;
    }
    EEPROM_PAYLOAD_NUMBER = ITEM;
    writeSettings();
    confirmLed("white", 20);
    NVIC_SystemReset();
  }

  void toggle_dual_boot()
  {
    if (EEPROM_DUAL_BOOT_TOGGLE == 1) EEPROM_DUAL_BOOT_TOGGLE = 2;
    else EEPROM_DUAL_BOOT_TOGGLE = 1;
    writeSettings();    
    confirmLed("red", 10);
    NVIC_SystemReset();
  }

  void toggle_screen_saver()
  {
    if (EEPROM_SCREENSAVER_ACTIVE == 1) EEPROM_SCREENSAVER_ACTIVE = 2;
    else EEPROM_SCREENSAVER_ACTIVE = 1;
    writeSettings();    
    confirmLed("orange", 10);
    NVIC_SystemReset();
  }

  void toggle_bitmap_logo()
  {
    if (EEPROM_BITMAP_BOOTLOGO_ON == 1) EEPROM_BITMAP_BOOTLOGO_ON = 2;
    else EEPROM_BITMAP_BOOTLOGO_ON = 1;
    writeSettings();    
    confirmLed("purple", 10);
    NVIC_SystemReset();
  }
  void long_press()

  {


    if (!EEPROM_VOL_CONTROL_STRAP) {
      EEPROM_VOL_CONTROL_STRAP = 1;
      writeSettings();
    }
    uint32_t current_tick = 0;
    while (digitalRead(VOLUP_STRAP_PIN) == LOW)
    {
      ++current_tick;
      if (current_tick > (TRIGGER1 /2) && current_tick < TRIGGER1) setLedColor("red");
      if (current_tick >  TRIGGER1 && current_tick <  TRIGGER2) setLedColor("green");
      if (current_tick >  TRIGGER2 && current_tick <  TRIGGER3) setLedColor("blue");
      if (current_tick >  TRIGGER3 && current_tick <  TRIGGER4) setLedColor("orange");
      if (current_tick >  TRIGGER4 && current_tick <  TRIGGER5) setLedColor("yellow");
      if (current_tick > TRIGGER5 && current_tick <  TRIGGER6) setLedColor("purple");
      if (current_tick > TRIGGER6 && current_tick <  TRIGGER7) setLedColor("white");
      if (current_tick > TRIGGER7)
      {
        
        EEPROM_MODE_NUMBER=EEPROM_MODE_NUMBER+5;
        writeSettings();
        while(true) ledBlink("white", 1, 500000, 500000);
      }

      if (current_tick == TRIGGER1)
      {
        confirmLed("red", 10);
        if (digitalRead(VOLUP_STRAP_PIN) == 1 )toggle_dual_boot();
      }
      if (current_tick == TRIGGER2)
      {
        confirmLed("green", 10);
        if (digitalRead(VOLUP_STRAP_PIN) == 1 )cycle_payloads();
      }
      if (current_tick == TRIGGER3)
      {
        confirmLed("blue", 10);
        if (digitalRead(VOLUP_STRAP_PIN) == 1 )cycle_bootlogo();
      }
      if (current_tick == TRIGGER4)
      {
        confirmLed("orange", 10);
        if (digitalRead(VOLUP_STRAP_PIN) == 1 )toggle_screen_saver();
      }
      if (current_tick == TRIGGER5)
      {
        confirmLed("yellow", 10);
        if (digitalRead(VOLUP_STRAP_PIN) == 1 ) set_dotstar_brightness();
      }
      if (current_tick == TRIGGER6)
      {
        confirmLed("purple", 10);
        if (digitalRead(VOLUP_STRAP_PIN) == 1 )toggle_bitmap_logo();
      }
      delayMicroseconds(1000);
    }
    current_tick = 0;
    setLedColor("black");
  }
