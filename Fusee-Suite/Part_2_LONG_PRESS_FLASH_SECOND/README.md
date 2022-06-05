## Multiple-press settings versions of Fusee_UF2. Identical to other versions but EEPROM bit changed from 00 to 01(or vise-versa) to enable Long-press settings.

#### These versions have the boot priority toggle able to be set in 3 ways

- Dual-boot
- ChipRCM
- FailsafeRCM (if boot fails, OFW will run)

#### If anyone wants a version with just 2 options (as before, let me know) or build yourself, adjusting #define TRIPLE_BOOT in Arduino source.