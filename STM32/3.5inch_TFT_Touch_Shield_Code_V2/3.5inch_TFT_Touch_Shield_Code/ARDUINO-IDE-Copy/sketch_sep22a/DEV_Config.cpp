/******************************************************************************
**************************Hardware interface layer*****************************
* | file          :   DEV_Config.cpp
* | function      :   Provide the hardware underlying interface
* | note          :   Arduino Uno control test — see DEV_Config.h for context.
*                      Uses the AVR-global `SPI` object on the fixed hardware
*                      SPI pins (D13/D12/D11), same as the official, already
*                      proven Waveshare Arduino/LCD_Show/DEV_Config.cpp.
******************************************************************************/
#include "DEV_Config.h"

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

  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.begin();

  return 0;
}

void PWM_SetValue(uint16_t value)
{
  analogWrite(LCD_BL, value);
}

/********************************************************************************
  function:    Hardware interface
  note:
    SPI4W_Write_Byte(value) : hardware SPI
********************************************************************************/
void SPI4W_Write_Byte(uint8_t DATA)
{
  SPI.transfer(DATA);
}

uint8_t SPI4W_Read_Byte(uint8_t DATA)
{
  return SPI.transfer(DATA);
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
