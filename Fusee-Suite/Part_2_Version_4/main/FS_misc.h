#include <Arduino.h>



const PROGMEM byte FSDUALBOOT[33] = {
  0x0A, 0x44, 0x75, 0x61, 0x6C, 0x2D, 0x42, 0x6F, 0x6F, 0x74, 0x20, 0x6D,
  0x6F, 0x64, 0x65, 0x20, 0x69, 0x73, 0x20, 0x61, 0x63, 0x74, 0x69, 0x76,
  0x65, 0x2E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0A
};



const PROGMEM byte FSAUTORCM[33] = {
  0x0A, 0x43, 0x68, 0x69, 0x70, 0x2D, 0x62, 0x61, 0x73, 0x65, 0x64, 0x20,
  0x61, 0x75, 0x74, 0x6F, 0x52, 0x43, 0x4D, 0x20, 0x6D, 0x6F, 0x64, 0x65,
  0x20, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x2E, 0x0A
};




const PROGMEM byte FILL0[71] = {
  0x00, 0x00, 0x00, 0x52, 0x65, 0x6E, 0x61, 0x6D, 0x65, 0x20, 0x61, 0x20,
  0x66, 0x69, 0x6C, 0x65, 0x20, 0x74, 0x6F, 0x20, 0x62, 0x6F, 0x6F, 0x74,
  0x6C, 0x6F, 0x61, 0x64, 0x65, 0x72, 0x2F, 0x75, 0x70, 0x64, 0x61, 0x74,
  0x65, 0x2E, 0x62, 0x69, 0x6E, 0x00, 0x00, 0x44, 0x69, 0x73, 0x61, 0x62,
  0x6C, 0x65, 0x20, 0x70, 0x61, 0x79, 0x6C, 0x6F, 0x61, 0x64, 0x2E, 0x62,
  0x69, 0x6E, 0x00, 0x44, 0x69, 0x73, 0x61, 0x62, 0x6C, 0x65, 0x20
};



const PROGMEM byte FILL1[63] = {
  0x00, 0x00, 0x00, 0x00, 0x44, 0x69, 0x73, 0x61, 0x62, 0x6C, 0x65, 0x20,
  0x62, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61, 0x64, 0x65, 0x72, 0x2F, 0x75,
  0x70, 0x64, 0x61, 0x74, 0x65, 0x2E, 0x62, 0x69, 0x6E, 0x00, 0x00, 0x00,
  0x45, 0x6E, 0x61, 0x62, 0x6C, 0x65, 0x20, 0x70, 0x61, 0x79, 0x6C, 0x6F,
  0x61, 0x64, 0x2E, 0x62, 0x69, 0x6E, 0x00, 0x00, 0x45, 0x6E, 0x61, 0x62,
  0x6C, 0x65, 0x20
};


