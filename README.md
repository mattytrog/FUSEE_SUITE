## FUSEE UF2

Nintendo Switch internal microcontroller-based payload loader/booter.

This consists of:
A Nintendo Switch v1 UNPATCHED CONSOLE

SAMD21 Device (Adafruit Trinket / Gemma / ItsyBitsy etc)

Part 1 of this software which is the SWITCHBOOT RCM bootloader that, once the chip is installed in your console, will trigger RCM mode.

Part 2 which is the actual payload loading software and chip settings...

## Features
I have been away for over 2 years so this is going to take me a while to get back correct. I can no longer access my old account on GBATemp.

## Fusee_UF2 V7 Changes
#### Simplified operation:

Looks for "fusee.bin" in SD root and boots it.  
If not found, looks for bootloader/update.bin( from SDSetup.com)  

Hold VOL- to skip "fusee.bin" and run Hekate from default location(bootloader/update.bin). Customise in menu  
"Payload<x>" feature removed. Pointless... Possibility of changing to incorrect payload.  

Hold VOL+ to enter config menu.

#### Only 3 operations needed on the chip now, all accessible from long press VOL+.
First option: Toggle autoRCM. Release at first LONG flash... Or  
keep holding to:  
TWO FLASHES: Release at any point within the 2 flashes. RGB LED brightness(0-250). Hit VOL+ at your desired brightness  
keep holding to:  
THREE FLASHES+HOLD UNTIL SLOW FLASHING ON AND OFF -  (steady flashing(White on Adafruit boards)): Chip Eeprom reset + SAMD21 update mode on next reboot. Returns to normal on subsequent reboots.  
  
## Changes
Faster boot time. Faster animation(can be disabled easily in hex editor or download the binary UF2)  
Screen dimming/ Screensaver(if screensaver variable is 0, screen will auto-dim to save battery.)  
Removed regenerate SXOS licence now they are dead.  
Green text because it looks nicer.  
Reboot bootloader option.  
About option - take a read.  
Unused stuff deleted from old Hekate base - Smaller binary size(you won't notice as UF2 = 512kb)  
Bootlogo support and manager. View / select up to 2 bootlogos:  
For Atmosphere: Drop your BMP anywhere and choose from within the app, with preview. Or place directly into atmosphere/splash.bmp  
For Hekate: Drop your BMP anywhere and choose from within the app, with preview. Or place directly into bootloader/bootlogo.bmp  
Bootlogos are chosen depending on payload in use. For example: If not using "fusee.bin", bootloader/bootlogo.bmp will be rendered.  
If fusee.bin is being used splash.bmp will be rendered, if fusee is present but overridden with VOL-(to boot Hekate), bootlogo.bmp will be rendered.  

## Bugfixes etc...  

"Payload<x>" feature removed. Pointless... Possibility of changing to incorrect payload.  

Reworked USB strap code to only place a wakeup interrupt in the following conditions:  

ChipRCM(autoRCM chip-based) is enabled  
Launch has failed(as it would when plugged into PC USB)  
USB voltage is detected(may not activate if battery is 100% and charger isn't doing anything).  
Values no longer stored to EEPROM for the USB strap  

## Could have added lots more things but I want it to be able to fit on a SAMD21E17(128K)  
  
  
## Fitting diagrams
  ![Chip](https://user-images.githubusercontent.com/41282276/154836661-c192851e-651c-4ae9-8b62-8a2551b5d843.jpg)

  
