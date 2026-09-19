#include "lcd_ili9486.h"

extern SPI_HandleTypeDef hspi2;

static void LCD_CS_Low(void)  { HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET); }
static void LCD_CS_High(void) { HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET); }

static void LCD_WriteReg(uint8_t reg)
{
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    LCD_CS_Low();
    HAL_SPI_Transmit(&hspi2, &reg, 1, HAL_MAX_DELAY);
    LCD_CS_High();
}

static void LCD_WriteData8(uint8_t data)
{
    /* ILI9486 on this shield expects every parameter byte padded to 16 bit */
    uint8_t buf[2] = { 0x00, data };
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    LCD_CS_Low();
    HAL_SPI_Transmit(&hspi2, buf, 2, HAL_MAX_DELAY);
    LCD_CS_High();
}

static void LCD_Reset(void)
{
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(500);
}

static void LCD_InitRegisters(void)
{
    LCD_WriteReg(0xF9); LCD_WriteData8(0x00); LCD_WriteData8(0x08);

    LCD_WriteReg(0xC0); LCD_WriteData8(0x19); LCD_WriteData8(0x1A);
    LCD_WriteReg(0xC1); LCD_WriteData8(0x45); LCD_WriteData8(0x00);
    LCD_WriteReg(0xC2); LCD_WriteData8(0x33);
    LCD_WriteReg(0xC5); LCD_WriteData8(0x00); LCD_WriteData8(0x28);

    LCD_WriteReg(0xB1); LCD_WriteData8(0xA0); LCD_WriteData8(0x11);
    LCD_WriteReg(0xB4); LCD_WriteData8(0x02);
    LCD_WriteReg(0xB6); LCD_WriteData8(0x00); LCD_WriteData8(0x62); LCD_WriteData8(0x3B);
    LCD_WriteReg(0xB7); LCD_WriteData8(0x07);

    LCD_WriteReg(0xE0);
    LCD_WriteData8(0x1F); LCD_WriteData8(0x25); LCD_WriteData8(0x22); LCD_WriteData8(0x0B);
    LCD_WriteData8(0x06); LCD_WriteData8(0x0A); LCD_WriteData8(0x4E); LCD_WriteData8(0xC6);
    LCD_WriteData8(0x39); LCD_WriteData8(0x00); LCD_WriteData8(0x00); LCD_WriteData8(0x00);
    LCD_WriteData8(0x00); LCD_WriteData8(0x00); LCD_WriteData8(0x00);

    LCD_WriteReg(0xE1);
    LCD_WriteData8(0x1F); LCD_WriteData8(0x3F); LCD_WriteData8(0x3F); LCD_WriteData8(0x0F);
    LCD_WriteData8(0x1F); LCD_WriteData8(0x0F); LCD_WriteData8(0x46); LCD_WriteData8(0x49);
    LCD_WriteData8(0x31); LCD_WriteData8(0x05); LCD_WriteData8(0x09); LCD_WriteData8(0x03);
    LCD_WriteData8(0x1C); LCD_WriteData8(0x1A); LCD_WriteData8(0x00);

    LCD_WriteReg(0xF1);
    LCD_WriteData8(0x36); LCD_WriteData8(0x04); LCD_WriteData8(0x00); LCD_WriteData8(0x3C);
    LCD_WriteData8(0x0F); LCD_WriteData8(0x0F); LCD_WriteData8(0xA4); LCD_WriteData8(0x02);

    LCD_WriteReg(0xF2);
    LCD_WriteData8(0x18); LCD_WriteData8(0xA3); LCD_WriteData8(0x12); LCD_WriteData8(0x02);
    LCD_WriteData8(0x32); LCD_WriteData8(0x12); LCD_WriteData8(0xFF); LCD_WriteData8(0x32);
    LCD_WriteData8(0x00);

    LCD_WriteReg(0xF4);
    LCD_WriteData8(0x40); LCD_WriteData8(0x00); LCD_WriteData8(0x08); LCD_WriteData8(0x91);
    LCD_WriteData8(0x04);

    LCD_WriteReg(0xF8); LCD_WriteData8(0x21); LCD_WriteData8(0x04);

    LCD_WriteReg(0x3A); LCD_WriteData8(0x55); /* 16bpp RGB565 */
    LCD_WriteReg(0x36); LCD_WriteData8(0x28); /* MADCTL: landscape, D2U_L2R scan */
}

