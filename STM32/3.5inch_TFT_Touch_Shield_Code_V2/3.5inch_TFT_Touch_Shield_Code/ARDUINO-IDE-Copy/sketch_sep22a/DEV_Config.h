/******************************************************************************
**************************Hardware interface layer*****************************
* | file          :   DEV_Config.h
* | function      :   Provide the hardware underlying interface
* | note          :   Arduino Uno control test (see project PRIZMA-DOSTOVERNOSTI.md
*                      §8, п.1). Same LCD_Driver.cpp/.h protocol as the STM32
*                      port in STM32-ARDUINO-IDE/ (untouched, do not edit), but
*                      this device layer targets a real Uno: hardware SPI on
*                      D13/D12/D11 and LCD control pins matching the official,
*                      already-proven Waveshare Arduino/LCD_Show/DEV_Config.h
*                      pin numbers, so the shield's loose header wires (same
*                      net as ICSP, confirmed §4.3) land on the right Uno pins.
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <Arduino.h>
#include <SPI.h>

//GPIO config
//LCD
#define LCD_CS 10
#define LCD_CS_0		digitalWrite(LCD_CS, LOW)
#define LCD_CS_1		digitalWrite(LCD_CS, HIGH)

#define LCD_BL 9

#define LCD_RST 8
#define LCD_RST_0		digitalWrite(LCD_RST, LOW)
#define LCD_RST_1		digitalWrite(LCD_RST, HIGH)

#define LCD_DC 7
#define LCD_DC_0		digitalWrite(LCD_DC, LOW)
#define LCD_DC_1		digitalWrite(LCD_DC, HIGH)

/*------------------------------------------------------------------------------------------------------*/
uint8_t System_Init(void);
void PWM_SetValue(uint16_t value);
void SPI4W_Write_Byte(uint8_t DATA);
uint8_t SPI4W_Read_Byte(uint8_t DATA);

void Driver_Delay_ms(unsigned long xms);
void Driver_Delay_us(int xus);

#endif
