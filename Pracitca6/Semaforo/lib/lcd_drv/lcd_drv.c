#include "lcd_drv.h"
#include "stdint.h"
#include "stm32f4xx_hal.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

char _displayfunction;
char _displaycontrol;


void sendCommand(char value)
{
    uint8_t data[2] = {0x80, value};
    HAL_I2C_Master_Transmit(&hi2c1, 0x7C, (uint8_t *)data, 2, 0x100);
}


void setReg(char addr, char val)
{
    uint8_t data[2] = {addr, val};
    HAL_I2C_Master_Transmit(&hi2c1, 0x62<<1, (uint8_t *)data, 2, 0x100);
}



void setRGB(unsigned char r, unsigned char g, unsigned char b)
{
    setReg(0x04, r);
    setReg(0x03, g);
    setReg(0x02, b);   
}


void printLCD(char *str)
{ 
    HAL_Delay(100);
    int i = 0;
    while(i != strlen(str)){
        uint8_t data[2] = {0x40, str[i]};
        HAL_I2C_Master_Transmit(&hi2c1, 0x7C, (uint8_t *)data, 2, 0x100);
        i++;
        HAL_Delay(10);
    }  
}


void locateLCD(unsigned char col, unsigned char row)
{
    col = (row == 0 ? col | 0x80 : col | 0xc0);
    unsigned char data[2] = {0x80, col};
    HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)0x7C, data, 2, 0x100);
}


void displayOn() 
{
    _displaycontrol |= LCD_DISPLAYON;
    sendCommand(LCD_DISPLAYCONTROL | _displaycontrol);
}


void clearLCD()
{
    sendCommand(LCD_CLEARDISPLAY);        
    HAL_Delay(2);         
}


void initLCD() 
{   
    //Initialize displayfunction parameter for setting up LCD display
   _displayfunction |= LCD_2LINE;
   _displayfunction |= LCD_5x10DOTS;
 
   //Wait for more than 30 ms after power rises above 4.5V per the data sheet
    HAL_Delay(50);
 
 
    // Send first function set command. Wait longer that 39 us per the data sheet
    sendCommand(LCD_FUNCTIONSET | _displayfunction);
    HAL_Delay(1);  
    
    // turn the display on
    displayOn();
 
    // clear the display
    clearLCD();
    
    // Initialize backlight
    setReg(0, 0);
    setReg(1, 0);
    setReg(0x08, 0xAA);   
}
