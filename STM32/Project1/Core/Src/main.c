/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi4;

/* USER CODE BEGIN PV */

#define LCD_CS_LOW()   HAL_GPIO_WritePin(LCD_CS_Pin_GPIO_Port, LCD_CS_Pin_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()  HAL_GPIO_WritePin(LCD_CS_Pin_GPIO_Port, LCD_CS_Pin_Pin, GPIO_PIN_SET)
#define LCD_DC_CMD()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_DATA()  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_GREEN   0x07E0

static void LCD_WriteCmd(uint8_t cmd) {
    LCD_DC_CMD();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi4, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

static void LCD_WriteData(uint8_t *data, uint16_t len) {
    LCD_DC_DATA();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi4, data, len, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

static void LCD_WriteData8(uint8_t d) { LCD_WriteData(&d, 1); }

static void ST7735_Init(void) {
    uint8_t buf[16];

    LCD_WriteCmd(0x01); HAL_Delay(150);      // SWRESET
    LCD_WriteCmd(0x11); HAL_Delay(255);      // SLPOUT (выход из сна)

    buf[0]=0x01; buf[1]=0x2C; buf[2]=0x2D;
    LCD_WriteCmd(0xB1); LCD_WriteData(buf,3); // FRMCTR1
    LCD_WriteCmd(0xB2); LCD_WriteData(buf,3); // FRMCTR2

    buf[0]=0x01; buf[1]=0x2C; buf[2]=0x2D; buf[3]=0x01; buf[4]=0x2C; buf[5]=0x2D;
    LCD_WriteCmd(0xB3); LCD_WriteData(buf,6); // FRMCTR3

    LCD_WriteCmd(0xB4); LCD_WriteData8(0x07); // INVCTR

    buf[0]=0xA2; buf[1]=0x02; buf[2]=0x84;
    LCD_WriteCmd(0xC0); LCD_WriteData(buf,3); // PWCTR1
    LCD_WriteCmd(0xC1); LCD_WriteData8(0xC5); // PWCTR2
    buf[0]=0x0A; buf[1]=0x00;
    LCD_WriteCmd(0xC2); LCD_WriteData(buf,2); // PWCTR3
    buf[0]=0x8A; buf[1]=0x2A;
    LCD_WriteCmd(0xC3); LCD_WriteData(buf,2); // PWCTR4
    buf[0]=0x8A; buf[1]=0xEE;
    LCD_WriteCmd(0xC4); LCD_WriteData(buf,2); // PWCTR5

    LCD_WriteCmd(0xC5); LCD_WriteData8(0x0E); // VMCTR1
    LCD_WriteCmd(0x21);                        // INVON (инверсия цвета включена для этой матрицы)

    LCD_WriteCmd(0x36); LCD_WriteData8(0x78);  // MADCTL (ориентация экрана + порядок RGB)
    LCD_WriteCmd(0x3A); LCD_WriteData8(0x05);  // COLMOD - 16 бит/пиксель (RGB565)

    LCD_WriteCmd(0x13); HAL_Delay(10);         // NORON
    LCD_WriteCmd(0x29); HAL_Delay(100);        // DISPON - включить экран

    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET); // подсветка ON (активный low)
}

static void ST7735_SetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t buf[4];
    // экран физически 160x80, но с offset x+1, y+26 (особенность этой матрицы)
    buf[0]=0x00; buf[1]=x0+1; buf[2]=0x00; buf[3]=x1+1;
    LCD_WriteCmd(0x2A); LCD_WriteData(buf,4); // CASET

    buf[0]=0x00; buf[1]=y0+26; buf[2]=0x00; buf[3]=y1+26;
    LCD_WriteCmd(0x2B); LCD_WriteData(buf,4); // RASET

    LCD_WriteCmd(0x2C); // RAMWR - начинаем писать пиксели
}

static void ST7735_FillScreen(uint16_t color) {
    ST7735_SetAddrWindow(0, 0, 159, 79);
    uint8_t hi = color >> 8, lo = color & 0xFF;
    uint8_t line[160*2];
    for (int i = 0; i < 160; i++) { line[i*2]=hi; line[i*2+1]=lo; }
    LCD_DC_DATA();
    LCD_CS_LOW();
    for (int y = 0; y < 80; y++) HAL_SPI_Transmit(&hspi4, line, sizeof(line), HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

// компактный шрифт 5x7, нужные буквы для "HELLO"
static const uint8_t font5x7[5][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x3E,0x41,0x41,0x41,0x3E}, // O
};

static void ST7735_DrawChar(uint16_t x, uint16_t y, uint8_t idx, uint16_t color, uint16_t bg, uint8_t scale) {
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = font5x7[idx][col];
        for (uint8_t row = 0; row < 7; row++) {
            uint16_t c = (line & (1 << row)) ? color : bg;
            ST7735_SetAddrWindow(x + col*scale, y + row*scale,
                                  x + col*scale + scale - 1, y + row*scale + scale - 1);
            uint8_t hi = c >> 8, lo = c & 0xFF;
            uint8_t px[2] = {hi, lo};
            LCD_DC_DATA();
            LCD_CS_LOW();
            for (int p = 0; p < scale*scale; p++) HAL_SPI_Transmit(&hspi4, px, 2, HAL_MAX_DELAY);
            LCD_CS_HIGH();
        }
    }
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI4_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI4_Init();
  /* USER CODE BEGIN 2 */
  ST7735_Init();
  ST7735_FillScreen(COLOR_BLACK);

  // "HELLO" — индексы в font5x7: space=0,H=1,E=2,L=3,O=4
  uint8_t word[] = {1,2,3,3,4}; // H E L L O
  uint16_t x = 20, y = 30, scale = 2;
  for (int i = 0; i < 5; i++) {
      ST7735_DrawChar(x + i*(6*scale), y, word[i], COLOR_GREEN, COLOR_BLACK, scale);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 25;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 0x0;
  hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi4.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LCD_BL_Pin|LCD_CS_Pin_Pin|LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LCD_BL_Pin LCD_CS_Pin_Pin LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_BL_Pin|LCD_CS_Pin_Pin|LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