const PROGMEM byte FILL2[449] = {
 0x00, 0x45, 0x6E, 0x61, 0x62, 0x6C, 0x65, 0x20, 0x62, 0x6F, 0x6F, 0x74,
  0x6C, 0x6F, 0x61, 0x64, 0x65, 0x72, 0x2F, 0x75, 0x70, 0x64, 0x61, 0x74,
  0x65, 0x2E, 0x62, 0x69, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x44, 0x65, 0x6C,
  0x65, 0x74, 0x65, 0x20, 0x66, 0x69, 0x6C, 0x65, 0x00, 0x2D, 0x2D, 0x2D,
  0x2D, 0x20, 0x45, 0x73, 0x73, 0x65, 0x6E, 0x74, 0x69, 0x61, 0x6C, 0x20,
  0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x42, 0x61, 0x63,
  0x6B, 0x75, 0x70, 0x20, 0x42, 0x4F, 0x4F, 0x54, 0x30, 0x2F, 0x31, 0x2F,
  0x50, 0x52, 0x4F, 0x44, 0x49, 0x4E, 0x46, 0x4F, 0x20, 0x74, 0x6F, 0x20,
  0x73, 0x61, 0x66, 0x65, 0x20, 0x66, 0x6F, 0x6C, 0x64, 0x65, 0x72, 0x00,
  0x00, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x20, 0x46, 0x75, 0x6C, 0x6C,
  0x20, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x00, 0x00, 0x00,
  0x00, 0x42, 0x61, 0x63, 0x6B, 0x75, 0x70, 0x20, 0x65, 0x4D, 0x4D, 0x43,
  0x20, 0x42, 0x4F, 0x4F, 0x54, 0x30, 0x2F, 0x31, 0x00, 0x42, 0x61, 0x63,
  0x6B, 0x75, 0x70, 0x20, 0x65, 0x4D, 0x4D, 0x43, 0x20, 0x52, 0x41, 0x57,
  0x20, 0x47, 0x50, 0x50, 0x00, 0x2D, 0x2D, 0x20, 0x47, 0x50, 0x50, 0x20,
  0x50, 0x61, 0x72, 0x74, 0x69, 0x74, 0x69, 0x6F, 0x6E, 0x73, 0x20, 0x2D,
  0x2D, 0x00, 0x00, 0x00, 0x00, 0x42, 0x61, 0x63, 0x6B, 0x75, 0x70, 0x20,
  0x65, 0x4D, 0x4D, 0x43, 0x20, 0x53, 0x59, 0x53, 0x00, 0x42, 0x61, 0x63,
  0x6B, 0x75, 0x70, 0x20, 0x65, 0x4D, 0x4D, 0x43, 0x20, 0x55, 0x53, 0x45,
  0x52, 0x00, 0x00, 0x00, 0x00, 0x52, 0x65, 0x73, 0x74, 0x6F, 0x72, 0x65,
  0x20, 0x73, 0x61, 0x66, 0x65, 0x20, 0x66, 0x6F, 0x6C, 0x64, 0x65, 0x72,
  0x20, 0x42, 0x4F, 0x4F, 0x54, 0x30, 0x2F, 0x31, 0x00, 0x52, 0x65, 0x73,
  0x74, 0x6F, 0x72, 0x65, 0x20, 0x73, 0x61, 0x66, 0x65, 0x20, 0x66, 0x6F,
  0x6C, 0x64, 0x65, 0x72, 0x20, 0x50, 0x52, 0x4F, 0x44, 0x49, 0x4E, 0x46,
  0x4F, 0x00, 0x00, 0x00, 0x00, 0x52, 0x65, 0x73, 0x74, 0x6F, 0x72, 0x65,
  0x20, 0x65, 0x4D, 0x4D, 0x43, 0x20, 0x42, 0x4F, 0x4F, 0x54, 0x30, 0x2F,
  0x31, 0x00, 0x00, 0x00, 0x00, 0x52, 0x65, 0x73, 0x74, 0x6F, 0x72, 0x65,
  0x20, 0x65, 0x4D, 0x4D, 0x43, 0x20, 0x52, 0x41, 0x57, 0x20, 0x47, 0x50,
  0x50, 0x00, 0x00, 0x00, 0x00, 0x52, 0x65, 0x73, 0x74, 0x6F, 0x72, 0x65,
  0x20, 0x47, 0x50, 0x50, 0x20, 0x70, 0x61, 0x72, 0x74, 0x69, 0x74, 0x69,
  0x6F, 0x6E, 0x73, 0x00, 0x00, 0x2D, 0x2D, 0x2D, 0x2D, 0x20, 0x44, 0x61,
  0x6E, 0x67, 0x65, 0x72, 0x6F, 0x75, 0x73, 0x20, 0x2D, 0x2D, 0x2D, 0x2D,
  0x2D, 0x00, 0x00, 0x00, 0x00, 0x52, 0x65, 0x73, 0x74, 0x6F, 0x72, 0x65,
  0x20, 0x42, 0x4F, 0x4F, 0x54, 0x30, 0x2F, 0x31, 0x20, 0x77, 0x69, 0x74,
  0x68, 0x6F, 0x75, 0x74, 0x20, 0x73, 0x69, 0x7A, 0x65, 0x20, 0x63, 0x68,
  0x65, 0x63, 0x6B, 0x00, 0x00, 0x62, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61,
  0x64, 0x65, 0x72, 0x2F, 0x75, 0x70, 0x64, 0x61, 0x74, 0x65, 0x2E, 0x64,
  0x69, 0x73, 0x00, 0x00, 0x00
};


