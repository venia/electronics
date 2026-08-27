/*****************************************************************************
* | File      	:	DEV_Config.c
* | Function    :	Provide the hardware underlying interface
*
* Адаптировано под STM32H723VGT6 / GFX-WaveShare: HAL_SPI_Transmit на hspi2
* вместо сырых регистров F103, backlight — простое GPIO вкл/выкл вместо PWM
* (TIM3 у нас не настроен, а прямое GPIO уже подтверждено рабочим на этой
* плате — см. журнал GFX-WaveShare-Workflow.md, разделы 6.11 и 6.18).
******************************************************************************/
#include "DEV_Config.h"

extern SPI_HandleTypeDef hspi2;

uint8_t System_Init(void)
{
    return 0;
}

void PWM_SetValue(uint16_t value)
{
    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, value > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void SPI4W_Write_Byte(uint8_t DATA)
{
    HAL_SPI_Transmit(&hspi2, &DATA, 1, HAL_MAX_DELAY);
}

uint8_t SPI4W_Read_Byte(uint8_t DATA)
{
    uint8_t rx = 0;
    HAL_SPI_TransmitReceive(&hspi2, &DATA, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

void Driver_Delay_ms(uint32_t xms)
{
    HAL_Delay(xms);
}

void Driver_Delay_us(uint32_t xus)
{
    volatile uint32_t j;
    for (j = xus * 10; j > 0; j--);
}
