## These are settings files for all versions of Fusee_UF2. Just open your UF2 drive and drag / drop. This will not overwrite your current firmware, this merely changes virtual EEPROM settings. You can of course use VOL+ to change these settings when CFW is running, as always.

## You can "reset" the EEPROM in the VOL+ settings, or just flash BLANK.UF2 for some default values.

## Naming Convention

The filename has 5 "sections"...

#### SETTINGS _ (COLOUR) _ (STRAPS) _ (BOOT OPTIONS AVAILABLE) _ (BOOT PRIORITY) _ (SETTINGS-CHANGE-METHOD). UF2

#### Section 1: Bootlogo colour - Black or Red (B / R)

#### Section 2: Strap presence prechecked. J - Joycon, V - Volume, U - USB... eg JVU (all straps pre-registered) or JV (all but USB strap registered) etc etc

#### Section 3: (B1 - Dualboot / ChipRCM) / (B2 - Dualboot / FailsafeRCM) / (B3 - Dualboot / ChipRCM / FailsafeRCM)

#### Section 4: Boot option preselected. 1 = Dual-boot, 2 - ChipRCM, 3 - FailsafeRCM

#### Section 5: Long-press(LP), Multiple-press(MP) or Bootloader Press (BP)- Long-press is for LED fitted devices. Just long-press and let go to change a setting. Multiple-press gives you an "assistant" in the menu, handy for people WITHOUT LED.


#### Example 1: SETTINGS_B_JVU_B1_2_LP.UF2
- B = BLACK
- JVU = ALL STRAPS PRE_CHECKED
- B1 = DUAL-BOOT / CHIP_RCM AVAILABLE
- 2 = OPTION 2 (CHIP_RCM - aka AUTORCM SELECTED)
- LP = LONG-PRESS VOLUME ONBOARD CHIP SETTINGS ADJUSTMENT


#### Example 2: SETTINGS_R_JVU_B2_1_BP.UF2 (ideal for childrens console)
- R = RED
- JVU = ALL STRAPS PRE_CHECKED
- B2 = DUAL-BOOT / FAILSAFE_RCM AVAILABLE
- 1 = OPTION 1 (DUAL-BOOT CURRENTLY SELECTED)
- BP = BOOTLOADER ONLY OPTION SETTABLE WITH VOL+. ACCESSIBLE WITH 20 SECOND LONG_PRESS ON VOL+


#### Example 3: SETTINGS_R_JVU_B3_3_MP.UF2 (ideal for blind installations (No reset switch or LED)
- R = RED
- JVU = ALL STRAPS PRE_CHECKED
- B3 = DUAL-BOOT / CHIP_RCM / FAILSAFE_RCM AVAILABLE
- 3 = OPTION 3 (FAILSAFE_RCM CURRENTLY SELECTED)
- MP = MULTIPLE-PRESS CHIP SETTINGS (TIMES OUT AFTER 30 SECONDS)

#### Example 4: SETTINGS_B_J_B1_1_LP.UF2 (Example Use-case - Someone with only Joycon strap attached)
- B = BLACK
- J = Joycon Only Prechecked (Only strap fitted for this chap)
- B1 = DUAL-BOOT / CHIP_RCM AVAILABLE
- 2 = OPTION 2 (CHIP_RCM - aka AUTORCM SELECTED - CONSOLE WOULD ALWAYS BE DUAL-BOOT, DUE TO LACK OF STRAPS FITTED)
- LP = LONG-PRESS VOLUME ONBOARD CHIP SETTINGS ADJUSTMENT - MATTERS LITTLE AS WITH NO VOL+ STRAP, NOT ACCESSIBLE

## Make your own? Easy...
![settingsgraphic](https://user-images.githubusercontent.com/41282276/172249882-cffa847a-9cb9-408e-9b36-6d4df604d758.jpg)