const PROGMEM byte FILL3[81] = {
 0x00, 0x00, 0x00, 0x00, 0x70, 0x61, 0x79, 0x6C, 0x6F, 0x61, 0x64, 0x2E,
  0x64, 0x69, 0x73, 0x00, 0x25, 0x6B, 0x46, 0x75, 0x73, 0x65, 0x65, 0x5F,
  0x55, 0x46, 0x32, 0x20, 0x49, 0x6E, 0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74,
  0x69, 0x6F, 0x6E, 0x2E, 0x20, 0x56, 0x33, 0x5F, 0x31, 0x32, 0x31, 0x39,
  0x2E, 0x0A, 0x0A, 0x25, 0x6B, 0x46, 0x6F, 0x6C, 0x6C, 0x6F, 0x77, 0x69,
  0x6E, 0x67, 0x20, 0x53, 0x74, 0x72, 0x61, 0x70, 0x73, 0x20, 0x44, 0x65,
  0x74, 0x65, 0x63, 0x74, 0x65, 0x64, 0x3A, 0x0A, 0x0A
};

const PROGMEM byte FILL4[2] = {0x0A, 0x0A};

const PROGMEM byte FILL5[68] = {
 0x25, 0x6B, 0x4E, 0x61, 0x6D, 0x65, 0x20, 0x79, 0x6F, 0x75, 0x72, 0x20,
  0x70, 0x61, 0x79, 0x6C, 0x6F, 0x61, 0x64, 0x28, 0x73, 0x29, 0x20, 0x61,
  0x73, 0x20, 0x66, 0x6F, 0x6C, 0x6C, 0x6F, 0x77, 0x73, 0x3A, 0x0A, 0x0A,
  0x4C, 0x6F, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x3A, 0x20, 0x53, 0x44,
  0x20, 0x52, 0x6F, 0x6F, 0x74, 0x2E, 0x0A, 0x0A, 0x70, 0x61, 0x79, 0x6C,
  0x6F, 0x61, 0x64, 0x2E, 0x62, 0x69, 0x6E, 0x0A
};



const PROGMEM byte USBINFO[11] = {0x55, 0x53, 0x42, 0x20, 0x56, 0x4F, 0x4C, 0x54, 0x41, 0x47, 0x45};
const PROGMEM byte NOUSBINFO[11] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
const PROGMEM byte VOLUPINFO[11] = {0x56, 0x4F, 0x4C, 0x55, 0x4D, 0x45, 0x2B, 0x20, 0x20, 0x20, 0x20};
const PROGMEM byte NOVOLUPINFO[11] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
const PROGMEM byte JOYCONINFO[11] = {0x4A, 0x4F, 0x59, 0x43, 0x4F, 0x4E, 0x20, 0x20, 0x20, 0x20, 0x20};
const PROGMEM byte NOJOYCONINFO[11] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};

