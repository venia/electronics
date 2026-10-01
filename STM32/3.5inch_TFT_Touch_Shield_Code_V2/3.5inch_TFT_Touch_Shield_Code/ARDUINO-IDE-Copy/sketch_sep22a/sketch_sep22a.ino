// Arduino Uno control test (PRIZMA-DOSTOVERNOSTI.md §8, п.1) — copy of the
// STM32duino sketch_sep22a, DEV_Config.h/.cpp retargeted to a real Uno (see
// DEV_Config.h). LCD_Driver.cpp/.h are the original Waveshare register
// sequence, unmodified. STM32-ARDUINO-IDE/ is untouched.
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
