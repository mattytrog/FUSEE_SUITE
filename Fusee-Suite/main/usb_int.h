byte usbWriteBuffer[PACKET_CHUNK_SIZE] = {
  0
};
uint32_t usbWriteBufferUsed = 0;
uint32_t packetsWritten = 0;
EpInfo epInfo[3];

#define INTERMEZZO_SIZE 92

#define INTERMEZZO_SIZE 92
const byte intermezzo[INTERMEZZO_SIZE] = {
  0x44,
  0x00,
  0x9F,
  0xE5,
  0x01,
  0x11,
  0xA0,
  0xE3,
  0x40,
  0x20,
  0x9F,
  0xE5,
  0x00,
  0x20,
  0x42,
  0xE0,
  0x08,
  0x00,
  0x00,
  0xEB,
  0x01,
  0x01,
  0xA0,
  0xE3,
  0x10,
  0xFF,
  0x2F,
  0xE1,
  0x00,
  0x00,
  0xA0,
  0xE1,
  0x2C,
  0x00,
  0x9F,
  0xE5,
  0x2C,
  0x10,
  0x9F,
  0xE5,
  0x02,
  0x28,
  0xA0,
  0xE3,
  0x01,
  0x00,
  0x00,
  0xEB,
  0x20,
  0x00,
  0x9F,
  0xE5,
  0x10,
  0xFF,
  0x2F,
  0xE1,
  0x04,
  0x30,
  0x90,
  0xE4,
  0x04,
  0x30,
  0x81,
  0xE4,
  0x04,
  0x20,
  0x52,
  0xE2,
  0xFB,
  0xFF,
  0xFF,
  0x1A,
  0x1E,
  0xFF,
  0x2F,
  0xE1,
  0x20,
  0xF0,
  0x01,
  0x40,
  0x5C,
  0xF0,
  0x01,
  0x40,
  0x00,
  0x00,
  0x02,
  0x40,
  0x00,
  0x00,
  0x01,
  0x40,
};

int foundTegra = 0;
byte tegraDeviceAddress = 1;
USBHost usb;

void
readTegraDeviceID(byte * deviceID) {
  byte readLength = 16;
  UHD_Pipe_Alloc(tegraDeviceAddress, 0x01, USB_HOST_PTYPE_BULK, USB_EP_DIR_IN,
    0x40, 0, USB_HOST_NB_BK_1);
  usb.inTransfer(tegraDeviceAddress, 0x01, & readLength, deviceID);
}

void
setupTegraDevice() {
  epInfo[0].epAddr = 0;
  epInfo[0].maxPktSize = 0x40;
  epInfo[0].epAttribs = USB_TRANSFER_TYPE_CONTROL;
  epInfo[0].bmNakPower = USB_NAK_MAX_POWER;
  epInfo[0].bmSndToggle = 0;
  epInfo[0].bmRcvToggle = 0;

  epInfo[1].epAddr = 0x01;
  epInfo[1].maxPktSize = 0x40;
  epInfo[1].epAttribs = USB_TRANSFER_TYPE_BULK;
  epInfo[1].bmNakPower = USB_NAK_MAX_POWER;
  epInfo[1].bmSndToggle = 0;
  epInfo[1].bmRcvToggle = 0;

  usb.setEpInfoEntry(tegraDeviceAddress, 2, epInfo);
  usb.setConf(tegraDeviceAddress, 0, 0);
  usb.Task();

  UHD_Pipe_Alloc(tegraDeviceAddress, 0x01, USB_HOST_PTYPE_BULK, USB_EP_DIR_IN,
    0x40, 0, USB_HOST_NB_BK_1);
}

void
findTegraDevice(UsbDeviceDefinition * pdev) {
  uint32_t address = pdev -> address.devAddress;
  //  if ( usb.getUsbTaskState() >= 0x80 )
  //  {
  USB_DEVICE_DESCRIPTOR deviceDescriptor;
  if (usb.getDevDescr(address, 0, 0x12, (uint8_t * ) & deviceDescriptor)) {
    return;
  }

  //  if (deviceDescriptor.idVendor == 0x057e && deviceDescriptor.idProduct ==
  //  0x2000)
  //  {
  //    while(true)
  //    {
  //      ledBlink("white",1, 500000, 500000);
  //    }
  //  }

  if (deviceDescriptor.idVendor == 0x0955 &&
    deviceDescriptor.idProduct == 0x7321) {
    tegraDeviceAddress = address;
    foundTegra = 1;
  }
  //}
}

