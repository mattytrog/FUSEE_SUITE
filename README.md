## Current Version: 9.2.1

#### TL;DR? Changelog and other bits...

- 06/06/22 - Silent bugfix. Neopixel brightness adjustment fixed. Added a VOL+ lockout (for kids Switches - only settable with SETTINGS.UF2, or building with Arduino). Changed settings filename convention. Improved SETTINGS.UF2 files. After flashing, UF2 mode will start automatically, for convenience of adding SETTINGS.UF2. If not required, just power down console and reboot to set some default values. All automatic.
- 05/06/22 - OK... All files now updated. Part 1 - Lots of modifications. Part 2 - Same. - SETTINGS.UF2 files introduced.

## Old README and older (basic) versions are [HERE](https://github.com/mattytrog/FUSEE_SUITE/tree/master/Fusee-Suite/Old)

## Quick upgrade procedure if coming from earlier version...
- Boot your Switch into either OFW or CFW
- Get into UF2 mode, by either flexing back cover to activate reset switch TWICE. So Press... Press... Or if you don't have a RESET button, take back off console and touch RESET to GND. If running a recent-ish version of Fusee_UF2, there are ways how to enter UF2 mode without taking the back off... Look in Old Releases above.
- Ensure Switch screen is sleeping and switched off - DONT REBOOT!
- Plug into PC using USB-C cable. You will get a "Unable to charge" error. This is normal
- Make the screen sleep (if it woken up) by pressing POWER to turn it off and sleep the console
- A UF2 drive should pop up (or mount). Will be called SWITCHBOOT probably
- Drag and drop the new part 1, for your chip. Will probably be a Trinket or a team clone. Drag and drop Part 1... The name will change from SWITCHBOOT to FUSEE. If it isn't FUSEE, something went wrong. Try again.
- UF2 drive should disappear and reopen again a few seconds later. If it doesn't, flex back cover twice again and switch screen off... Should now appear. Check drive is called FUSEE! This is important.
- Choose a Part 2. Long-press, or Multiple-press? You decide. Multiple-press is designed for people without LED. But anyone can use it. Drag and drop. UF2 drive should open again...
- You are finished. You can either power down or flash a SETTINGS.UF2. Console will boot normally or however you have configured it.
- End. If all went well, you should never need to open your console (if you did) to enter UF2 mode again. Most people shouldn't need to strip their console just to reset their chip into UF2 mode.
## FUSEE UF2

Nintendo Switch internal microcontroller-based payload loader/booter.  

This consists of:  
A Nintendo Switch v1 UNPATCHED CONSOLE  
SAMD21 Device (Adafruit Trinket / Gemma / ItsyBitsy etc)  
Part 1 of this software which is the SWITCHBOOT or FUSEE RCM bootloader that, once the chip is installed in your console, will trigger RCM mode.  
Part 2 which is the actual payload loading software and chip settings...

