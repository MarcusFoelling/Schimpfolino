/*
    oled.cpp - SSD1306 and SH1106 OLED driver library
    based on SSD1306_minimal.cpp
  
    Copyright (c) 2012 Adafruit Industries. All rights reserved
    Copyright (c) 2012 GOF Electronics Co. Ltd (http://www.geekonfire.com)
    Copyright (c) 2015 CoPiino Electronics. All rights reserved
    Copyright (c) 2016 Kirk Northrop (github.com/kirknorthrop)
    Copyright (c) 2024 Nikolai Radke (http://www.monstermaker.de)
  
    Original Author: Limor Fried/Ladyada Adafruit Industries
    Modified by: Jimbo.we (www.geekonfire.com)
    Modified by: CoPiino Electronics (http://copiino.cc)
    Modified by: Kirk Northrop (github.com/kirknorthrop)
    Modified by: Nikolai Radke (www.monstermaker.de)
                 -- Reworked formatting and spelling
                 -- Removed unused code
                 -- Modified for TinyI2C.h instead of TinyWireM.h
                 -- Optimized init sequence and functions
                 -- SH1106 support by Sebastian Völke
                 -- Bold font by Sebastian Völke

    What is it?
      This library is derived from GOFi2cOLED library, only for SSD1306 in I2C Mode.
      As the original library only supports frame buffered mode which requires to have
      at least 1024 bytes of free RAM for a 128 x 64 px display it is too big for smaller devices.

      So this a SDD1306/SH1106 library that works great with ATtiny45/85 devices :)

    It is a free software; you can redistribute it and/or modify it under the terms of 
    BSD license, check LICENSE for more information. All text above must be included in 
    any redistribution.
*/

#include <util/delay.h>                          // Needs less flash memory than delay()
#include "TinyI2CMaster.h"                       // Arduino and ATtiny I2C library
#include "oled.h"

#define InitLength 11                            // Number of init commands

const uint8_t InitSequence[] PROGMEM = {
  0x20,             	                           // Set addressing mode
  0x00,                                          // Horizontal mode
  0xA8, 0x3F,                                    // Set multiplex. Height: 64 - 1
  0x81,                                          // Set contrast
  0xff,                                          // Highest level
  0x8D, 0x14,                                    // Set charge pump
  0xAF,                                          // Display ON
  0xA1, 0xC8                                     // Flip the screen
};

