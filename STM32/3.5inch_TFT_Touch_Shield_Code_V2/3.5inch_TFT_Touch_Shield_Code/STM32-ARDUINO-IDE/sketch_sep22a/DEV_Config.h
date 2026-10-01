/******************************************************************************
**************************Hardware interface layer*****************************
* | file          :   DEV_Config.h
* | function      :   Provide the hardware underlying interface
* | note          :   STM32duino (Arduino core for STM32) port for
*                      WeAct STM32H723VGT6 + Waveshare 3.5" TFT Touch Shield.
*                      Uses the SPI2 pins already wired for this board (see
*                      project Readme.md), driven through STM32duino's SPIClass
*                      instead of the AVR-only global `SPI` object used by the
*                      original Waveshare Arduino sketch.
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <Arduino.h>
#include <SPI.h>

// SPI2 pins (SPIClass constructor needs the PinName-style PB_x/PC_x names)
#define TFT_SCK   PB_13
#define TFT_MISO  PB_14
#define TFT_MOSI  PB_15

// LCD control pins
#define LCD_CS    PC_0
#define LCD_DC    PC_1
#define LCD_RST   PC_2
#define LCD_BL    PC_3

#define LCD_CS_0		digitalWrite(LCD_CS, LOW)
#define LCD_CS_1		digitalWrite(LCD_CS, HIGH)

#define LCD_RST_0		digitalWrite(LCD_RST, LOW)
#define LCD_RST_1		digitalWrite(LCD_RST, HIGH)

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