## Features
- Fast exploiting and boot
- Fast (approx < 5mS) triggering of RCM
- Built-in SD browsing tools
- Recalibrate battery and fuel gauge (it isn't God though - a knackered battery is still knackered)
- "Correct" boot screen. Either stock black, or red/white
- Change between OFW / CFW / Linux etc etc just by holding down a button at switch-on
- Fast SAFE backup feature
- Software easy to program and update (once you get your head around it)
- Bin the PC launch tools, joycon jigs and battery-killing autoRCM forever
- Many users over 4+ years (went a long time without updates as they aren't essential)
- If your chip is fitted right, will last for a lifetime

## Brief instructions:
- Fit chip
- Flash "part 1"
- Choose a "part 2"... Flash
- Flash optional SETTINGS.UF2
- Set up your SD card at sdsetup.com
- Put fusee.bin in root of SD
- Power on.
- Short press VOL+ boots fusee.bin(or Hekate/Nyx). Long press on VOL+ enters menu
- VOL- AND VOL+ runs Hekate/Nyx(no further config necessary - payload should already be "bootloader/update.bin"
- Press no buttons - chosen payload will launch, or Original Firmware will launch depending on your chip settings

## FITTING A CHIP

Most SAMD21 devices are or can be supported.

Boards supported are:
- Adafruit Trinket M0
- Adafruit QTPY M0
- Rebug SWITCHME
- RCM-X86
- Adafruit Gemma
- Adafruit ItsyBitsy
- Adafruit Feather
- **NEW** Naked chip - if you are REALLY gifted at soldering, fit a QFP chip underneath. No buttons or LEDS needed

Look in the [install diagrams](https://github.com/mattytrog/FUSEE_SUITE/tree/master/Install%20Diagrams) to see how to fit one. Or fit a 'Naked' chip, without buttons, LED or even PCB.

## USERS WITHOUT RESET BUTTON
To access the UF2 bootloader, either HOLD VOL+ for 20 Seconds (LED users) or SELECT THE OPTION IN THE CHIP ASSISTANT MENU.
If you have no part 2 flashed, or is corrupt, and you have no access to the RESET pin, you can alternatively power-off your chip,
touch SWDIO to GND and power on. This will get you into the part 1 bootloader... See below...

It is recommended to flash your chip BEFORE FITTING in most cases. This is your decision though. You can access the UF2 bootloader in all circumstances. A new VIRGIN SOLO chip MUST be flashed before fitting, for your own sanity.
	
So...
	
- Flash Part 1 - a Fusee drive will open.
- Flash Part 2 - the software will run immediately.
- OPTIONAL - You will see new "SETTINGS.UF2" files. These are universal across all SAMD21 chips. Access UF2 mode and drag/drop to change your chip settings.


## Operation

#### Flash part 1
For "stock" microcontrollers (ie Adafruit), just drag and drop your Part 1 onto your chips UF2 drive. Thats it.

If flashing a NEW SAMD21 chip, you will need to do this with your favourite flasher, or alternatively use my Raspberry Pi / OpenOCD image that is configured, ready to go.
Connect power, GND, SWCLK and SWDIO and flash a BIN file. I recommend using TRINKET version, but any will do. Remember if using the FEATHER version, your chip will
require an external oscillator. Whatever you choose, your virgin chip will be using those pins as in a full board. So you would wire your straps as that chip.

If you are reflashing a bricked chip, same instructions apply.
 
#### Flash part 2
Drag and drop chosen part 2 to FUSEE UF2 drive. Flashing complete.

I will upload 2 identical versions. One with the long-press settings approach, one with multiple press approach. These are identical files with just one 
option "preset" to enable long or multiple-press. You can change this option in the menu or with your VOL+ button. This is for convenience only. The files are exactly
identical. You can also change your settings with a SETTINGS.UF2 file.
	
Long-press - at ANY POINT the console is powered on, you can HOLD VOL+ and and LED will flash. Count these flashes for a setting.
The settings are:
Flash 1 - Dual-boot / Chip-based RCM toggle (by default, chip RCM is enabled and console will enter RCM every boot if straps are connected)
Flash 2 - Bootlogo colour toggle - "Stock" black or "Switch" red
Flash 3 - RGB LED / Dotstar / Neopixel brightness
Flash 4 - Long-press / Multiple-press settings toggle
Flash 5 - Reset all chip options back to defaults(AutoRCM - ChipRCM actually enabled, black / white "stock" bootscreen)
Flash 6 - UF2 Bootloader access. Activate this when in the Fusee_UF2 program, connect to PC and launch something (OFW / CFW?) and the FUSEE (was SWITCHBOOT) will pop right open

Multiple-press - This mode is only active for 30 seconds from a cold-boot. So if you want to set multiple options, you might need to reboot to reset the countdown.
	The countdown is displayed on the screen. Why have a countdown? So it cannot be accidentally activated. Kids and grownups will multiple-press VOL+ to increase volume
	in normal usage. This could cause settings to get changed. There is a "plus" side to this mode. You can set the options in the Fusee menu. Look at the
	"Chip Setting Assistant". This makes it possible to use a chip completely "blind" and "naked" - IE without LED or reset switch.
	
Options 1 - 6 are the same as above.
	
Enter the Chip Setup Assistant in the menu and quickly press VOL+ to change your desired option. 
	
Why not just have a menu that you can use "normally"?
Limitations.
It isn't possible to talk to the chip once the inbuilt "payload" is running. Or rather, it IS... But USB code would require A LOT of work, to flip from host to device mode,
enumerate from Switch side and communicate via Serial. Adding new wires to implement I2C or something was considered. But this doesn't help old chip users from day 1.
Also there is space. The current size makes it a REAL squeeze to fit onto a SAMD21E17 as it is. Thats why E18 is default (with 256kb flash).
For the options available, it just isn't worth it.
So we are "mirroring" the multiple-presses in software that we are doing in hardware. That is all.
	
Other menu-based features - bare minimum QUICK backup. Not got patience to wait for full rawnand dump? Backup just your BOOT0/1/PRODINFO partitions. Rebuilding a NAND
from scratch is still a faff, but you will have all your certs and calibration data intact. There are some vicious b**tards out there. Do a SAFE backup. Now.

## Summary of works done in this version of the package

UF2 Bootloader modifications

- Updated for QTPY
- (As yet unpublished Raspberry Pi Pico support - this requires testing further, so isn't included at this time)
- Detection code to check dual-boot flag, with new Failsafe option
- Renamed
- Option to remove RST button support for faster booting and units without RST switch - this isn't active. After consideration, it was decided to leave double-press
intact. If you want to disable RESET button for whatever reason, a rebuild from source is required, after editing your "board_config.h" file.
- Part 1 now detects if USB has power before pulling the straps low(if using the Failsafe RCM setting). If it does, the PC is plugged in and no payload can be pushed. This could cause a battery to go flat
and become damaged. The new option (settable in part 2) will not allow this to happen. If this setting is used, if attempting to boot using the chip and USB is plugged in,
a NORMAL OFW BOOT will occur and the chip will switch off until next reboot. This is optional and is in no way required. But it can be handy. It WILL burn fuses - so if you are trying to save fuses,
for whatever reason, (no real need to any longer), do not use this.
- Emergency bootloader access (if you have no RESET pin access or whatever)... Touch SWDIO to GND when pressing PWR to enter UF2 mode.(PWR off, touch SWDIO to GND, power on).
- Adjustable delay for straps(currently 75mS - I've had good results using a strap hold time of 25 mS. If you want to change this, rebuild from source included. Edit "board_config.h" for your board.
- Built using 2019-9-Q4 GCC toolchain. Newer versions result in a slower binary which cannot init fast enough to enter RCM.
- To accomodate the larger init code and fuse settings in SAMD21, the binary has grown substantially for part 1. Its been necessary to remove RGB LED support for PART 1 ONLY. You will only see a single
LED if fitted. RGB LED will work normally once Part 2 is flashed. This applies to Part 1 only. We do not need an RGB LED with Part 1 anyway... Adafruit QTPY is excluded from this - it has no provision for
onboard single LED - so RGB LED is enabled on this.


#### Main program modifications

- Started with Hekate 5.2+ base
- Screen flipped horizontal
- Font changed
- Colours modified
- tui modified - reimp. of status bar. New timeout feature - returns to main screen after so long.
- gfx modified - new font, new batt asset, screen dimming
- File browser - cleaned up. Multiple pages supported. Easy to use "api". Browse / launch / delete any file in any location on SD
- Safe backup added (quick - BOOOT0/1/PRODINFO)
- Repair pack BOOT0/1 (undersize) flash support
- File / folder deletion
- Enable / disable nyx
- Backup and restore from any folder
- Pre-caching / pre-"stat"ing of payloads during bootscreen show for speed
- Chip setup assistant reordered so people without RST or LED and get into bootloader mode
- Chip setup assistant (if multiple-press is used) makes LED not required.
- Now you can fit a "naked" chip to the underside and access all functions without LED or RST switch
- Added more informative chip help screen / about screen


#### Arduino modifications

- Cleanup
- Speedup check / launch / stack collapse. Fusee launching (to Atmosphere logo at least) is 2x faster than stock.
- Neopixel support added
- 20s long-press for "blind" sans-LED UF2 bootloader access.

## ISSUES / FAQ

- I flashed Part 1 - Nothing happened. No drive popped up.

Disconnect from USB if connected, power off and back on again. If still nothing, you need to manually enter RESET. Either touch RESET pin to GND or touch SWDIO to GND while powering on.

- I cannot drag and drop anything to my chip. Nothing happens when I press RESET.

You have a virgin chip? This needs to be flashed initially with OpenOCD / Raspberry Pi / Other flasher. I can do this for price of postage.
If you have a board that is or was working, do a quick check... If fitted to console, turn on while pressing VOL+. Is RCM triggered? Then you just need to enter UF2 mode. Refer to first question.
If nothing, you have incredibly managed to corrupt Part 1. Treat as a virgin chip. Connect SWCLK and SWDIO and flash using OpenOCD.
If you have USB port still fitted to board, try plugging into PC and double-pressing RESET.
	
#### TODO: More indepth guide on flashing and usage
	
#### TODO: Raspberry Pi OpenOCD image link here and brief guide.
	
## Disclaimer: You should know exactly what you are doing before attempting this modification. It isn't easy - there are really really small points to solder to. No keys or Nintendo copyrighted works are included in this. This is released as open-source, with no warranty given or implied. YOU ONLY HAVE YOURSELF TO BLAME.

