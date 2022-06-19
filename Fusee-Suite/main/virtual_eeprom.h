#include <Arduino.h>

#define DBL_TAP_MAGIC 0xf01669ef
#define DBL_TAP_PTR ((volatile uint32_t * )(HMCRAMC0_ADDR + HMCRAMC0_SIZE - 4))
#define PAGE_00 0x3FC00
#define PAGE_01 0x3FC40
#define PAGE_02 0x3FC80
#define PAGE_03 0x3FCC0
#define PAGE_04 0x3FD00
#define PAGE_05 0x3FD40
#define PAGE_06 0x3FD80
#define PAGE_07 0x3FDC0
#define PAGE_08 0x3FE00
#define PAGE_09 0x3FE40
#define PAGE_10 0x3FE80
#define PAGE_11 0x3FEC0
#define PAGE_12 0x3FF00
#define PAGE_13 0x3FF40
#define PAGE_14 0x3FF80
#define PAGE_15 0x3FFC0
#define LAST_PAGE ((usersettings_t * ) PAGE_15)
#define PAGE_SIZE 0x40

uint32_t settingsarray[16];
uint32_t blankarray[16];
uint8_t pagedata = 0;
uint8_t EEPROM_INITIAL_WRITE;
uint8_t EEPROM_COLOUR;
uint8_t SPARE1;
uint8_t SPARE2;
uint8_t EEPROM_USB_STRAP;
uint8_t EEPROM_VOL_CONTROL_STRAP;
uint8_t EEPROM_JOYCON_CONTROL_STRAP;
uint8_t EEPROM_DOTSTAR_BRIGHTNESS;
uint8_t SPARE3;
uint8_t SPARE4;
uint8_t EEPROM_SETTINGS_CHANGE;
uint8_t EEPROM_DUAL_BOOT_TOGGLE;
uint8_t EEPROM_BOOT_OPTIONS_AVAILABLE;
uint8_t EEPROM_SETTINGS_LOCKOUT;
uint8_t SPARE7;

uint8_t TEMP_EEPROM_SETTINGS_CHANGE;
typedef struct __attribute__((__packed__)) usersettings {
  uint8_t a1;
  uint8_t b1;
  uint8_t c1;
  uint8_t d1;
  uint8_t e1;
  uint8_t f1;
  uint8_t g1;
  uint8_t h1;
  uint8_t i1;
  uint8_t j1;
  uint8_t k1;
  uint8_t l1;
  uint8_t m1;
  uint8_t n1;
  uint8_t o1;
  uint8_t p1;
}
usersettings_t;

static inline void wait_ready(void) {
  while (NVMCTRL -> INTFLAG.bit.READY == 0);
}

void flash_erase_row(uint32_t * dst) {
  wait_ready();
  NVMCTRL -> STATUS.reg = NVMCTRL_STATUS_MASK;

  // Execute "ER" Erase Row
  NVMCTRL -> ADDR.reg = (uint32_t) dst / 2;
  NVMCTRL -> CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
  wait_ready();
}

void flash_write_words(uint32_t * dst, uint32_t * src, uint32_t n_words) {
  // Set automatic page write
  NVMCTRL -> CTRLB.bit.MANW = 0;

  while (n_words > 0) {
    uint32_t len = (FLASH_PAGE_SIZE >> 2) < n_words ? (FLASH_PAGE_SIZE >> 2) : n_words;
    n_words -= len;

    wait_ready();
    // Execute "PBC" Page Buffer Clear
    NVMCTRL -> CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
    wait_ready();

    // make sure there are no other memory writes here
    // otherwise we get lock-ups

    while (len--)
      *
      dst++ = * src++;

    // Execute "WP" Write Page
    NVMCTRL -> CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
    wait_ready();
  }
}

extern uint8_t STG_TIMEOUT;

