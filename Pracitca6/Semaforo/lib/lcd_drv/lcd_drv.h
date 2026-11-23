#ifndef __LCD_DRV__
#define __LCD_DRV__

// i2c addresses
#define LCD_ADDR 0x7c //(0x3e<<1)
#define RGB_ADDR 0xc4 //(0x62<<1)

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET 0x20
 
// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
 
// flag for entry mode
#define LCD_ENTRYLEFT 0x02
 
// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_2LINE 0x08
#define LCD_5x10DOTS 0x04

void setRGB(unsigned char r, unsigned char g, unsigned char b);
void printLCD(char *str);
void locateLCD(unsigned char col, unsigned char row);
void clearLCD();
//void displayOn();
void initLCD();

#endif
