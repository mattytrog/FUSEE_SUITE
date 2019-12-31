#include <Arduino.h>


#define INTERMEZZO_SIZE 92

#define INTERMEZZO_SIZE 92
const byte intermezzo[INTERMEZZO_SIZE] =
{
  0x44, 0x00, 0x9F, 0xE5, 0x01, 0x11, 0xA0, 0xE3, 0x40, 0x20, 0x9F, 0xE5, 0x00, 0x20, 0x42, 0xE0,
  0x08, 0x00, 0x00, 0xEB, 0x01, 0x01, 0xA0, 0xE3, 0x10, 0xFF, 0x2F, 0xE1, 0x00, 0x00, 0xA0, 0xE1,
  0x2C, 0x00, 0x9F, 0xE5, 0x2C, 0x10, 0x9F, 0xE5, 0x02, 0x28, 0xA0, 0xE3, 0x01, 0x00, 0x00, 0xEB,
  0x20, 0x00, 0x9F, 0xE5, 0x10, 0xFF, 0x2F, 0xE1, 0x04, 0x30, 0x90, 0xE4, 0x04, 0x30, 0x81, 0xE4,
  0x04, 0x20, 0x52, 0xE2, 0xFB, 0xFF, 0xFF, 0x1A, 0x1E, 0xFF, 0x2F, 0xE1, 0x20, 0xF0, 0x01, 0x40,
  0x5C, 0xF0, 0x01, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x01, 0x40,
};

#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.print (x)
#define DEBUG_PRINTLN(x)  Serial.println (x)
#define DEBUG_PRINTHEX(x,y)  serialPrintHex (x,y)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTHEX(x,y)
#endif

#define PACKET_CHUNK_SIZE 0x1000

void usbOutTransferChunk(uint32_t addr, uint32_t ep, uint32_t nbytes, uint8_t* data)
{


  EpInfo* epInfo = usb.getEpInfoEntry(addr, ep);

  usb_pipe_table[epInfo->epAddr].HostDescBank[0].CTRL_PIPE.bit.PDADDR = addr;

  if (epInfo->bmSndToggle)
    USB->HOST.HostPipe[epInfo->epAddr].PSTATUSSET.reg = USB_HOST_PSTATUSSET_DTGL;
  else
    USB->HOST.HostPipe[epInfo->epAddr].PSTATUSCLR.reg = USB_HOST_PSTATUSCLR_DTGL;

  UHD_Pipe_Write(epInfo->epAddr, PACKET_CHUNK_SIZE, data);
  uint32_t rcode = usb.dispatchPkt(tokOUT, epInfo->epAddr, 15);
  if (rcode)
  {
    if (rcode == USB_ERROR_DATATOGGLE)
    {
      epInfo->bmSndToggle = USB_HOST_DTGL(epInfo->epAddr);
      if (epInfo->bmSndToggle)
        USB->HOST.HostPipe[epInfo->epAddr].PSTATUSSET.reg = USB_HOST_PSTATUSSET_DTGL;
      else
        USB->HOST.HostPipe[epInfo->epAddr].PSTATUSCLR.reg = USB_HOST_PSTATUSCLR_DTGL;
    }
    else
    {
      #ifdef DOTSTAR_ENABLED
      strip.setPixelColor(0, 64, 0, 0); strip.show();
      #endif
      return;
    }
  }

  epInfo->bmSndToggle = USB_HOST_DTGL(epInfo->epAddr);
}

void usbFlushBuffer()
{
  usbOutTransferChunk(tegraDeviceAddress, 0x01, PACKET_CHUNK_SIZE, usbWriteBuffer);

  memset(usbWriteBuffer, 0, PACKET_CHUNK_SIZE);
  usbWriteBufferUsed = 0;
  packetsWritten++;
}

