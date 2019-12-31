#include <Arduino.h>

bool switchboot; bool fusee; bool update_samd;
void mode_check() {
  if (EEPROM_MODE_NUMBER == 1) {
    AMOUNT_OF_PAYLOADS = 8;
    AUTO_INCREASE_PAYLOAD_on = 0;
    switchboot = false; fusee = true; update_samd = false;
    }
    if (EEPROM_MODE_NUMBER > 4) {
    AMOUNT_OF_PAYLOADS = 1;
    AUTO_INCREASE_PAYLOAD_on = 0;
    switchboot = false; fusee = false; update_samd = true;
    } 
  }
