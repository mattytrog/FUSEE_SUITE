## Current Version: 9.1
## Part 1 updated (after 4 years!) to support newer boards
## Old README and other versions are HERE

## USERS WITHOUT RESET BUTTON
To access the UF2 bootloader, either HOLD VOL+ for 20 Seconds (LED users) or SELECT THE OPTION IN THE CHIP ASSISTANT MENU. See below...

## FUSEE UF2

Nintendo Switch internal microcontroller-based payload loader/booter.  

This consists of:  
A Nintendo Switch v1 UNPATCHED CONSOLE  
SAMD21 Device (Adafruit Trinket / Gemma / ItsyBitsy etc)  
Part 1 of this software which is the SWITCHBOOT or FUSEE RCM bootloader that, once the chip is installed in your console, will trigger RCM mode.  
Part 2 which is the actual payload loading software and chip settings...  

Brief instructions:
Fit chip
Choose and flash a "part 1"
Flash UF2
Fin

## FITTING A CHIP

Most SAMD21 devices are or can be supported.

Look in the <install diagrams> to see how to fit one. Or fit a 'Naked' chip, without buttons, LED or even PCB.

## Operation

#### Choose a part 1
Either with reset switch support or without. Without is faster but you cannot double-press reset to access bootloader... Instead hold VOL+ for 20 seconds.
This is for naked chips / boards (eg RCMX86) or "thick" boards, eg QTPY_m0, which require removing RST button.
 
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
- Detection code to check dual-boot flag
- Renamed
- Option to remove RST button support for faster booting and units without RST switch.
- Adjustable delay for straps(currently 50mS)
- Built using 2019-9-Q4 GCC toolchain. Newer versions result in a slower binary which cannot init fast enough to enter RCM.


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

