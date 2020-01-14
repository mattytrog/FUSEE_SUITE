#include <Arduino.h>
#include <Usb.h>
#include <Adafruit_DotStar.h>
#include "virtual_eeprom.h"

#define RCM_STRAP_TIME_us 1000000
#define LED_CONFIRM_TIME_us 500000
#define PACKET_CHUNK_SIZE 0x1000

//bootlogo colours. Hex. 0x00 - 0xFF (0 - 255)
#define LOGO_BACKGROUND_RED 0xE6 // 0xE6 Stock Nintendo Red
#define LOGO_BACKGROUND_GREEN 0x00 // 0x00 Stock Nintendo Red
#define LOGO_BACKGROUND_BLUE 0x12 // 0x12 Stock Nintendo Red
#define LOGO_FOREGROUND_RED 0xFF // 0xFF Stock White
#define LOGO_FOREGROUND_GREEN 0xFF // 0xFF Stock White
#define LOGO_FOREGROUND_BLUE 0xFF // 0xFF Stock White

#define DEFAULT_PAYLOAD 1
#define DEFAULT_MODE 1
#define DEFAULT_DOTSTAR_BRIGHTNESS 180
#define DEFAULT_JOYCON 0
#define DEFAULT_VOLUME 0
#define MODES_AVAILABLE 1
#define DEFAULT_BOOTLOGO 3 //1 = no bootlogo, 2 = static bootlogo, 3 = (sliding), 4 = (wiping) 5 = (white noise)
#define LOGOS_AVAILABLE 5
#define DEFAULT_SCREENSAVER_ON 1 //1 = active. 0 = inactive.


#define PAYLOAD_FLASH_LED_ON_TIME_SECONDS 0.05 // controls blink during payload indication. On
#define PAYLOAD_FLASH_LED_OFF_TIME_SECONDS 0.3 // as above, but amount of time for DARKNESS ;)
#define DELAY_BEFORE_SLEEP 500000
#define AUTO_SEND_ON_PAYLOAD_INCREASE_PIN 0  //Automatic send when payload pin is activated. 1 = on, 0 = off
#define LOOK_FOR_TEGRA_LED_SPEED 100 //How fast to blink when searching. Higher = slower
#define AMOUNT_OF_OTHER_OPTIONS 2 // lock out disable chip

///////////////////////////////////////////
//#define TXTFILEPROVIDESTRAPVALUES

//#define TRINKET
//#define TRINKETMETHOD3
//#define TRINKETLEGACY3
//#define REBUG
//#define RCMX86_INTERNAL
//#define RCMX86
//#define GENERIC_TRINKET_DONGLE
//#define GEMMA
//#define GENERIC_GEMMA_DONGLE
//#define ITSYBITSY
#define FEATHER


//non eeprom globals
uint8_t AMOUNT_OF_PAYLOADS;
uint8_t AUTO_INCREASE_PAYLOAD_on;
int currentTime = 0;
USBHost usb;
EpInfo epInfo[3];

byte usbWriteBuffer[PACKET_CHUNK_SIZE] = {0};
uint32_t usbWriteBufferUsed = 0;
uint32_t packetsWritten = 0;

bool foundTegra = false;
byte tegraDeviceAddress = -1;

unsigned long lastCheckTime = 0;


#include "boards.h"
#include "PL1.h"
#include "PL2.h"
#include "FS_misc.h"
#include "STRAP_INFO.h"
#include "LED_control.h"
#include "modes.h"
#include "longpress.h"
#include "usb_control.h"

void runOnce()
{
#ifdef USB_LOW_RESET
  pinMode(USB_LOW_RESET, INPUT);
  while (update_samd && EEPROM_USB_REBOOT_STRAP == 1)
  {
    if (digitalRead(USB_LOW_RESET) == LOW) break;
    ledBlink("white", 1, 500000, 500000);
  }
#endif
  if (EEPROM_EMPTY != 1) EEPROM_EMPTY = 0;
  if (EEPROM_EMPTY == 1) return;

  EEPROM_PAYLOAD_NUMBER = DEFAULT_PAYLOAD;
  EEPROM_MODE_NUMBER = DEFAULT_MODE;
  EEPROM_DOTSTAR_BRIGHTNESS = DEFAULT_DOTSTAR_BRIGHTNESS;
  EEPROM_JOYCON_CONTROL_STRAP = DEFAULT_JOYCON;
  EEPROM_VOL_CONTROL_STRAP = DEFAULT_VOLUME;
  EEPROM_BOOTLOGO = DEFAULT_BOOTLOGO;
  EEPROM_PAYLOAD_INDICATION = 0;
  EEPROM_DUAL_BOOT_TOGGLE = 2;
  EEPROM_SCREENSAVER_ACTIVE = DEFAULT_SCREENSAVER_ON;
  EEPROM_AUTO_INCREASE = 0;

#ifdef USB_LOW_RESET
  pinMode(USB_LOW_RESET, INPUT_PULLDOWN); // use internal pulldown on this boot only
  uint32_t usb_voltage_check = digitalRead(USB_LOW_RESET); //check voltage on thermistor pad on BQ24193
  if (usb_voltage_check == HIGH) {
    setLedColor("blue");
    //delay so I can activate bootloader mode to pull UF2 without Eeprom data
    EEPROM_USB_REBOOT_STRAP = 1; //strap is fitted. Lets store to flash
  } else {
    setLedColor("red");
    EEPROM_USB_REBOOT_STRAP = 0; //strap is not fitted. Lets store to flash
  }
#endif
  delayMicroseconds(2000000);
  EEPROM_EMPTY = 1;
  writeSettings(); // run-once complete. Store to flash to say it has ran
  confirmLed("white", 10);
  NVIC_SystemReset(); //restart to reflect changes

}




