/*****************************************************************************
* | File      	:	DEV_Config.h
* | Function    :	Provide the hardware underlying interface
*
* Адаптировано под STM32H723VGT6 / GFX-WaveShare (SPI2, пины из main.h,
* сгенерированные CubeMX). Логика самого LCD_Driver.c НЕ трогается — только
* эта прослойка "как именно дёрнуть пин/послать байт". См. журнал
* GFX-WaveShare-Workflow.md, п. 6.18.
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include "main.h"

//LCD
#define LCD_CS_0		HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_1		HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)

#define LCD_RST_0		HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_1		HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

#define LCD_DC_0		HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_1		HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

/*------------------------------------------------------------------------------------------------------*/
uint8_t System_Init(void);
void PWM_SetValue(uint16_t value);
void SPI4W_Write_Byte(uint8_t DATA);
uint8_t SPI4W_Read_Byte(uint8_t DATA);

void Driver_Delay_ms(uint32_t xms);
void Driver_Delay_us(uint32_t xus);

#endif
