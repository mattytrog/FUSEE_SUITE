#include <Usb.h>
#include "settings.h"
#include "virtual_eeprom.h"
#include "FS_misc.h"
#include "PL1.h"

#include "usb_int.h"

#include <Arduino.h>

unsigned long currentTime = 0;
unsigned long lastCheckTime = 0;
unsigned long startTime = 0;
static bool write_to_eeprom = false;
uint8_t STG_TIMEOUT;

#ifdef DOTSTAR_ENABLED
#include <Adafruit_DotStar.h>
Adafruit_DotStar strip
  = Adafruit_DotStar (1, INTERNAL_DS_DATA, INTERNAL_DS_CLK, DOTSTAR_BGR);
#endif

#ifdef NEOPIXEL_ENABLED
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(BOARD_NEOPIXEL_COUNT, BOARD_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
#endif

void
ledInit ()
{
#ifdef DOTSTAR_ENABLED
  strip.begin ();
  strip.setPixelColor (0, 0, 0, 0);
  strip.show ();
#endif

#ifdef NEOPIXEL_ENABLED
  pixels.begin ();
  pixels.setPixelColor(1, pixels.Color(0, 0, 0));
  pixels.show();
#endif

#ifdef ONBOARD_LED
  pinMode (ONBOARD_LED, OUTPUT);
#endif
}

void
setLedColor (const char color[])
{

  if (!strcmp (color, "red"))
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, EEPROM_DOTSTAR_BRIGHTNESS, 0, 0);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(EEPROM_DOTSTAR_BRIGHTNESS, 0, 0));
#endif
  }
  else if (!strcmp (color, "green"))
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, 0, EEPROM_DOTSTAR_BRIGHTNESS, 0);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(0, EEPROM_DOTSTAR_BRIGHTNESS, 0));
#endif
  }
  else if (!strcmp (color, "orange"))
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, EEPROM_DOTSTAR_BRIGHTNESS,
                         (EEPROM_DOTSTAR_BRIGHTNESS / 2), 0);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(EEPROM_DOTSTAR_BRIGHTNESS,
                                         (EEPROM_DOTSTAR_BRIGHTNESS / 2), 0));
#endif
  }
  else if (!strcmp (color, "yellow"))
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, EEPROM_DOTSTAR_BRIGHTNESS,
                         EEPROM_DOTSTAR_BRIGHTNESS, 0);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(EEPROM_DOTSTAR_BRIGHTNESS,
                                         EEPROM_DOTSTAR_BRIGHTNESS, 0));
#endif
  }
  else if (!strcmp (color, "purple"))
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, EEPROM_DOTSTAR_BRIGHTNESS, 0,
                         EEPROM_DOTSTAR_BRIGHTNESS);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(EEPROM_DOTSTAR_BRIGHTNESS, 0,
                                         EEPROM_DOTSTAR_BRIGHTNESS));
#endif
  }
  else if (!strcmp (color, "black"))
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, 0, 0, 0);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
#endif
  }
  else if (!strcmp (color, "blue"))
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, 0, 0, EEPROM_DOTSTAR_BRIGHTNESS);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(0, 0, EEPROM_DOTSTAR_BRIGHTNESS));
#endif
  }
  else if (!strcmp (color, "white"))
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, EEPROM_DOTSTAR_BRIGHTNESS,
                         EEPROM_DOTSTAR_BRIGHTNESS,
                         EEPROM_DOTSTAR_BRIGHTNESS);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(EEPROM_DOTSTAR_BRIGHTNESS,
                                         EEPROM_DOTSTAR_BRIGHTNESS,
                                         EEPROM_DOTSTAR_BRIGHTNESS));
#endif
  }
  else
  {
#ifdef DOTSTAR_ENABLED
    strip.setPixelColor (0, 255, 255, 255);
#endif
#ifdef NEOPIXEL_ENABLED
    pixels.setPixelColor(0, pixels.Color(255, 255, 255));
#endif
  }
#ifdef DOTSTAR_ENABLED
  strip.show ();
#endif
#ifdef NEOPIXEL_ENABLED
  pixels.show ();
#endif

#ifdef ONBOARD_LED
  if (!strcmp (color, "black"))
    digitalWrite (ONBOARD_LED, LOW);
  else
    digitalWrite (ONBOARD_LED, HIGH);
#endif
}

void
ledBlink (const char color[], uint32_t count, uint32_t durationon,
          uint32_t durationoff)
{
  for (uint32_t counter = 0; counter < count; counter++)
  {
    for (int onOff = 1; onOff >= 0; onOff--)
    {
      if (onOff == 1)
      {
        setLedColor (color);
        delayMicroseconds (durationon);
      }
      else
      {
        setLedColor ("black");
        delayMicroseconds (durationoff);
      }
    }
  }
  setLedColor ("black");
}