// Reworked 5x8 font table with only used characters
#ifdef bold
const uint8_t BasicFont[] PROGMEM = {            // Bold font
  0xFE, 0xFF, 0x11, 0xFF, 0xFE,  // A 0
  0xFF, 0xFF, 0x89, 0xFF, 0x76,  // B 1
  0x7E, 0xFF, 0x81, 0xC3, 0x42,  // C 2
  0xFF, 0xFF, 0x81, 0xFF, 0x7E,  // D 3
  0xFF, 0xFF, 0xDB, 0xDB, 0x81,  // E 4
  0xFF, 0xFF, 0x1B, 0x0B, 0x01,  // F 5
  0x7E, 0xFF, 0xC9, 0x7B, 0xFA,  // G 6
  0xFF, 0xFF, 0x08, 0xFF, 0xFF,  // H 7
  0xC3, 0xC3, 0xFF, 0xFF, 0xC3,  // I 8
  0x42, 0xE3, 0xC3, 0xFF, 0x7E,  // J 9
  0xFF, 0xFF, 0x08, 0xF7, 0xE3,  // K 10
  0xFF, 0xFF, 0xC0, 0xC0, 0xC0,  // L 11
  0xFF, 0xFE, 0x04, 0xFE, 0xFF,  // M 12
  0xFF, 0xFE, 0x3C, 0x7F, 0xFF,  // N 13
  0x7E, 0xFF, 0x81, 0xFF, 0x7E,  // O 14
  0xFF, 0xFF, 0x09, 0x0F, 0x06,  // P 15
  0x7E, 0xFF, 0xA1, 0xFF, 0xBE,  // Q 16
  0xFF, 0xFF, 0x29, 0xCF, 0x86,  // R 17
  0x46, 0xCF, 0x89, 0xFB, 0x72,  // S 18
  0x03, 0xFF, 0xFF, 0x03, 0x03,  // T 19
  0x7F, 0xFF, 0x80, 0xFF, 0x7F,  // U 20
  0x3F, 0x7F, 0x80, 0x7F, 0x3F,  // V 21
  0x7F, 0xFF, 0x78, 0xFF, 0x7F,  // W 22
  0xE3, 0xFF, 0x08, 0xFF, 0xE3,  // X 23
  0x07, 0x0F, 0xF8, 0xFF, 0x07,  // Y 24
  0xE3, 0xF3, 0xDB, 0xCF, 0xC7,  // Z 25
  0x1C, 0x3E, 0x7C, 0x3E, 0x1C,  // ♥ 26
  0x69, 0xFD, 0x94, 0x7D, 0xFD,  // ä 27
  0x79, 0xFD, 0x84, 0xFD, 0x79,  // ö 28
  0x7D, 0xFD, 0x80, 0x7D, 0xFF,  // ü 29
  0xFF, 0xFF, 0x49, 0xD6, 0x60,  // ß 30
  0x00, 0x00, 0x00, 0x00, 0x00,  //   31 Space - unused
  0x68, 0xFC, 0x94, 0x7C, 0xF8,  // a 32 
  0xFF, 0xFF, 0x88, 0xF8, 0x70,  // b 33
  0x78, 0xFC, 0x84, 0xCC, 0x48,  // c 34
  0x70, 0xF8, 0x88, 0xFF, 0xFF,  // d 35
  0x78, 0xFC, 0xB4, 0xDC, 0x58,  // e 36
  0x08, 0xFE, 0xFF, 0x09, 0x02,  // f 37
  0x98, 0xBC, 0xA4, 0xFC, 0x7C,  // g 38
  0xFF, 0xFF, 0x08, 0xF8, 0xF0,  // h 39
  0x00, 0x84, 0xFD, 0xFD, 0x80,  // i 40
  0x40, 0x84, 0xFD, 0x7D, 0x00,  // j 41
  0xFF, 0xFF, 0x18, 0xF6, 0xE6,  // k 42
  0x00, 0x83, 0xFF, 0xFF, 0x80,  // l 43
  0xF8, 0xFC, 0x18, 0xFC, 0xF8,  // m 44
  0xFC, 0xF8, 0x04, 0xFC, 0xF8,  // n 45
  0x78, 0xFC, 0x84, 0xFC, 0x78,  // o 46
  0xFC, 0xFC, 0x24, 0x3C, 0x18,  // p 47
  0x18, 0x3C, 0x24, 0xFC, 0xFC,  // q 48
  0xFC, 0xF8, 0x04, 0x0C, 0x08,  // r 49
  0x48, 0xDC, 0xB4, 0xEC, 0x48,  // s 50
  0x08, 0x7F, 0xFF, 0x88, 0x48,  // t 51
  0x7C, 0xFC, 0x80, 0x7C, 0xFC,  // u 52
  0x3C, 0x7C, 0x80, 0x7C, 0x3C,  // v 53
  0x7C, 0xFC, 0x70, 0xFC, 0x7C,  // w 54
  0xCC, 0xFC, 0x30, 0xFC, 0xCC,  // x 55
  0x9C, 0xBC, 0xA0, 0xFC, 0x7C,  // y 56
  0xC4, 0xE4, 0xB4, 0x9C, 0x8C,  // z 57
  0xF9, 0xFD, 0x16, 0xFD, 0xF9,  // Ä 58
  0x79, 0xFD, 0x86, 0xFD, 0x79,  // Ö 59
  0x7D, 0xFD, 0x80, 0xFD, 0x7D   // Ü 60
};
#else
const uint8_t BasicFont[] PROGMEM = {            // Standard font
  0x7C, 0x12, 0x11, 0x12, 0x7C, // A 0
  0x7F, 0x49, 0x49, 0x49, 0x36, // B 1
  0x3E, 0x41, 0x41, 0x41, 0x22, // C 2
  0x7F, 0x41, 0x41, 0x22, 0x1C, // D 3
  0x7F, 0x49, 0x49, 0x49, 0x41, // E 4
  0x7F, 0x09, 0x09, 0x09, 0x01, // F 5
  0x3E, 0x41, 0x49, 0x49, 0x7A, // G 6
  0x7F, 0x08, 0x08, 0x08, 0x7F, // H 7
  0x00, 0x41, 0x7F, 0x41, 0x00, // I 8
  0x20, 0x40, 0x41, 0x3F, 0x01, // J 9
  0x7F, 0x08, 0x14, 0x22, 0x41, // K 10
  0x7F, 0x40, 0x40, 0x40, 0x40, // L 11
  0x7F, 0x02, 0x0C, 0x02, 0x7F, // M 12
  0x7F, 0x04, 0x08, 0x10, 0x7F, // N 13
  0x3E, 0x41, 0x41, 0x41, 0x3E, // O 14
  0x7F, 0x09, 0x09, 0x09, 0x06, // P 15
  0x3E, 0x41, 0x51, 0x21, 0x5E, // Q 16
  0x7F, 0x09, 0x19, 0x29, 0x46, // R 17
  0x46, 0x49, 0x49, 0x49, 0x31, // S 18
  0x01, 0x01, 0x7F, 0x01, 0x01, // T 19
  0x3F, 0x40, 0x40, 0x40, 0x3F, // U 20
  0x1F, 0x20, 0x40, 0x20, 0x1F, // V 21
  0x3F, 0x40, 0x38, 0x40, 0x3F, // W 22
  0x63, 0x14, 0x08, 0x14, 0x63, // X 23
  0x07, 0x08, 0x70, 0x08, 0x07, // Y 24
  0x61, 0x51, 0x49, 0x45, 0x43, // Z 25
  0x1C, 0x3E, 0x7C, 0x3E, 0x1C, // ♥ 26
  0x20, 0x55, 0x54, 0x55, 0x78, // ä 27 
  0x38, 0x45, 0x44, 0x45, 0x38, // ö 28
  0x3C, 0x41, 0x40, 0x21, 0x7C, // ü 29
  0xFC, 0x02, 0x2A, 0x2A, 0x28, // ß 30
  0x00, 0x00, 0x00, 0x00, 0x00, //   31 Space - unused
  0x20, 0x54, 0x54, 0x54, 0x78, // a 32 
  0x7F, 0x48, 0x44, 0x44, 0x38, // b 33
  0x38, 0x44, 0x44, 0x44, 0x20, // c 34
  0x38, 0x44, 0x44, 0x48, 0x7F, // d 35
  0x38, 0x54, 0x54, 0x54, 0x18, // e 36
  0x08, 0x7E, 0x09, 0x01, 0x02, // f 37
  0x18, 0xA4, 0xA4, 0xA4, 0x7C, // g 38
  0x7F, 0x08, 0x04, 0x04, 0x78, // h 39
  0x00, 0x44, 0x7D, 0x40, 0x00, // i 40
  0x40, 0x80, 0x84, 0x7D, 0x00, // j 41
  0x7F, 0x10, 0x28, 0x44, 0x00, // k 42
  0x00, 0x41, 0x7F, 0x40, 0x00, // l 43
  0x7C, 0x04, 0x18, 0x04, 0x78, // m 44
  0x7C, 0x08, 0x04, 0x04, 0x78, // n 45
  0x38, 0x44, 0x44, 0x44, 0x38, // o 46
  0xFC, 0x24, 0x24, 0x24, 0x18, // p 47
  0x18, 0x24, 0x24, 0x18, 0xFC, // q 48
  0x7C, 0x08, 0x04, 0x04, 0x08, // r 49
  0x48, 0x54, 0x54, 0x54, 0x20, // s 50
  0x04, 0x3F, 0x44, 0x40, 0x20, // t 51
  0x3C, 0x40, 0x40, 0x20, 0x7C, // u 52
  0x1C, 0x20, 0x40, 0x20, 0x1C, // v 53
  0x3C, 0x40, 0x30, 0x40, 0x3C, // w 54
  0x44, 0x28, 0x10, 0x28, 0x44, // x 55
  0x1C, 0xA0, 0xA0, 0xA0, 0x7C, // y 56
  0x44, 0x64, 0x54, 0x4C, 0x44, // z 57
  0x7D, 0x12, 0x11, 0x12, 0x7D, // Ä 58
  0x3C, 0x43, 0x42, 0x43, 0x3C, // Ö 59
  0x3E, 0x41, 0x40, 0x41, 0x3E  // Ü 60
};
#endif