void wakeup() {
  SCB->AIRCR = ((0x5FA << SCB_AIRCR_VECTKEY_Pos) | SCB_AIRCR_SYSRESETREQ_Msk); //full software reset
}

void setup()
{
  ledInit();
  readSettings();
  mode_check();
  runOnce();



#ifdef USB_LOGIC
  pinMode(USB_LOGIC, OUTPUT);
  digitalWrite(USB_LOGIC, LOW);
#endif

  int startTime = millis();

  EIC->WAKEUP.vec.WAKEUPEN |= (1 << 6);

  int usbInitialized = usb.Init();
  if (usbInitialized == -1) sleep(-1);

  bool blink = true;



  while (!foundTegra)
  {
    currentTime = millis() - startTime;
    usb.Task();

    if (currentTime > lastCheckTime + LOOK_FOR_TEGRA_LED_SPEED) {
      usb.ForEachUsbDevice(&findTegraDevice);
      if (blink && !foundTegra) {
        if (update_samd) setLedColor("white");
        else setLedColor("orange");
      } else {
        setLedColor("black"); //led to black
      }
      blink = !blink;
      lastCheckTime = currentTime;
    }
    if (currentTime > (LOOK_FOR_TEGRA_SECONDS * 1000)) {
      if (!update_samd) sleep(-1);
      else {
        EEPROM_MODE_NUMBER = EEPROM_MODE_NUMBER - 5;
        writeSettings();
        *DBL_TAP_PTR = DBL_TAP_MAGIC;
        NVIC_SystemReset();
      }
    }

  }

  setupTegraDevice();

  byte deviceID[16] = {0};
  readTegraDeviceID(deviceID);
  UHD_Pipe_Alloc(tegraDeviceAddress, 0x01, USB_HOST_PTYPE_BULK, USB_EP_DIR_OUT, 0x40, 0, USB_HOST_NB_BK_1);
  packetsWritten = 0;

  if (update_samd) sendPayload(SAMD_UPDATE, 928);

  if (fusee == true) {
    if (EEPROM_PAYLOAD_NUMBER == 1) {
      send_fusee(PAYLOAD1);
    } else if (EEPROM_PAYLOAD_NUMBER == 2) {
      send_fusee(PAYLOAD2);
    } else if (EEPROM_PAYLOAD_NUMBER == 3) {
      send_fusee(PAYLOAD3);
    } else if (EEPROM_PAYLOAD_NUMBER == 4) {
      send_fusee(PAYLOAD4);
    } else if (EEPROM_PAYLOAD_NUMBER == 5) {
      send_fusee(PAYLOAD5);
    } else if (EEPROM_PAYLOAD_NUMBER == 6) {
      send_fusee(PAYLOAD6);
    } else if (EEPROM_PAYLOAD_NUMBER == 7) {
      send_fusee(PAYLOAD7);
    } else if (EEPROM_PAYLOAD_NUMBER == 8) {
      send_fusee(PAYLOAD8);
    }
  }

  if (packetsWritten % 2 != 1)
  {
    usbFlushBuffer();
  }


  usb.ctrlReq(tegraDeviceAddress, 0, USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE,
              0x00, 0x00, 0x00, 0x00, 0x7000, 0x7000, usbWriteBuffer, NULL);

  if (update_samd)
  {
    EEPROM_MODE_NUMBER = EEPROM_MODE_NUMBER - 5;
    writeSettings();
    *DBL_TAP_PTR = DBL_TAP_MAGIC;
    NVIC_SystemReset();
  }
  sleep(1);
}

void loop()
{
  sleep(1);
}