void
confirmLed (const char color[], uint32_t count)
{
  ledBlink (color, count, 37500, 37500);
}

int
pauseInterrupt (const uint32_t uS)
{
  #ifdef VOLUP_STRAP_PIN
  uint32_t uSTimer = 0;
  int res = 0;
  while (uSTimer < uS)
  {
    if (digitalRead (VOLUP_STRAP_PIN) == LOW)
    {
      res = 1;
    }
    if (res)
      break;
    delayMicroseconds (1);
    ++uSTimer;
  }
  return res;
  #endif
}


void
toggle_dual_boot_safe ()
{
  if (EEPROM_DUAL_BOOT_TOGGLE == 1)
    EEPROM_DUAL_BOOT_TOGGLE = 3;
  else
    EEPROM_DUAL_BOOT_TOGGLE = 1;
  write_to_eeprom = true;
  // NVIC_SystemReset();
}



void
toggle_dual_boot ()
{
  if (EEPROM_DUAL_BOOT_TOGGLE == 1)
    EEPROM_DUAL_BOOT_TOGGLE = 2;
  else
    EEPROM_DUAL_BOOT_TOGGLE = 1;
  write_to_eeprom = true;
  // NVIC_SystemReset();
}


void
toggle_triple_boot ()
{
  EEPROM_DUAL_BOOT_TOGGLE += 1;
  if (EEPROM_DUAL_BOOT_TOGGLE > 3) EEPROM_DUAL_BOOT_TOGGLE = 1;
  write_to_eeprom = true;
  // NVIC_SystemReset();
}


void
full_reset ()
{
  confirmLed ("white", 10);
  
  writeSettings (true);
  
  NVIC_SystemReset();
}

void set_dotstar_brightness ()
{
#ifdef VOLUP_STRAP_PIN
  delayMicroseconds (1000000);

  int currentfade = 0;
  int i = 0;
  int res = 0;
  while (currentfade < 3)
  {
    if (!res)
    {
      for (i = 0; i <= 249; ++i)
      {
        if (digitalRead (VOLUP_STRAP_PIN) == LOW)
        {
          EEPROM_DOTSTAR_BRIGHTNESS = i;
          write_to_eeprom = true;
          confirmLed ("white", 20);
          res = 1;
          break;
          // NVIC_SystemReset();
        }
#ifdef DOTSTAR_ENABLED
        strip.setPixelColor (0, i, i, i);
        strip.show ();
        delayMicroseconds (5000);
#endif
#ifdef NEOPIXEL_ENABLED
        pixels.setPixelColor(0, pixels.Color(i, i, i));
        pixels.show();
        delayMicroseconds (5000);
#endif
      }
    }

    if (!res)
    {
      for (i = 0; i <= 20; ++i)
      {
        if (digitalRead (VOLUP_STRAP_PIN) == LOW)
        {
          EEPROM_DOTSTAR_BRIGHTNESS = 249;
          write_to_eeprom = true;
          confirmLed ("white", 20);
          res = 1;
          break;
          // NVIC_SystemReset();
        }
#ifdef DOTSTAR_ENABLED
        strip.setPixelColor (0, 249, 249, 249);
        strip.show ();
        delayMicroseconds (20000);
        strip.setPixelColor (0, 0, 0, 0);
        strip.show ();
        delayMicroseconds (20000);
#endif

#ifdef NEOPIXEL_ENABLED
        pixels.setPixelColor(0, pixels.Color(249, 249, 249));
        pixels.show();
        delayMicroseconds (20000);
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
        delayMicroseconds (20000);
#endif
      }
    }
    if (!res)
    {
      for (i = 249; i >= 0; --i)
      {
        if (digitalRead (VOLUP_STRAP_PIN) == LOW)
        {
          EEPROM_DOTSTAR_BRIGHTNESS = i;
          write_to_eeprom = true;
          confirmLed ("white", 20);
          res = 1;
          break;
          // NVIC_SystemReset();
        }
#ifdef DOTSTAR_ENABLED
        strip.setPixelColor (0, i, i, i);
        strip.show ();
        delayMicroseconds (5000);
#endif
#ifdef NEOPIXEL_ENABLED
        pixels.setPixelColor(0, pixels.Color(i, i, i));
        pixels.show();
        delayMicroseconds (5000);
#endif
      }
    }
    ++currentfade;
    // if(res) return;
  }
  // NVIC_SystemReset();
#endif
}

