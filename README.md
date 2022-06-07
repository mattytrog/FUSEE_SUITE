## Current Version: 9.2.1 - Scroll down for info / intro...

#### TL;DR? Changelog and other bits...

- 06/06/22 - Silent bugfix. Neopixel brightness adjustment fixed. Added a VOL+ lockout (for kids Switches - only settable with SETTINGS.UF2, or building with Arduino). Changed settings filename convention. Improved SETTINGS.UF2 files. After flashing, UF2 mode will start automatically, for convenience of adding SETTINGS.UF2. If not required, just power down console and reboot to set some default values. All automatic.
- 05/06/22 - OK... All files now updated. Part 1 - Lots of modifications. Part 2 - Same. - SETTINGS.UF2 files introduced.

## Old README and older (basic) versions are [HERE](https://github.com/mattytrog/FUSEE_SUITE/tree/master/Fusee-Suite/Old)

## Quick upgrade procedure if coming from earlier version...
- Make sure your SD is set up as SDSetup does it... bootloader/update.bin present AT LEAST. Drag and drop fusee.bin to SD root (its in payloads folder) if desired. If you want your own file organisation, you will need to choose your fusee.bin payload(can be anything) and update.bin (again, anything) in the menu of Fusee_UF2.
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

#### Important differences...
- This doesn't look for payload.bin any longer. It looks for fusee.bin (or bootloader/update.bin). You can rename your payload.bin in the Fusee_UF2 menu.
- Want to boot Hekate every time? Easy. Just disable or delete fusee.bin if you have it (or even rename hekate to that if you desire). Hekate / Nyx will boot automatically.
- Hold down BOTH VOL buttons and power on if you want to cold-boot to Hekate / Nyx, rather than fusee.bin(not permanent, this boot only. To make permanent, disable fusee.bin in menu)
## FUSEE UF2

Nintendo Switch internal microcontroller-based payload loader/booter.  

This consists of:  
A Nintendo Switch v1 UNPATCHED CONSOLE  
SAMD21 Device (Adafruit Trinket / Gemma / ItsyBitsy etc)  
Part 1 of this software which is the SWITCHBOOT or FUSEE RCM bootloader that, once the chip is installed in your console, will trigger RCM mode.  
Part 2 which is the actual payload loading software and chip settings...

## Features - Thanks to CTCAer for Hekate base that is partly used in Part 2. Arduino being the other part.
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
To access the UF2 bootloader, either HOLD VOL+ for 20 Seconds or SELECT THE OPTION IN THE CHIP ASSISTANT MENU.
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
Flash 1 - Dual-boot / Chip-based RCM toggle or FailsafeRCM (by default, chip RCM is enabled and console will enter RCM every boot if straps are connected)
Flash 2 - Bootlogo colour toggle - "Stock" black or "Switch" red
Flash 3 - RGB LED / Dotstar / Neopixel brightness (if fitted to correct pins, stock Adafruit devices are)
Flash 4 - Long-press / Multiple-press settings toggle
Flash 5 - Reset all chip options back to defaults(AutoRCM - ChipRCM actually enabled, black / white "stock" bootscreen)
Flash 6 - UF2 Bootloader access. Activate this when in the Fusee_UF2 program, connect to PC and launch something (OFW / CFW?) and the FUSEE (was SWITCHBOOT) will pop right open. People without LED can still access this by holding VOL+ for 20 seconds

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

## Glossary of terms used in this document

- CFW - Custom firmware
- OFW - Original (standard) firmware
- RCM - Special mode of the Switch to do things, probably in the factory. Not normally accessible to end-users. We just it as the special mode required to launch "payloads"
- AutoRCM - Automatically entering RCM mode needed to begin the process of running CFW on the Switch
- Payload - A piece of software that basically boots and does something on the console, exploiting the unit to run code it shouldn't
- ChipRCM - My version of autoRCM. Instead of controlled corrupting of flash storage like other methods, this uses basically the "official" way to achieve RCM, with a chip
- FailsafeRCM - Same as ChipRCM, but automatically disables itself when it cannot do anything eg when plugged into PC (using Fusee-UF2 via a chip is not possible if USB is plugged into PC, resulting the unit being "trapped" in RCM mode. If left in this mode, the battery will be ran completely flat and damaged potentially)
- SAFE backup - Essential backup of BOOT0/1 and PRODINFO. These contain certificates and calibration data for your console. They are not interchangeable. Google "Brickachu" for more information
- Strap - A wire or conductor linking an input/output of a chip (our SAMD21 we use for Fusee_UF2) to a point on the Switch motherboard, to perform a function. EG VOL+ strap emulates somebody pressing VOL+ button. We use 3. Joycon, Volume and USB. Only Joycon is essential to running CFW. Vol+ is pressed by hand (can be)
- Drop the straps / Drop a strap - Pull a strap to GND, triggering its action

## Why keep working on such a simple piece of software to do a simple job?

- Because I want it right and as problem-free as possible
- It should be easy to use
- And easy to install

## How?

- It builds the payload on-the-fly in a split-second, based on selected settings. Using code from Me, Usbhost, other people from back in the day and the creator of Hekate, and Arduino. Its a combined effort (or it was)

## Doesn't it wear the chip out keeping writing to it?

- Yes, eventually. Maybe. Atmel quote 10,000 write cycles before the chip starts getting dementia. However, this software uses full wear-levelling, and only individual bits are written only when needed, at once, not rewriting the same data over and over, effectively giving us 10000 ^ 15 (we have 15 pages we use, right at the end of the flash) write cycles. Which is probably more than an SD card... I've been using the same SAMD21 test rig, with same chip, since 2018. Written and in use almost daily. Its still fine. Its not written to all the time. You will probably set your options and never change them

## Disclaimer: You should know exactly what you are doing before attempting this modification. It isn't easy - there are really really small points to solder to. No keys or Nintendo copyrighted works are included in this. This is released as open-source, with no warranty given or implied. YOU ONLY HAVE YOURSELF TO BLAME.

