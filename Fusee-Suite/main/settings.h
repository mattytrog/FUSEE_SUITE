#include <Arduino.h>


//When EEPROM_DUAL_BOOT_TOGGLE = 1, normal dual-boot(Hold VOL+ to boot into RCM / Fusee)

//When EEPROM_DUAL_BOOT_TOGGLE = 2, ChipRCM is active - PERMANENT autoRCM (chipRCM is autoRCM triggered by the chip) - 
    //so if plugged into USB when attempting to boot, RCM mode will be active and no boot(black screen).

//When EEPROM_DUAL_BOOT_TOGGLE = 3, Failsafe RCM is active. This is PERMANENT autoRCM, IF USB PORT HAS ZERO VOLTS - 
    //so if you are plugged into USB when attempting to boot, the chip functions will be bypassed and a normal OFW boot will occur.
    //IF YOU ARE SAVING YOUR FUSES, NEVER USE THIS. THIS WILL BURN YOUR FUSES. This mode was designed so its impossible to
    //get stuck in RCM mode, flattening battery.
    //REMEMBER! Most units will NOT BOOT CFW from a chip if USB is plugged into a power source! We normally have to unplug
    //or pull unit out of dock to boot CFW

// 1 = Dual-boot / ChipRCM.   2 = Dual-boot / FailsafeRCM   3 = Dual-boot / ChipRCM / FailsafeRCM
#define BOOT_OPTIONS_AVAILABLE 1

//All VOL+ chip options disabled(a child mode?). Updateable via SETTINGS.UF2. 1 = Settings Disabled. 0 = Enabled 
//Long-press for 20 seconds for UF2 mode is still available in all modes in all circumstances.
#define VOLUP_SETTINGS_DISABLED 0

//Uncomment chosen chip and CHANGE CHIP in Tools menu under "Board"
#define TRINKET
//#define QTPY
//#define REBUG
//#define RCMX86_INTERNAL
//#define GEMMA
//#define ITSYBITSY
//#define FEATHER

//NOT_BUILT_BY_DEFAULT
//#define TRINKETMETHOD3
//#define TRINKETLEGACY3
//#define RCMX86
//#define GENERIC_TRINKET_DONGLE
//#define GENERIC_GEMMA_DONGLE

#define DEFAULT_DOTSTAR_BRIGHTNESS 180
#define DEFAULT_JOYCON 0
#define DEFAULT_VOLUME 0
#define DEFAULT_COLOUR 1
#define DEFAULT_SETTINGS_CHANGE 0 //0 = Long-press (hold for at least 4 seconds and count flashes) or 1 = (multiple-press quickly).
//These are disabled if VOLUP_SETTINGS_DISABLED = 1 (see above)


//Time in mS to select an option with rapid-press in Chip Assistant
#define SELECTION_RELEASE_TIME 200
#define SELECTION_CONFIRM_TIME 200
#define TIME_TUNE 30 // Compensation for timing errors. Worked out manually. Switch timer runs approx 4.8% faster than SAMD21 when counting Milliseconds.
#define SAFETY_PRESS_BARRIER 5 // How many "non-presses" before you get to your first setting
#define BUTTON_GAP 2 // Gap to leave between options
#define OPTIONS_TO_SHOW 6 // How many options to show

#define SETTINGSMENU_HOLD_TIME 125 //Time to hold VOL+ to enter menu
#define BOOTLOGO_DISP_TIME 100 //Time to display Nintendo bootlogo
#define SETTINGS_TIMEOUT 34 // Chip setup assistant timeout. It times out because we don't want accidental presses when console is operating

#define PACKET_CHUNK_SIZE 0x1000 //Size of packet parts transported to higher / lower buffer

//Board definitions
#ifdef TRINKET
#include <Usb.h>
#define DOTSTAR_ENABLED 1
#define USB_LOW_RESET 2
#define JOYCON_STRAP_PIN 3         // Solder to pin 10 on joycon rail
#define VOLUP_STRAP_PIN 0
#define ONBOARD_LED 13
#endif

#ifdef QTPY
#include <Usb.h>
#define NEOPIXEL_ENABLED 1
#define USB_LOW_RESET 1
#define JOYCON_STRAP_PIN 0         // Solder to pin 10 on joycon rail
#define VOLUP_STRAP_PIN 3
#define BOARD_NEOPIXEL_PIN 11
#define BOARD_NEOPIXEL_COUNT 1
#endif

#ifdef REBUG
#include <Usb.h>
//#define DOTSTAR_ENABLED 1
#define USB_LOW_RESET 2
#define JOYCON_STRAP_PIN 3         // Solder to pin 10 on joycon rail
#define VOLUP_STRAP_PIN 0
#define ONBOARD_LED 13
#endif

#ifdef GEMMA
#include <Usb.h>
#define DOTSTAR_ENABLED 1
#define USB_LOW_RESET 1
#define JOYCON_STRAP_PIN 2         // Solder to pin 10 on joycon rail
#define VOLUP_STRAP_PIN 0
#define ONBOARD_LED 13
#endif

#ifdef ITSYBITSY
#include <Usb.h>
#define DOTSTAR_ENABLED 1
#define USB_LOW_RESET 7
#define JOYCON_STRAP_PIN 9         // Solder to pin 10 on joycon rail
#define VOLUP_STRAP_PIN 11
#define INTERNAL_DS_DATA 41
#define INTERNAL_DS_CLK 40
#define ONBOARD_LED 13
#endif

#ifdef FEATHER
#include <Usb.h>
#define DOTSTAR_ENABLED 1
#define USB_LOW_RESET 6
#define JOYCON_STRAP_PIN 9         // Solder to pin 10 on joycon rail
#define VOLUP_STRAP_PIN 11
#define INTERNAL_DS_DATA 41
#define INTERNAL_DS_CLK 40
#define ONBOARD_LED 13
#endif

#ifdef RCMX86_INTERNAL
#include <Usb.h>
#define USB_LOW_RESET 4
#define JOYCON_STRAP_PIN 2         // Solder to pin 10 on joycon rail
#define VOLUP_STRAP_PIN 1
#define USB_LOGIC 3
#define ONBOARD_LED 0
#endif

//#ifdef TRINKETMETHOD3
//#define DOTSTAR_ENABLED 1
//#define USB_LOW_RESET 4
//#define WAKEUP_PIN 2        // Method 3 pin to M92T36 pin 5 capacitor / rail
//#define JOYCON_STRAP_PIN 3         // Solder to pin 10 on joycon rail
//#define VOLUP_STRAP_PIN 0
//#define ONBOARD_LED 13
//#endif
//
//#ifdef TRINKETLEGACY3
//#define DOTSTAR_ENABLED 1
//#define USB_LOW_RESET 2
//#define WAKEUP_PIN 4        // Method 3 pin to M92T36 pin 5 capacitor / rail
//#define JOYCON_STRAP_PIN 3         // Solder to pin 10 on joycon rail
//#define VOLUP_STRAP_PIN 0
//#define ONBOARD_LED 13
//#endif