void LCD_Init(void)
{
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET); /* deselect touch chip, shares SPI2 bus */
    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET); /* backlight on, active high */

    LCD_Reset();
    LCD_InitRegisters();
    HAL_Delay(200);

    LCD_WriteReg(0x11); /* sleep out */
    HAL_Delay(120);
    LCD_WriteReg(0x29); /* display on */
}

void LCD_ReadID(uint8_t id[3])
{
    uint8_t cmd = 0xD3;
    uint8_t tx[4] = { 0x00, 0x00, 0x00, 0x00 };
    uint8_t rx[4] = { 0x00, 0x00, 0x00, 0x00 };

    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET); /* command phase */
    LCD_CS_Low();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET); /* data phase, CS stays low */
    HAL_SPI_TransmitReceive(&hspi2, tx, rx, 4, HAL_MAX_DELAY);
    LCD_CS_High();

    /* rx[0] is a dummy byte per datasheet; ID bytes follow */
    id[0] = rx[1];
    id[1] = rx[2];
    id[2] = rx[3];
}

/* Диагностика: чтение координат XPT2046 (тач на той же линии MISO/PB14, что и экран).
 * Используется только чтобы проверить, работает ли приём по MISO вообще —
 * не финальный драйвер тача. */
void TP_ReadRaw(uint16_t *x, uint16_t *y)
{
    uint8_t tx[3];
    uint8_t rx[3];

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET); /* деселект экрана, шина общая */

    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
    tx[0] = 0xD0; tx[1] = 0x00; tx[2] = 0x00; /* control byte: X position, 12-bit, differential */
    HAL_SPI_TransmitReceive(&hspi2, tx, rx, 3, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);
    *x = ((uint16_t)rx[1] << 8 | rx[2]) >> 3;

    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
    tx[0] = 0x90; tx[1] = 0x00; tx[2] = 0x00; /* control byte: Y position */
    HAL_SPI_TransmitReceive(&hspi2, tx, rx, 3, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);
    *y = ((uint16_t)rx[1] << 8 | rx[2]) >> 3;
}

static void LCD_SetWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    LCD_WriteReg(0x2A);
    LCD_WriteData8(xs >> 8); LCD_WriteData8(xs & 0xFF);
    LCD_WriteData8((xe - 1) >> 8); LCD_WriteData8((xe - 1) & 0xFF);

    LCD_WriteReg(0x2B);
    LCD_WriteData8(ys >> 8); LCD_WriteData8(ys & 0xFF);
    LCD_WriteData8((ye - 1) >> 8); LCD_WriteData8((ye - 1) & 0xFF);

    LCD_WriteReg(0x2C);
}

void LCD_FillScreen(uint16_t color)
{
    /* Байт-за-байтом, как в обоих проверенных референсах (Arduino LCD_Write_AllData,
     * STM32F103 LCD_Write_AllData поверх SPI4W_Write_Byte) — CS держим низко на весь
     * кадр, но НЕ гоним все 960 байт строки одним HAL_SPI_Transmit(), в отличие от
     * прошлой версии. У обоих референсов между байтами есть небольшая программная
     * пауза (F103-версия явно ждёт флаг BSY и вычитывает SPI1->DR после каждого байта) —
     * у сдвиговых регистров/счётчика (74HC4094 x2 + 74HC4040, см. PRIZMA-DOSTOVERNOSTI.md)
     * такой зазор мог быть, наоборот, обязательным условием корректной защёлки, а не просто
     * следствием более медленного железа референсов. */
    const uint8_t hi = (uint8_t)(color >> 8);
    const uint8_t lo = (uint8_t)(color & 0xFF);
    const uint32_t total_pixels = (uint32_t)LCD_WIDTH * (uint32_t)LCD_HEIGHT;

    LCD_SetWindow(0, 0, LCD_WIDTH, LCD_HEIGHT);

    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    LCD_CS_Low();
    for (uint32_t i = 0; i < total_pixels; i++)
    {
        HAL_SPI_Transmit(&hspi2, (uint8_t *)&hi, 1, HAL_MAX_DELAY);
        HAL_SPI_Transmit(&hspi2, (uint8_t *)&lo, 1, HAL_MAX_DELAY);
    }
    LCD_CS_High();
}