const PROGMEM byte SAMD_UPDATE[928] = {
  0x00, 0x00, 0xA0, 0xE3, 0x74, 0x10, 0x9F, 0xE5, 0x50, 0x00, 0x81, 0xE5,
  0xB4, 0x01, 0x81, 0xE5, 0x40, 0x08, 0x81, 0xE5, 0x68, 0x00, 0x9F, 0xE5,
  0x68, 0x10, 0x9F, 0xE5, 0x00, 0x00, 0x81, 0xE5, 0x04, 0x00, 0xA0, 0xE3,
  0x60, 0x10, 0x9F, 0xE5, 0x60, 0x20, 0x9F, 0xE5, 0x23, 0x00, 0x00, 0xEB,
  0x05, 0x00, 0xA0, 0xE3, 0x58, 0x10, 0x9F, 0xE5, 0x58, 0x20, 0x9F, 0xE5,
  0x1F, 0x00, 0x00, 0xEB, 0x06, 0x00, 0xA0, 0xE3, 0x50, 0x10, 0x9F, 0xE5,
  0x50, 0x20, 0x9F, 0xE5, 0x1B, 0x00, 0x00, 0xEB, 0x4C, 0x00, 0x9F, 0xE5,
  0x4C, 0x10, 0x9F, 0xE5, 0x00, 0x20, 0xA0, 0xE3, 0x48, 0x30, 0x9F, 0xE5,
  0x02, 0x40, 0x90, 0xE7, 0x02, 0x40, 0x81, 0xE7, 0x04, 0x20, 0x82, 0xE2,
  0x03, 0x00, 0x52, 0xE1, 0xFA, 0xFF, 0xFF, 0x1A, 0x34, 0x00, 0x9F, 0xE5,
  0x10, 0xFF, 0x2F, 0xE1, 0x1B, 0x00, 0x00, 0xEA, 0x00, 0xE4, 0x00, 0x70,
  0x30, 0x4C, 0x00, 0x40, 0x08, 0xF2, 0x00, 0x60, 0xDC, 0x15, 0x00, 0x00,
  0x20, 0xE0, 0x00, 0x00, 0xEE, 0x4A, 0x00, 0x00, 0x5B, 0xE0, 0x00, 0x00,
  0x88, 0x4E, 0x00, 0x00, 0x18, 0xE0, 0x00, 0x00, 0x00, 0xF1, 0x03, 0x40,
  0x40, 0x00, 0x01, 0x40, 0xC0, 0x02, 0x00, 0x00, 0x10, 0x10, 0x10, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x14, 0x30, 0x9F, 0xE5, 0x00, 0x01, 0xA0, 0xE1, 0xA1, 0x10, 0xA0, 0xE1,
  0x01, 0x18, 0xA0, 0xE1, 0x02, 0x10, 0x81, 0xE1, 0x00, 0x10, 0x83, 0xE7,
  0x1E, 0xFF, 0x2F, 0xE1, 0x00, 0xDC, 0x01, 0x60, 0xF8, 0xB5, 0xC0, 0x46,
  0xF8, 0xBC, 0x08, 0xBC, 0x9E, 0x46, 0x70, 0x47, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xC0, 0x9F, 0xE5, 0x1C, 0xFF, 0x2F, 0xE1, 0x05, 0x01, 0x01, 0x40,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xD0, 0x1F, 0xE5, 0x9F, 0x00, 0x00, 0xEA,
  0x00, 0x00, 0x01, 0x40, 0x06, 0x48, 0x07, 0x4B, 0x10, 0xB5, 0x83, 0x42,
  0x04, 0xD0, 0x06, 0x4B, 0x00, 0x2B, 0x01, 0xD0, 0x00, 0xF0, 0x0A, 0xF8,
  0x10, 0xBC, 0x01, 0xBC, 0x00, 0x47, 0xC0, 0x46, 0xD8, 0x02, 0x01, 0x40,
  0xD8, 0x02, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x18, 0x47, 0xC0, 0x46,
  0x08, 0x48, 0x09, 0x49, 0x09, 0x1A, 0x89, 0x10, 0xCB, 0x0F, 0x59, 0x18,
  0x10, 0xB5, 0x49, 0x10, 0x04, 0xD0, 0x06, 0x4B, 0x00, 0x2B, 0x01, 0xD0,
  0x00, 0xF0, 0x0A, 0xF8, 0x10, 0xBC, 0x01, 0xBC, 0x00, 0x47, 0xC0, 0x46,
  0xD8, 0x02, 0x01, 0x40, 0xD8, 0x02, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x18, 0x47, 0xC0, 0x46, 0x10, 0xB5, 0x08, 0x4C, 0x23, 0x78, 0x00, 0x2B,
  0x09, 0xD1, 0xFF, 0xF7, 0xC9, 0xFF, 0x06, 0x4B, 0x00, 0x2B, 0x02, 0xD0,
  0x05, 0x48, 0x00, 0xE0, 0x00, 0xBF, 0x01, 0x23, 0x23, 0x70, 0x10, 0xBC,
  0x01, 0xBC, 0x00, 0x47, 0xE0, 0x02, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00,
  0xD8, 0x02, 0x01, 0x40, 0x06, 0x4B, 0x10, 0xB5, 0x00, 0x2B, 0x03, 0xD0,
  0x05, 0x49, 0x06, 0x48, 0x00, 0xE0, 0x00, 0xBF, 0xFF, 0xF7, 0xC4, 0xFF,
  0x10, 0xBC, 0x01, 0xBC, 0x00, 0x47, 0xC0, 0x46, 0x00, 0x00, 0x00, 0x00,
  0xE4, 0x02, 0x01, 0x40, 0xD8, 0x02, 0x01, 0x40, 0x82, 0x20, 0x56, 0x4B,
  0xC0, 0x00, 0x1A, 0x58, 0x55, 0x49, 0x11, 0x40, 0x80, 0x22, 0x12, 0x02,
  0x0A, 0x43, 0xD0, 0x21, 0x10, 0xB5, 0x1A, 0x50, 0x81, 0x22, 0x58, 0x58,
  0xD2, 0x05, 0x02, 0x43, 0x5A, 0x50, 0xAB, 0x22, 0x90, 0x39, 0x92, 0x00,
  0x99, 0x50, 0x80, 0x21, 0x18, 0x3A, 0xC9, 0x02, 0x99, 0x50, 0xC0, 0x21,
  0x70, 0x32, 0x49, 0x05, 0x99, 0x50, 0x4A, 0x4A, 0x11, 0x68, 0x13, 0x68,
  0x5B, 0x1A, 0x02, 0x2B, 0xFB, 0xD9, 0x80, 0x23, 0x47, 0x48, 0x01, 0x68,
  0xDB, 0x00, 0x19, 0x43, 0x01, 0x60, 0x01, 0x21, 0x45, 0x4C, 0x20, 0x68,
  0x88, 0x43, 0x20, 0x60, 0x44, 0x4C, 0x20, 0x68, 0x18, 0x43, 0x20, 0x60,
  0x43, 0x4C, 0x20, 0x68, 0x88, 0x43, 0x20, 0x60, 0x42, 0x4C, 0x20, 0x68,
  0x18, 0x43, 0x20, 0x60, 0x41, 0x4C, 0x20, 0x68, 0x88, 0x43, 0x20, 0x60,
  0x40, 0x4C, 0x20, 0x68, 0x18, 0x43, 0x20, 0x60, 0x3F, 0x4C, 0x20, 0x68,
  0x88, 0x43, 0x20, 0x60, 0x3E, 0x48, 0x04, 0x68, 0x23, 0x43, 0x03, 0x60,
  0x3D, 0x48, 0x03, 0x68, 0x8B, 0x43, 0x03, 0x60, 0x04, 0x20, 0x3C, 0x49,
  0x0B, 0x68, 0x03, 0x43, 0x0B, 0x60, 0x01, 0x21, 0x3A, 0x4B, 0x49, 0x42,
  0x19, 0x60, 0x11, 0x68, 0x2C, 0x4A, 0x13, 0x68, 0x5B, 0x1A, 0x02, 0x2B,
  0xFB, 0xD9, 0xAA, 0x22, 0x40, 0x21, 0x27, 0x4B, 0x92, 0x00, 0x99, 0x50,
  0xC0, 0x21, 0x58, 0x32, 0x49, 0x05, 0x99, 0x50, 0x80, 0x20, 0xA4, 0x21,
  0xC0, 0x02, 0x89, 0x00, 0x58, 0x50, 0xD1, 0x39, 0xFF, 0x39, 0x59, 0x61,
  0x2E, 0x49, 0x19, 0x61, 0x2E, 0x49, 0x99, 0x61, 0xD8, 0x21, 0x2E, 0x48,
  0x89, 0x00, 0x58, 0x50, 0x2D, 0x48, 0x04, 0x31, 0x58, 0x50, 0x2D, 0x48,
  0xE4, 0x39, 0x58, 0x50, 0x18, 0x31, 0x5A, 0x50, 0x00, 0x22, 0xA1, 0x39,
  0xFF, 0x39, 0x5A, 0x50, 0x04, 0x31, 0x5A, 0x50, 0xE8, 0x21, 0x89, 0x00,
  0x5A, 0x50, 0x04, 0x31, 0x5A, 0x50, 0x26, 0x49, 0x5A, 0x50, 0xD0, 0x21,
  0x25, 0x48, 0x5A, 0x58, 0x02, 0x40, 0x5A, 0x50, 0x82, 0x21, 0xC9, 0x00,
  0x5A, 0x58, 0x0E, 0x48, 0x02, 0x40, 0xA4, 0x20, 0x5A, 0x50, 0x80, 0x21,
  0x40, 0x00, 0x1A, 0x58, 0xD2, 0x00, 0x09, 0x06, 0xD2, 0x08, 0x0A, 0x43,
  0x1A, 0x50, 0x38, 0x30, 0x1A, 0x58, 0xD2, 0x00, 0xD2, 0x08, 0x0A, 0x43,
  0x1A, 0x50, 0xD4, 0x20, 0xC0, 0x00, 0x1A, 0x58, 0xD2, 0x00, 0xD2, 0x08,
  0x11, 0x43, 0x19, 0x50, 0xFE, 0xE7, 0xC0, 0x46, 0x00, 0x60, 0x00, 0x60,
  0xFF, 0x3F, 0xFF, 0xFF, 0x10, 0x50, 0x00, 0x60, 0xA0, 0x10, 0x2D, 0x70,
  0x88, 0x10, 0x2D, 0x70, 0xA0, 0x11, 0x2D, 0x70, 0x88, 0x11, 0x2D, 0x70,
  0xA0, 0x12, 0x2D, 0x70, 0x88, 0x12, 0x2D, 0x70, 0xA0, 0x13, 0x2D, 0x70,
  0x88, 0x13, 0x2D, 0x70, 0xA0, 0x14, 0x2D, 0x70, 0x88, 0x14, 0x2D, 0x70,
  0xF8, 0x0C, 0x20, 0x54, 0x8C, 0x00, 0x34, 0x54, 0x30, 0x01, 0x00, 0x80,
  0x00, 0x02, 0xF0, 0x01, 0x08, 0x08, 0x40, 0x80, 0xFC, 0x00, 0x20, 0x40,
  0x80, 0x07, 0x00, 0x23, 0x54, 0x05, 0x00, 0x00, 0xFF, 0xFF, 0x7F, 0x1F,
  0xF8, 0xB5, 0xC0, 0x46, 0xF8, 0xBC, 0x08, 0xBC, 0x9E, 0x46, 0x70, 0x47,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x9F, 0xE5, 0x1C, 0xFF, 0x2F, 0xE1,
  0x05, 0x01, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0xDD, 0x00, 0x01, 0x40,
  0xAD, 0x00, 0x01, 0x40
};