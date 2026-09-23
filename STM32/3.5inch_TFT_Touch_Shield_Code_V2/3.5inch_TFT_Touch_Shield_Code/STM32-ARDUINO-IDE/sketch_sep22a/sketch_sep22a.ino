// STM32duino port of Waveshare's proven Arduino/LCD_Show driver for this
// exact ILI9486 shift-register shield — see DEV_Config.h/.cpp for the STM32
// SPI2 adaptation. LCD_Driver.cpp/.h are the original Waveshare register
// sequence, unmodified.
#include "DEV_Config.h"
#include "LCD_Driver.h"

void setup()
{
  System_Init();

  LCD_Init(SCAN_DIR_DFT, 200);

  // Cycle solid colors — simplest possible test: if any of the three shows
  // up cleanly across the whole panel, SPI2 + ILI9486 init are both correct.
}

void loop()
{
  LCD_Clear(0xF800); // red   (RGB565)
  delay(1000);
  LCD_Clear(0x07E0); // green
  delay(1000);
  LCD_Clear(0x001F); // blue
  delay(1000);
}