// This accepts arbitrary sized USB writes and will automatically chunk them into writes of size 0x1000 and increment
// packetsWritten every time a chunk is written out.
void usbBufferedWrite(const byte *data, uint32_t length)
{
  while (usbWriteBufferUsed + length >= PACKET_CHUNK_SIZE)
  {
    uint32_t bytesToWrite = min(PACKET_CHUNK_SIZE - usbWriteBufferUsed, length);
    memcpy(usbWriteBuffer + usbWriteBufferUsed, data, bytesToWrite);
    usbWriteBufferUsed += bytesToWrite;
    usbFlushBuffer();
    data += bytesToWrite;
    length -= bytesToWrite;
  }

  if (length > 0)
  {
    memcpy(usbWriteBuffer + usbWriteBufferUsed, data, length);
    usbWriteBufferUsed += length;
  }
}

void usbBufferedWriteU32(uint32_t data)
{
  usbBufferedWrite((byte *)&data, 4);
}

void readTegraDeviceID(byte *deviceID)
{
  byte readLength = 16;
  UHD_Pipe_Alloc(tegraDeviceAddress, 0x01, USB_HOST_PTYPE_BULK, USB_EP_DIR_IN, 0x40, 0, USB_HOST_NB_BK_1);
  if (usb.inTransfer(tegraDeviceAddress, 0x01, &readLength, deviceID))
    DEBUG_PRINTLN("Failed to get device ID!");
}

void sendPayload(const byte *payload, uint32_t payloadLength)
{
  byte zeros[0x1000] = {0};

  usbBufferedWriteU32(0x30298);
  usbBufferedWrite(zeros, 680 - 4);

  for (uint32_t i = 0; i < 0x3C00; i++)
    usbBufferedWriteU32(0x4001F000);

  usbBufferedWrite(intermezzo, INTERMEZZO_SIZE);
  usbBufferedWrite(zeros, 0xFA4);
  usbBufferedWrite(payload, payloadLength);
  usbFlushBuffer();
}

void send_fusee(const byte *payload)
{

  byte zeros[0x1000] = {0};

  usbBufferedWriteU32(0x30298);
  usbBufferedWrite(zeros, 680 - 4);

  for (uint32_t i = 0; i < 0x3C00; i++)
    usbBufferedWriteU32(0x4001F000);

  usbBufferedWrite(intermezzo, INTERMEZZO_SIZE);
  usbBufferedWrite(zeros, 0xFA4);

  usbBufferedWrite(FS1,88389);
  usbBufferedWrite(payload, 12);
  usbBufferedWrite(FILL0,71);
  usbBufferedWrite(payload, 12);
  usbBufferedWrite(FILL1,63);
  usbBufferedWrite(payload, 12);
  usbBufferedWrite(FILL2,449);

  if (EEPROM_PAYLOAD_NUMBER == 1) {usbBufferedWrite(DIS1,12);}
  if (EEPROM_PAYLOAD_NUMBER == 2) {usbBufferedWrite(DIS1,12);}
  if (EEPROM_PAYLOAD_NUMBER == 3) {usbBufferedWrite(DIS1,12);}
  if (EEPROM_PAYLOAD_NUMBER == 4) {usbBufferedWrite(DIS1,12);}
  if (EEPROM_PAYLOAD_NUMBER == 5) {usbBufferedWrite(DIS1,12);}
  if (EEPROM_PAYLOAD_NUMBER == 6) {usbBufferedWrite(DIS1,12);}
  if (EEPROM_PAYLOAD_NUMBER == 7) {usbBufferedWrite(DIS1,12);}
  if (EEPROM_PAYLOAD_NUMBER == 8) {usbBufferedWrite(DIS1,12);}  
  usbBufferedWrite(FILL3,81);
  
  if(EEPROM_USB_REBOOT_STRAP == 1) usbBufferedWrite(USBINFO,11);
  else usbBufferedWrite(NOUSBINFO,11);
  usbBufferedWrite(FILL4,2);
  if(EEPROM_VOL_CONTROL_STRAP == 1) usbBufferedWrite(VOLUPINFO,11);
  else usbBufferedWrite(NOVOLUPINFO,11);
  usbBufferedWrite(FILL4,2);
  if(EEPROM_JOYCON_CONTROL_STRAP == 1) usbBufferedWrite(JOYCONINFO,11);
  else usbBufferedWrite(NOJOYCONINFO,11);
  usbBufferedWrite(FILL4,2);
  if (EEPROM_DUAL_BOOT_TOGGLE == 1) usbBufferedWrite(FSDUALBOOT,33);
    else usbBufferedWrite(FSAUTORCM,33);
  usbBufferedWrite(FILL4,2);
  usbBufferedWrite(FILL5,68);
  usbBufferedWrite(payload, 12);
  
  usbBufferedWrite(FS2,12233);
  usbFlushBuffer();
}