void reset_to_bootloader()
{
  *DBL_TAP_PTR = DBL_TAP_MAGIC;
  NVIC_SystemReset();
}

void
toggle_settings_method ()
{
  if (EEPROM_SETTINGS_CHANGE == 0)
    EEPROM_SETTINGS_CHANGE = 1;
  else
    EEPROM_SETTINGS_CHANGE = 0;
  write_to_eeprom = true;
  //NVIC_SystemReset();
}

void
toggle_colour ()
{
  if (EEPROM_COLOUR == 0)
    EEPROM_COLOUR = 1;
  else
    EEPROM_COLOUR = 0;
  write_to_eeprom = true;
  //NVIC_SystemReset();
}




int
get_btn_res (const char releasetime, const char confirmtime)
{
  #ifdef VOLUP_STRAP_PIN
  char btn_count = 0;
  unsigned long ticktick = ((millis() / 100) * 105);
  unsigned long newticktick = (ticktick + releasetime);


  while (true)
  {
    ticktick = ((millis() / 100) * 105);
    if (!(digitalRead (VOLUP_STRAP_PIN)))
    {

      newticktick = ((ticktick + confirmtime) + TIME_TUNE);
      ledBlink ("red", 1, 10000, 90000);

      ++btn_count;
      while (!(digitalRead (VOLUP_STRAP_PIN))) {long_press(2000);}

    }

    if (ticktick >= newticktick + releasetime) break;


  }
  //

  return btn_count;
  #endif
}

void
multiple_press ()
{
#ifdef VOLUP_STRAP_PIN
  if (!EEPROM_VOL_CONTROL_STRAP)
  {
    EEPROM_VOL_CONTROL_STRAP = 1;
    write_to_eeprom = true;
  }

  char btn = 0;

  btn = (get_btn_res (SELECTION_RELEASE_TIME, SELECTION_CONFIRM_TIME));
  //ledBlink ("red", btn, 10000, 500000);
  bool act = false;

  int TOTAL_BUTTON_COUNT = (SAFETY_PRESS_BARRIER + (OPTIONS_TO_SHOW * BUTTON_GAP));
  int res [TOTAL_BUTTON_COUNT];

  memset(res, 0, TOTAL_BUTTON_COUNT * sizeof(int) );
  int j = 0, i = 0;
  //for (i = 0; i <= (TOTAL_BUTTON_COUNT); ++i){i = 0;}
  for (i = SAFETY_PRESS_BARRIER; i <= (TOTAL_BUTTON_COUNT); i += BUTTON_GAP)
  {
    res[j] = i;
    j += 1;
  }
  if (btn < SAFETY_PRESS_BARRIER) return;
  //if(btn > res[14]) return;

  if (btn == res[0]) {
    confirmLed ("red", 10);
    if ((EEPROM_BOOT_OPTIONS_AVAILABLE < 1) || (EEPROM_BOOT_OPTIONS_AVAILABLE > 3)) {
      EEPROM_BOOT_OPTIONS_AVAILABLE = 2;  //Sanity check
      write_to_eeprom = true;
    }
    if (EEPROM_BOOT_OPTIONS_AVAILABLE == 1) toggle_dual_boot ();
    if (EEPROM_BOOT_OPTIONS_AVAILABLE == 2) toggle_dual_boot_safe ();
    if (EEPROM_BOOT_OPTIONS_AVAILABLE == 3) toggle_triple_boot ();

    act = true;
  }

  else if (btn == res[1]) {
    confirmLed ("green", 10);
    toggle_colour();

    act = true;
  }

  else if (btn == res[2]) {
    confirmLed ("blue", 10);
    set_dotstar_brightness ();
    act = true;

  }

  else if (btn == res[3]) {
    confirmLed ("red", 10);
    toggle_settings_method ();

    act = true;
  }

  else if (btn == res[4]) {
    write_to_eeprom = false;
    full_reset();
    confirmLed ("green", 10);
    act = true;
  }

  else if (btn == res[5]) {
    confirmLed ("blue", 10);
    reset_to_bootloader();
    act = true;
  }

  if (write_to_eeprom == true)
  {
    writeSettings(false);
    write_to_eeprom = false;
    //confirmLed("white", 50);
  }
  btn = 0;
  return;
  #endif
}