void
usbOutTransferChunk(uint32_t addr, uint32_t ep, uint32_t nbytes, uint8_t * data) {

  EpInfo * epInfo = usb.getEpInfoEntry(addr, ep);

  usb_pipe_table[epInfo -> epAddr].HostDescBank[0].CTRL_PIPE.bit.PDADDR = addr;

  if (epInfo -> bmSndToggle)
    USB -> HOST.HostPipe[epInfo -> epAddr].PSTATUSSET.reg = USB_HOST_PSTATUSSET_DTGL;
  else
    USB -> HOST.HostPipe[epInfo -> epAddr].PSTATUSCLR.reg = USB_HOST_PSTATUSCLR_DTGL;

  UHD_Pipe_Write(epInfo -> epAddr, PACKET_CHUNK_SIZE, data);
  uint32_t rcode = usb.dispatchPkt(tokOUT, epInfo -> epAddr, 15);
  if (rcode) {
    if (rcode == USB_ERROR_DATATOGGLE) {
      epInfo -> bmSndToggle = USB_HOST_DTGL(epInfo -> epAddr);
      if (epInfo -> bmSndToggle)
        USB -> HOST.HostPipe[epInfo -> epAddr].PSTATUSSET.reg = USB_HOST_PSTATUSSET_DTGL;
      else
        USB -> HOST.HostPipe[epInfo -> epAddr].PSTATUSCLR.reg = USB_HOST_PSTATUSCLR_DTGL;
    } else {
      return;
    }
  }

  epInfo -> bmSndToggle = USB_HOST_DTGL(epInfo -> epAddr);
}

void
usbFlushBuffer() {
  usbOutTransferChunk(tegraDeviceAddress, 0x01, PACKET_CHUNK_SIZE,
    usbWriteBuffer);

  memset(usbWriteBuffer, 0, PACKET_CHUNK_SIZE);
  usbWriteBufferUsed = 0;
  packetsWritten++;
}

void
usbBufferedWrite(const byte * data, uint32_t length) {
  while (usbWriteBufferUsed + length >= PACKET_CHUNK_SIZE) {
    uint32_t bytesToWrite
      = min(PACKET_CHUNK_SIZE - usbWriteBufferUsed, length);
    memcpy(usbWriteBuffer + usbWriteBufferUsed, data, bytesToWrite);
    usbWriteBufferUsed += bytesToWrite;
    usbFlushBuffer();
    data += bytesToWrite;
    length -= bytesToWrite;
  }

  if (length > 0) {
    memcpy(usbWriteBuffer + usbWriteBufferUsed, data, length);
    usbWriteBufferUsed += length;
  }
}

void
usbBufferedWriteU32(uint32_t data) {
  usbBufferedWrite((byte * ) & data, 4);
}

void
sendPayload(const byte * payload, uint32_t payloadLength) {
  byte zeros[0x1000] = {
    0
  };

  usbBufferedWriteU32(0x30298);
  usbBufferedWrite(zeros, 680 - 4);

  for (uint32_t i = 0; i < 0x3C00; i++)
    usbBufferedWriteU32(0x4001F000);

  usbBufferedWrite(intermezzo, INTERMEZZO_SIZE);
  usbBufferedWrite(zeros, 0xFA4);
  usbBufferedWrite(payload, payloadLength);
  usbFlushBuffer();
}