// Private functions
void Oled_commandMode() {               
  TinyI2C.start(SlaveAddress, 0);                // Begin I2C transmission
  TinyI2C.write(0x80);                           // Command mode
}

void Oled_dataMode() {
  TinyI2C.start(SlaveAddress, 0);                // Begin I2C transmission
  TinyI2C.write(0x40);                           // Data mode
}

void Oled_sendCommand(uint8_t command) { // Public function now to turn off display (old boards)
  Oled_commandMode();                            // Set command mode
  TinyI2C.write(command);                        // Send command
  TinyI2C.stop();    		                         // End I2C transmission
}

void Oled_sendData(uint8_t data) {
  Oled_dataMode();                               // Set data mode
  TinyI2C.write(data);                           // Send data
  TinyI2C.stop();                                // Stop I2C transmission
}

// Public functions
void Oled_init() {
  uint8_t i;
  _delay_ms(50);	                               // Wait for OLED hardware init
  Oled_commandMode();                            // Set command mode
  for (i = 0; i < InitLength; i++)              
    TinyI2C.write(pgm_read_byte(&InitSequence[i])); // Write init sequence from PROGMEM
  TinyI2C.stop();
}

void Oled_cursorTo(uint8_t col, uint8_t row) {
  Oled_sendCommand(0xb0 | row);
  Oled_sendCommand(0x00 | (col & 0xf));
  Oled_sendCommand(0x10 | ((col>>4)& 0xf));
}

void Oled_clear() {
  uint8_t p, x;
  for (p = 0; p < 8; p++) {
    Oled_sendCommand(0xb0 | p);                  // Page 0 - 7   
    Oled_sendCommand(0x00 | 0x00);               // Low col = 0
    Oled_sendCommand(0x10 | 0x00);               // Hi col = 0
    Oled_dataMode();
    for (x = 0; x <= 128; x++) TinyI2C.write(0x00); // Clear every column
    TinyI2C.stop();
  }
}

void Oled_printChar(char ch) { // Reworked for Schimpfolino
  uint8_t a;
  Oled_dataMode();                               // Set data mode
  for (a = 0; a < 5; a ++)                       // Write 5 columns for each character
    TinyI2C.write(pgm_read_byte(&BasicFont[ch * 5 + a])); // Write column from PROGMEM
  TinyI2C.write(0x00);                           // One column space for better readabiltiy
  if (chars < 19) TinyI2C.write(0x00);           // One more column space when the line has enough room
  TinyI2C.stop();
}
