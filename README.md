## Current Version: 9.2

I've dropped a big b0llock. I pushed v9.1 to my repo my side but for whatever idiotic reason, I didn't actually push it. I will do this on Friday when I'm home. I'm on holiday. And can't really get back to my computer until then. 
I added more documentation etc etc but it's all still f..kin sat there.
So... Wait on updating part 1 until Friday. Anyone who has and is struggling entering bootloader mode(because they dropped part 2 on there), I'll fix free of charge if you are unable to access bootloader mode. In meantime, your chip should continue to work normally using v9.0. if not, you should be able to enter bootloader mode.
## Part 1 updated (after 4 years!) to support newer boards
Boards supported are:
- Adafruit Trinket M0
- Adafruit QTPY M0
- Rebug SWITCHME
- RCM-X86
- Adafruit Gemma
- Adafruit ItsyBitsy
- Adafruit Feather
- **NEW** Naked chip - if you are REALLY gifted at soldering, fit a QFP chip underneath. No buttons or LEDS needed
## Old README and other versions are HERE
(https://github.com/mattytrog/FUSEE_SUITE/tree/master/Fusee-Suite/Old)

## USERS WITHOUT RESET BUTTON
To access the UF2 bootloader, either HOLD VOL+ for 20 Seconds (LED users) or SELECT THE OPTION IN THE CHIP ASSISTANT MENU.
If you have no part 2 flashed, or is corrupt, and you have no access to the RESET pin, you can alternatively power-off your chip,
touch SWDIO to GND and power on. This will get you into the part 1 bootloader... See below...

## FUSEE UF2

Nintendo Switch internal microcontroller-based payload loader/booter.  

This consists of:  
A Nintendo Switch v1 UNPATCHED CONSOLE  
SAMD21 Device (Adafruit Trinket / Gemma / ItsyBitsy etc)  
Part 1 of this software which is the SWITCHBOOT or FUSEE RCM bootloader that, once the chip is installed in your console, will trigger RCM mode.  
Part 2 which is the actual payload loading software and chip settings...  

Brief instructions:
- Fit chip
- Flash "part 1"
- Choose a "part 2"... Flash
- Flash optional SETTINGS.UF2
- Fin

## FITTING A CHIP

Most SAMD21 devices are or can be supported.

Look in the <install diagrams> to see how to fit one. Or fit a 'Naked' chip, without buttons, LED or even PCB.
	
It is recommended to flash your chip BEFORE FITTING in most cases. This is your decision though. You can access the UF2 bootloader in all circumstances. A new VIRGIN SOLO chip MUST be flashed before fitting, for your own sanity.
	
So...
	
- Flash Part 1 - a Fusee drive will open.
- Flash Part 2 - the software will run immediately.
- OPTIONAL - You will see new "SETTINGS.UF2" files. These are universal across all SAMD21 chips. Access UF2 mode and drag/drop to change your chip settings.


## Operation

#### Flash part 1
If flashing a NEW SAMD21 chip, you will need to do this with your favourite flasher, or alternatively use my Raspberry Pi / OpenOCD image that is configured, ready to go.
Connect power, GND, SWCLK and SWDIO and flash a BIN file. I recommend using TRINKET version, but any will do. Remember if using the FEATHER version, your chip will
require an external oscillator. Whatever you choose, your virgin chip will be using those pins as in a full board. So you would wire your straps as that chip.

If you are reflashing a bricked chip, same instructions apply.
 
#### Flash part 2
I will upload 2 identical versions. One with the long-press settings approach, one with multiple press approach. These are identical files with just one 
option "preset" to enable long or multiple-press. You can change this option in the menu or with your VOL+ button. This is for convenience only. The files are exactly
identical.
	
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

#### Summary of works done in this version of the package

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
- If faulty charger or connection, this can be detected and shown (repeated reboots are counted and stored), if necessary, -control switched off.
- Neopixel support added
- 20s long-press for "blind" sans-LED UF2 bootloader access.
	
#### TODO: More indepth guide on flashing and usage
	
#### TODO: Raspberry Pi OpenOCD image link here and brief guide.
	
## Disclaimer: You should know exactly what you are doing before attempting this modification. It isn't easy - there are really really small points to solder to. No keys or Nintendo copyrighted works are included in this. This is released as open-source, with no warranty given or implied. YOU ONLY HAVE YOURSELF TO BLAME.

