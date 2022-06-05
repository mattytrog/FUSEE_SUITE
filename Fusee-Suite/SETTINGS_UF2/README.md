## Naming Convention

The filename has 4 "sections"...

#### (COLOUR) _ (STRAPS) _ (BOOT PRIORITY) _ (SETTINGS-CHANGE-METHOD) . UF2

#### Section 1: Bootlogo colour - Black or Red (B / R)

#### Section 2: Strap presence prechecked. Assumes all straps are fitted and working(AS), or tested on-device(NS) - All straps or No straps (AS / NS)

#### Section 3: Dual-boot(DB), ChipRCM(CRCM)(permanent CFW boot), FailsafeRCM(FRCM) - if USB strap is fitted - (Normal boot will occur if USB power is connected, to prevent locked in RCM. WILL BURN FUSES)

#### Section 4: Long-press(LP) or Multiple-press(MP) - Long-press is for LED fitted devices. Just long-press and let go to change a setting. Multiple-press gives you an "assistant" in the menu, handy for people WITHOUT LED.

#### Example 1: B_AS_CRCM_LP.UF2
- B = BLACK
- AS = ALL STRAPS PRE_CHECKED
- CRCM = CHIPRCM DEFAULT
- LP = LONG-PRESS VOLUME ONBOARD CHIP SETTINGS ADJUSTMENT

#### Example 2: R_NS_DB_MP.UF2
- R = RED
- NS = NO STRAPS PRECHECKED - VALUES FILLED IN AS CONSOLE RUNS
- DB = DUAL-BOOT DEFAULT
- MP = MULTIPLE-PRESS VOLUME ONBOARD CHIP SETTINGS ADJUSTMENT

#### Example 3: B_AS_FRCM_LP.UF2
- B = BLACK
- AS = ALL STRAPS PRE_CHECKED
- FRCM = FAILSAFE RCM - Normal boot will occur if USB power is connected, to prevent locked in RCM.
- LP = LONG-PRESS VOLUME ONBOARD CHIP SETTINGS ADJUSTMENT
