#ifndef LCD_ILI9486_H
#define LCD_ILI9486_H

#include "main.h"

#define LCD_WIDTH  480
#define LCD_HEIGHT 320

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED   0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE  0x001F

void LCD_Init(void);
void LCD_FillScreen(uint16_t color);

#endif /* LCD_ILI9486_H */