void
send_fusee() {
  const PROGMEM byte TIMERMAGIC[8] = {
    SELECTION_RELEASE_TIME,
    SELECTION_CONFIRM_TIME,
    SAFETY_PRESS_BARRIER,
    BUTTON_GAP,
    OPTIONS_TO_SHOW,
    SETTINGSMENU_HOLD_TIME,
    BOOTLOGO_DISP_TIME,
    STG_TIMEOUT
  };

  byte zeros[0x1000] = {
    0
  };

  usbBufferedWriteU32(0x30298);
  usbBufferedWrite(zeros, 680 - 4);

  for (uint32_t i = 0; i < 0x3C00; i++)
    usbBufferedWriteU32(0x4001F000);

  usbBufferedWrite(intermezzo, INTERMEZZO_SIZE);
  usbBufferedWrite(zeros, 0xFA4);

  usbBufferedWrite(FS1, 100096);

  if (EEPROM_SETTINGS_LOCKOUT) usbBufferedWrite(CHIP_OPTIONS_DISABLED_WHITESPACE, 32);
  else usbBufferedWrite(REBOOT_CHIP_SETUP_ASSISTANT, 32);
  usbBufferedWrite(FS1a, 2100);
  usbBufferedWrite(TIMERMAGIC, 8);

  if (EEPROM_BOOT_OPTIONS_AVAILABLE == 3) {
    if (EEPROM_DUAL_BOOT_TOGGLE == 1)
      usbBufferedWrite(ACTIVATE_CHIP_RCM_MODE_MENU, 26);
    if (EEPROM_DUAL_BOOT_TOGGLE == 2)
      usbBufferedWrite(ACTIVATE_FAILSAFE_RCM_MODE_MENU, 26);
    if (EEPROM_DUAL_BOOT_TOGGLE == 3)
      usbBufferedWrite(ACTIVATE_DUAL_BOOT_MODE_MENU, 26);
  } else if (EEPROM_BOOT_OPTIONS_AVAILABLE == 2) {
    if (EEPROM_DUAL_BOOT_TOGGLE == 1)
      usbBufferedWrite(ACTIVATE_FAILSAFE_RCM_MODE_MENU, 26);
    if (EEPROM_DUAL_BOOT_TOGGLE == 3)
      usbBufferedWrite(ACTIVATE_DUAL_BOOT_MODE_MENU, 26);
  } else {
    if (EEPROM_DUAL_BOOT_TOGGLE == 1)
      usbBufferedWrite(ACTIVATE_CHIP_RCM_MODE_MENU, 26);
    if (EEPROM_DUAL_BOOT_TOGGLE == 2)
      usbBufferedWrite(ACTIVATE_DUAL_BOOT_MODE_MENU, 26);
  }
  usbBufferedWrite(EMPTY, 6);
  if (EEPROM_COLOUR == 0)
    usbBufferedWrite(RED_BOOTLOGO_FALSE, 22);
  else if (EEPROM_COLOUR == 1)
    usbBufferedWrite(RED_BOOTLOGO_TRUE, 22);

  usbBufferedWrite(FS2, 42);
  usbBufferedWrite(LPRESS_MENU, 25);

  usbBufferedWrite(FS3, 456);

  if (EEPROM_JOYCON_CONTROL_STRAP == 1)
    usbBufferedWrite(STRAP_SEARCH_INFO_TRUE, 36);
  else
    usbBufferedWrite(STRAP_SEARCH_INFO_FALSE, 36);

  usbBufferedWrite(FS4, 7);

  if (EEPROM_VOL_CONTROL_STRAP == 1)
    usbBufferedWrite(STRAP_SEARCH_INFO_TRUE, 36);
  else
    usbBufferedWrite(STRAP_SEARCH_INFO_FALSE, 36);

  usbBufferedWrite(FS4, 7);

  if (EEPROM_USB_STRAP == 2) usbBufferedWrite(CHARGER_FAULT, 36);
  else if (EEPROM_USB_STRAP == 1)
    usbBufferedWrite(STRAP_SEARCH_INFO_TRUE, 36);
  else
    usbBufferedWrite(STRAP_SEARCH_INFO_FALSE, 36);
  usbBufferedWrite(FS5, 2845);

  if (EEPROM_DUAL_BOOT_TOGGLE == 1)
    usbBufferedWrite(DUALBOOT_MODE_ACTIVE_INFO, 22);
  if (EEPROM_DUAL_BOOT_TOGGLE == 2)
    usbBufferedWrite(CHIP_RCM_MODE_ACTIVE_INFO, 22);
  if (EEPROM_DUAL_BOOT_TOGGLE == 3)
    usbBufferedWrite(FAILSAFE_RCM_MODE_ACTIVE_INFO, 22);

  usbBufferedWrite(EMPTY, 10);

  if (EEPROM_SETTINGS_LOCKOUT) usbBufferedWrite(LPRESS_INFO_SECONDS, 22);
  else {
    if (TEMP_EEPROM_SETTINGS_CHANGE == 1)
      usbBufferedWrite(LPRESS_INFO_FALSE, 22);
    else
      usbBufferedWrite(LPRESS_INFO_TRUE, 22);
  }

  usbBufferedWrite(EMPTY, 10);

  if (EEPROM_COLOUR == 0)
    usbBufferedWrite(COLOUR0_SETTING_TRUE, 6);
  else if (EEPROM_COLOUR == 1)
    usbBufferedWrite(COLOUR1_SETTING_TRUE, 6);

  usbBufferedWrite(EMPTY, 1);

  if (TEMP_EEPROM_SETTINGS_CHANGE == 1)
    usbBufferedWrite(LPRESS_SETTING_FALSE, 6);
  else
    usbBufferedWrite(LPRESS_SETTING_TRUE, 6);

  usbBufferedWrite(FS6, 19664);
  usbFlushBuffer();
}