void long_press(const int first_press_time)
{
  delayMicroseconds(100000);
#ifdef VOLUP_STRAP_PIN
  if (!EEPROM_VOL_CONTROL_STRAP)
  {
    EEPROM_VOL_CONTROL_STRAP = 1;
    write_to_eeprom = true;
  }

  uint8_t res = 0;
  uint8_t timer_gap = 150;
  uint16_t flash_time = 5000, flash_off_time = 25000;
  uint32_t steady_on_time = 500000;

  static unsigned long hold_timer = 1, old_timer = 0;
  pinMode (VOLUP_STRAP_PIN, INPUT_PULLUP);
  while (true)
  {
    if ((digitalRead(VOLUP_STRAP_PIN) != LOW) && (old_timer < hold_timer)) {
      //ledBlink("red", 1, 500000, 0);
      res = 0;
      hold_timer = 1;
      old_timer = 0;
      break;
    }
    if ((digitalRead(VOLUP_STRAP_PIN) != LOW) && (old_timer == hold_timer)) break;
    if (digitalRead(VOLUP_STRAP_PIN) == LOW)
    {
      ++hold_timer;
      if (!res)
        if (hold_timer == first_press_time) {
          old_timer = hold_timer;
          ledBlink("green", 10, flash_time, flash_off_time);
          ledBlink("green", 1, steady_on_time, 0);
          if(first_press_time <= 400) res += 1;
          else
          {
            res = 6;
            break;
          }
          
        }
      if (res)
        if (hold_timer == (old_timer + timer_gap)) {
          ledBlink("green", 10, flash_time, flash_off_time);
          ledBlink("green", 1, steady_on_time, 0);
          old_timer = hold_timer;
          res += 1;
        }
      if (res >= OPTIONS_TO_SHOW)
      {
        ledBlink("red", 1, 500000, 0);
        res = 6;
        break;
      }

    }
    delayMicroseconds(10000);
  }
  if (res) {
    hold_timer = 1;
    old_timer = 0;
  }

  if (res == 1) {
    confirmLed ("red", 10);
    if ((EEPROM_BOOT_OPTIONS_AVAILABLE < 1) || (EEPROM_BOOT_OPTIONS_AVAILABLE > 3)) {
      EEPROM_BOOT_OPTIONS_AVAILABLE = 2;  //Sanity check
      write_to_eeprom = true;
    }
    if (EEPROM_BOOT_OPTIONS_AVAILABLE == 1) toggle_dual_boot ();
    if (EEPROM_BOOT_OPTIONS_AVAILABLE == 2) toggle_dual_boot_safe ();
    if (EEPROM_BOOT_OPTIONS_AVAILABLE == 3) toggle_triple_boot ();
  }

  else if (res == 2) {
    confirmLed ("red", 10);
    toggle_colour();
  }

  else if (res == 3) {
    confirmLed ("blue", 10);
    set_dotstar_brightness ();
  }

  else if (res == 4) {
    confirmLed ("blue", 10);
    toggle_settings_method ();
  }

  else if (res == 5) {
    write_to_eeprom = false;
    full_reset();
    confirmLed ("green", 10);
  }

  else if (res == 6) {
    confirmLed ("green", 10);
    reset_to_bootloader();
  }

  if (write_to_eeprom == true)
  {
    writeSettings(false);
    write_to_eeprom = false;
    //confirmLed("white", 50);
  }
  return;
  #endif
}



bool find_RCM()
{

  currentTime = millis();
  foundTegra = false;
  bool blink = true;

  while (!foundTegra)
  {
    usb.Task ();
    usb.ForEachUsbDevice (&findTegraDevice);
    if ((!foundTegra) && (millis() > (currentTime + 300))) return false;
  }
  return true;
}


void reset()
{
#ifdef USB_LOW_RESET
  pinMode(USB_LOW_RESET, INPUT_PULLDOWN); // use internal pulldown on this boot only
  delayMicroseconds(100000);
  int usb_voltage_check = digitalRead (USB_LOW_RESET); // check voltage on thermistor pad on BQ24193
  if (usb_voltage_check == HIGH || usb_voltage_check == RISING)
  {
    pinMode(USB_LOW_RESET, INPUT);
    return;
  }
  else NVIC_SystemReset();
#endif
}

void send_payload(const bool res)
{
  if (res)
  {
    setupTegraDevice ();

    byte deviceID[16] = { 0 };
    readTegraDeviceID (deviceID);
    UHD_Pipe_Alloc (tegraDeviceAddress, 0x01, USB_HOST_PTYPE_BULK, USB_EP_DIR_OUT,
                    0x40, 0, USB_HOST_NB_BK_1);
    packetsWritten = 0;

    send_fusee ();

    if (packetsWritten % 2 != 1)
    {
      usbFlushBuffer ();
    }

    usb.ctrlReq (tegraDeviceAddress, 0,
                 USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD
                 | USB_SETUP_RECIPIENT_INTERFACE,
                 0x00, 0x00, 0x00, 0x00, 0x7000, 0x7000, usbWriteBuffer, NULL);

    sleep (1); //success
  }

  else sleep(0);

}

