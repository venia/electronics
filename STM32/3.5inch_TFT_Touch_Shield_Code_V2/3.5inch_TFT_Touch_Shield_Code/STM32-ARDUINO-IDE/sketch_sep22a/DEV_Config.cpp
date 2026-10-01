/******************************************************************************
**************************Hardware interface layer*****************************
* | file          :   DEV_Config.cpp
* | function      :   Provide the hardware underlying interface
* | note          :   STM32duino port — see DEV_Config.h for context.
******************************************************************************/
#include "DEV_Config.h"

// Explicit SPI2 instance on the pins wired to the shield (matches the pin
// table already confirmed working for this board in the project Readme.md).
SPIClass SPI_2(TFT_MOSI, TFT_MISO, TFT_SCK);

/********************************************************************************
  function:    System Init
  note:
    Initialize the communication method
********************************************************************************/
uint8_t System_Init(void)
{
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  pinMode(LCD_BL, OUTPUT);

  Serial.begin(115200);

  SPI_2.begin();
  SPI_2.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  return 0;
}

// PC3/LCD_BL is wired as a plain GPIO_Output in this project's pin plan, not
// a confirmed PWM/timer pin, so backlight is driven as a hard on/off instead
// of analogWrite() dimming.
void PWM_SetValue(uint16_t value)
{
  digitalWrite(LCD_BL, value > 0 ? HIGH : LOW);
}

/********************************************************************************
  function:    Hardware interface
  note:
    SPI4W_Write_Byte(value) : hardware SPI
********************************************************************************/
void SPI4W_Write_Byte(uint8_t DATA)
{
  SPI_2.transfer(DATA);
}

uint8_t SPI4W_Read_Byte(uint8_t DATA)
{
  return SPI_2.transfer(DATA);
}

/********************************************************************************
  function:    Delay function
********************************************************************************/
void Driver_Delay_ms(unsigned long xms)
{
  delay(xms);
}

void Driver_Delay_us(int xus)
{
  delayMicroseconds(xus);
}