void findTegraDevice(UsbDeviceDefinition *pdev)
{
  uint32_t address = pdev->address.devAddress;
  USB_DEVICE_DESCRIPTOR deviceDescriptor;
  if (usb.getDevDescr(address, 0, 0x12, (uint8_t *)&deviceDescriptor))
  {
    return;
  }

  if (deviceDescriptor.idVendor == 0x0955 && deviceDescriptor.idProduct == 0x7321)
  {
    tegraDeviceAddress = address;
    foundTegra = true;
  }
}

void setupTegraDevice()
{
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

  UHD_Pipe_Alloc(tegraDeviceAddress, 0x01, USB_HOST_PTYPE_BULK, USB_EP_DIR_IN, 0x40, 0, USB_HOST_NB_BK_1);

  
}

void isfitted() {
  if (!EEPROM_JOYCON_CONTROL_STRAP) {
    EEPROM_JOYCON_CONTROL_STRAP = 1;
    writeSettings();
  }
}

extern void wakeup();

void sleep(int errorCode) {
if (errorCode == 1) setLedColor("green");
else setLedColor("red");

delayMicroseconds(DELAY_BEFORE_SLEEP);
setLedColor("black");
delayMicroseconds(DELAY_BEFORE_SLEEP);

  

  if (EEPROM_PAYLOAD_INDICATION == 0)
  {
    ledBlink("red", EEPROM_PAYLOAD_NUMBER, 25000, 375000);
  }
  
  if (AUTO_INCREASE_PAYLOAD_on == 1 && errorCode != 1) {
    ++EEPROM_PAYLOAD_NUMBER;
    if (EEPROM_PAYLOAD_NUMBER > 8) EEPROM_PAYLOAD_NUMBER = 1;
    writeSettings();
  }
  
  foundTegra = false;

  #ifdef USB_LOGIC
  digitalWrite(USB_LOGIC, HIGH);
  #endif
  delayMicroseconds(500000);
  #ifdef JOYCON_STRAP_PIN
  pinMode(JOYCON_STRAP_PIN, INPUT);
  if (!EEPROM_JOYCON_CONTROL_STRAP) attachInterrupt(JOYCON_STRAP_PIN, isfitted, FALLING);
#endif

#ifdef VOLUP_STRAP_PIN
  pinMode(VOLUP_STRAP_PIN, INPUT_PULLUP);
  attachInterrupt(VOLUP_STRAP_PIN, long_press, LOW);
#endif

#ifdef WAKEUP_PIN
  pinMode(WAKEUP_PIN, INPUT);
  attachInterrupt(WAKEUP_PIN, wakeup, RISING);
#endif

#ifdef USB_LOW_RESET
  pinMode(USB_LOW_RESET, INPUT);
  if (EEPROM_USB_REBOOT_STRAP == 1) attachInterrupt(USB_LOW_RESET, wakeup, FALLING);
#endif
  
  if (DISABLE_USB == 1) {

    USB->DEVICE.CTRLB.bit.DETACH = 1;
    USB->DEVICE.CTRLA.bit.ENABLE = 0;
    while (USB->DEVICE.SYNCBUSY.bit.ENABLE | USB->DEVICE.CTRLA.bit.ENABLE == 0);
  }
  

  

  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; /* Enable deepsleep */

  GCLK->CLKCTRL.reg = uint16_t(
                        GCLK_CLKCTRL_CLKEN |
                        GCLK_CLKCTRL_GEN_GCLK2 |
                        GCLK_CLKCTRL_ID( GCLK_CLKCTRL_ID_EIC_Val )
                      );
  while (GCLK->STATUS.bit.SYNCBUSY) {}
  
  __DSB(); /* Ensure effect of last store takes effect */
  __WFI(); /* Enter sleep mode */
}