void
isfitted ()
{
#ifdef JOYCON_STRAP_PIN
  if (!EEPROM_JOYCON_CONTROL_STRAP)
  {
    EEPROM_JOYCON_CONTROL_STRAP = 1;
    writeSettings(false);
  }
#endif
}

void long_press1() {long_press(400);}
void long_press2() {long_press(2000);}


void
sleep (const int errorCode)
{
  if (errorCode == 1)
  {
    ledBlink("green", 1, 25000, 500000);
#ifdef JOYCON_STRAP_PIN
    isfitted();
#endif
  }

  if (errorCode == 0)
  {
    ledBlink("red", 1, 25000, 500000);
    #ifdef USB_LOW_RESET
    if (EEPROM_USB_STRAP == 1 && EEPROM_DUAL_BOOT_TOGGLE == 2)
    {
      pinMode (USB_LOW_RESET, INPUT);
      attachInterrupt (USB_LOW_RESET, reset, FALLING);
      // confirmLed("green",50);
    }
    #endif

  }



#ifdef USB_LOGIC
  digitalWrite (USB_LOGIC, HIGH);
#endif



#ifdef VOLUP_STRAP_PIN
  pinMode (VOLUP_STRAP_PIN, INPUT_PULLUP);

int temp_errorCode = errorCode;  
if(EEPROM_SETTINGS_LOCKOUT) temp_errorCode = 1;
  
  
  if (TEMP_EEPROM_SETTINGS_CHANGE == 1)
  {
    if (temp_errorCode == 1)
    {

      while (((millis() / 1000) * 1052) < (STG_TIMEOUT) * 1000)
      {
        if (digitalRead(VOLUP_STRAP_PIN) == 0) multiple_press();
        pinMode (VOLUP_STRAP_PIN, INPUT_PULLUP);
      }
      //confirmLed("white", 5);
      attachInterrupt (VOLUP_STRAP_PIN, long_press2, LOW);
    }
  }
  else attachInterrupt (VOLUP_STRAP_PIN, long_press1, LOW);
#endif

  //
  //detachInterrupt (VOLUP_STRAP_PIN);
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; /* Enable deepsleep */
  GCLK->CLKCTRL.reg = uint16_t (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2
                                | GCLK_CLKCTRL_ID (GCLK_CLKCTRL_ID_EIC_Val));
  while (GCLK->STATUS.bit.SYNCBUSY)
  {
  }

  __DSB (); /* Ensure effect of last store takes effect */
  __WFI (); /* Enter sleep mode */

}

void first_run()
{
  readSettings ();
  if (EEPROM_INITIAL_WRITE == 1) return;
  else
  {
    EEPROM_INITIAL_WRITE = 1;
  EEPROM_COLOUR = DEFAULT_COLOUR;
  SPARE1 = 0;
  SPARE2 = 0;
  EEPROM_VOL_CONTROL_STRAP = 0;
  EEPROM_JOYCON_CONTROL_STRAP = 0;
  EEPROM_DOTSTAR_BRIGHTNESS = DEFAULT_DOTSTAR_BRIGHTNESS;
  SPARE3 = 0;
  SPARE4 = 0;
  EEPROM_DUAL_BOOT_TOGGLE = 2;
  EEPROM_BOOT_OPTIONS_AVAILABLE = BOOT_OPTIONS_AVAILABLE;
  EEPROM_SETTINGS_LOCKOUT = VOLUP_SETTINGS_DISABLED;
  EEPROM_SETTINGS_CHANGE = DEFAULT_SETTINGS_CHANGE;
  
  SPARE7 = 0;
  //is usb strap fitted?
#ifdef USB_LOW_RESET
  pinMode(USB_LOW_RESET, INPUT_PULLDOWN); // use internal pulldown on this boot only
  int usb_voltage_check = digitalRead (USB_LOW_RESET); // check voltage on thermistor pad on BQ24193
  if (usb_voltage_check == HIGH || usb_voltage_check == RISING) EEPROM_USB_STRAP = 1;
  else EEPROM_USB_STRAP = 0;
#else
  EEPROM_USB_STRAP = 0;
#endif
  }
  confirmLed("white", 50);
  writeSettings(false);
  reset_to_bootloader();
}

void
setup ()
{
  ledInit ();
  first_run();
  usb.Init ();
#ifdef USB_LOGIC
  pinMode (USB_LOGIC, OUTPUT);
  digitalWrite (USB_LOGIC, LOW);
#endif


}

void
loop ()
{
  bool res = find_RCM();
  send_payload(res);


}