void readSettings() {
  for (int y = PAGE_15; y >= (PAGE_00); y -= PAGE_SIZE) {
    usersettings_t * config = (usersettings_t * ) y;
    if (config -> a1 == 0) {
      EEPROM_INITIAL_WRITE = config -> b1;
      EEPROM_COLOUR = config -> c1;
      SPARE1 = config -> d1;
      SPARE2 = config -> e1;
      EEPROM_USB_STRAP = config -> f1;
      EEPROM_VOL_CONTROL_STRAP = config -> g1;
      EEPROM_JOYCON_CONTROL_STRAP = config -> h1;
      EEPROM_DOTSTAR_BRIGHTNESS = config -> i1;
      SPARE3 = config -> j1;
      SPARE4 = config -> k1;
      EEPROM_SETTINGS_CHANGE = config -> l1;
      EEPROM_DUAL_BOOT_TOGGLE = config -> m1;
      EEPROM_BOOT_OPTIONS_AVAILABLE = config -> n1;
      EEPROM_SETTINGS_LOCKOUT = config -> o1;
      SPARE7 = config -> p1;
      break;
    }
  }
  if (EEPROM_SETTINGS_LOCKOUT) {
    TEMP_EEPROM_SETTINGS_CHANGE = 1;
    STG_TIMEOUT = 0;
  } else {
    TEMP_EEPROM_SETTINGS_CHANGE = EEPROM_SETTINGS_CHANGE;
    STG_TIMEOUT = SETTINGS_TIMEOUT;
  }
}

//extern void
//confirmLed(const char color[], uint32_t count);

extern void reset_to_bootloader();
void writeSettings(const bool overwrite) {

  if (overwrite) {
    flash_erase_row((uint32_t * ) PAGE_00);
    flash_erase_row((uint32_t * ) PAGE_04);
    flash_erase_row((uint32_t * ) PAGE_08);
    flash_erase_row((uint32_t * ) PAGE_12);
    memset(blankarray, 0xFF, 64);
    //Store settings in flash. 16 pages used for wear-levelling, starts reading from first page.
    for (int y = PAGE_00; y <= (PAGE_15); y += PAGE_SIZE) {
      //If we find a page with no user settings, write them and stop looking.
      flash_write_words((uint32_t * ) y, blankarray, 16);
    }

    //flash_write_words((uint32_t *)PAGE_00, settingsarray, 4);
    reset_to_bootloader();
  }

  usersettings_t config;

  config.a1 = 0;
  config.b1 = EEPROM_INITIAL_WRITE;
  config.c1 = EEPROM_COLOUR;
  config.d1 = SPARE1;
  config.e1 = SPARE2;
  config.f1 = EEPROM_USB_STRAP;
  config.g1 = EEPROM_VOL_CONTROL_STRAP;
  config.h1 = EEPROM_JOYCON_CONTROL_STRAP;
  config.i1 = EEPROM_DOTSTAR_BRIGHTNESS;
  config.j1 = SPARE3;
  config.k1 = SPARE4;
  config.l1 = EEPROM_SETTINGS_CHANGE;
  config.m1 = EEPROM_DUAL_BOOT_TOGGLE;
  config.n1 = EEPROM_BOOT_OPTIONS_AVAILABLE;
  config.o1 = EEPROM_SETTINGS_LOCKOUT;
  config.p1 = SPARE7;

  memcpy(settingsarray, & config, 16);

  if (LAST_PAGE -> a1 == 0) {
    flash_erase_row((uint32_t * ) PAGE_00);
    flash_erase_row((uint32_t * ) PAGE_04);
    flash_erase_row((uint32_t * ) PAGE_08);
    flash_erase_row((uint32_t * ) PAGE_12);
  }

  for (int y = PAGE_00; y <= (PAGE_15); y += PAGE_SIZE) {
    usersettings_t * flash_config = (usersettings_t * ) y;
    if (flash_config -> a1 == 0xFF)

    {
      //If we find a page with no user settings, write them and stop looking.
      flash_write_words((uint32_t * ) y, settingsarray, 4);
      break;
    }
  }
  //confirmLed("white", 50);
}
