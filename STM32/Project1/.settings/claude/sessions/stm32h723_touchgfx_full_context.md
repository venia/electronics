# Полный технический журнал проекта: STM32H723VGT6 + TouchGFX

Этот документ — полная хронология и техническая документация проекта, собранная для передачи контекста в новую сессию (Claude Code или любую другую). Пишется человеком, который не новичок в программировании (Java, веб), но новичок именно в embedded/STM32.

---

## ЧАСТЬ 1. ЖЕЛЕЗО

### 1.1 Плата

**WeAct Studio Mini STM32H723VGT6** — компактная core-плата.

Характеристики чипа:
- ARM Cortex-M7, до 550 МГц
- Flash: 1024 КБ (внутренняя)
- SRAM: 564 КБ (разбита по доменам: AXI SRAM 192KB, DTCM 128KB, ITCM 64KB, плюс части в D2/D3 доменах)
- FPU (double precision) + DSP-инструкции + I/D-cache (16KB каждый)
- Корпус LQFP100, 82 физических GPIO
- 2× 8MB NOR Flash (SPI1 обычный + QSPI отдельный, оба W25Q64)
- MicroSD слот (SDMMC1)
- Разъём камеры (DCMI, FPC 24-pin, поддержка OV7670/OV2640/OV5640)
- USB-C (USB OTG HS, для питания и/или DFU-прошивки)
- Встроенный 0.96" TFT-дисплей ST7735, 160×80

### 1.2 Питание платы

- USB-C VBUS = 5В → через диод Шоттки (падение ~0.3-0.45В) → выведен на пин "5V" (реальное напряжение ~4.6-4.7В)
- Внутренний DC-DC (XT3410AFMR-G / TLV62569 / SY8088AAC, конфигурируемый резисторным делителем) понижает до 3.3В для чипа
- Даталист питания: "3.3-5.5V input, DC-DC output 1A max"
- ⚠️ У WeAct плат бывают несовпадения между шёлкографией и реальной схемой на пинах (известный баг у платы-сестры STM32F103) — перед подключением внешних модулей стоит проверять мультиметром

### 1.3 Разводка встроенного дисплея (ST7735, SPI4)

Из официальной схемы (`06-TFT-LCD.SchDoc`), подтверждено:

| Сигнал | Нога STM32 | Назначение |
|---|---|---|
| LCD_LEDA (подсветка) | PE10 | активный LOW (0=горит) |
| LCD_CS | PE11 | Chip Select |
| LCD_SCL (SCK) | PE12 | SPI clock |
| LCD_WR_RS (DC) | PE13 | команда(0)/данные(1) |
| LCD_SDA (MOSI) | PE14 | данные к экрану |
| LCD_RESET | — | нет отдельного пина, сброс программный (SWRESET команда 0x01) |

Матрица: 0.96", 80×160 (физическое разрешение), driver-подобный ST7735R.
Особенность адресации: offset **x+1, y+26** при задании CASET/RASET окна.
MADCTL = **0x78** (ориентация+RGB порядок для этой конкретной партии матриц — подбирался экспериментально; если экран "вверх ногами" — пробовать 0xC8, 0x08, 0xA8, 0x68, 0xF8).
COLMOD = **0x05** (RGB565, 16 бит/пиксель).
INVON включен (инверсия цвета для этой матрицы).

### 1.4 Прочая занятая периферия платы (из схемы, для справки — что НЕ трогать)

| Периферия | Пины |
|---|---|
| SPI-флешка (обычная, W25Q64) | PB3(CLK), PB4(MISO), PD6(CS через SB3), PD7(MOSI) |
| QSPI-флешка (вторая, W25Q64) | PB2(CLK), PB6(NCS), PD11(IO0), PD12(IO1), PE2(IO2), PD13(IO3) |
| MicroSD (SDMMC1) | PC8,PC9,PC10,PC11(данные), PC12(CK), PD2(CMD), PD4(детект карты через SB2) |
| Камера (DCMI) | PA4,PA6,PA7,PA8,PB7,PC6,PC7,PD3,PE0,PE1,PE4,PE5,PE6 + I2C1 на PB8/PB9 |
| Кварцы | PH0,PH1 (25МГц HSE), PC14,PC15 (32.768кГц LSE) |
| USB | PA11(DN),PA12(DP) |
| SWD | PA13(SWDIO),PA14(SWCLK) |
| Синий LED | PE3 |
| Кнопка K1 (пользовательская) | PC13 |
| Кнопка B0 (BOOT0) | системная, режим загрузки |
| Кнопка NR (сброс) | NRST |

### 1.5 Свободные пины (подтверждено на внешних разъёмах платы)

PA0, PA1, PA2, PA3, PA5, PA9, PA10, PA15, PB0, PB1, PB5, PB10, PB11, PB14, PB15, PC0, PC1, PC2, PC3, PC4, PC5, PD0, PD1, PD5, PD8, PD9, PD10, PD14, PD15, PE7, PE8, PE9, PE15.

Важная деталь по подписям: на шёлкографии платы пины подписаны **без буквы "P"** — то есть "B14" на плате = "PB14" в коде/CubeMX/datasheet. Это просто экономия места, не ошибка.

---

## ЧАСТЬ 2. ИНСТРУМЕНТЫ И ИХ УСТАНОВКА

### 2.1 Состав

- **STM32CubeIDE** 2.2.0 — основная среда разработки (Eclipse-based)
- **STM32CubeMX** — в версии CubeIDE 2.2.0 (v2.0+) графический конфигуратор пинов/тактирования/периферии **убран из самой IDE**, работает только как отдельное standalone-приложение. Проект создаётся в CubeMX, затем импортируется в CubeIDE (`File → Import → General → Existing Projects into Workspace`).
- **X-CUBE-TOUCHGFX** 4.26.1 — устанавливается через CubeMX `Software Packs → Manage Software Packs → STMicroelectronics → X-CUBE-TOUCHGFX`
- **TouchGFX Designer** 4.26.1 — визуальный редактор GUI. НЕ ставится автоматически как отдельная программа через CubeMX — установщик (`TouchGFX-4.26.1.msi`) лежит внутри скачанного пакета, путь примерно: `Downloads/STMicroelectronics.X-CUBE-TOUCHGFX.4.26.1/Utilities/PC_Software/TouchGFXDesigner/TouchGFX-4.26.1.msi`
- **STM32CubeProgrammer** — для прошивки через DFU

### 2.2 Известная проблема при установке пакетов через CubeMX

При скачивании крупных пакетов (например crdb.zip — clock resource database) часто вылезает ошибка "Problem during download and/or unzip". Это **известный, подтверждённый баг на стороне серверов ST** (не у пользователя) — официальное решение от поддержки ST: просто нажать OK на ошибке и продолжить работу (некритичный файл), либо `Help → Connection and Updates → No Auto-Refresh at Application start`, чтобы ошибка не всплывала каждый раз.

### 2.3 Как открыть проект правильно

1. CubeMX: `File → New Project → MCU Selector → STM32H723VGT6 → Start Project`
2. Настроить Pinout/Clock/Peripherals (см. Часть 3)
3. `Project Manager` → указать имя, папку, **Toolchain/IDE = STM32CubeIDE**
4. `GENERATE CODE`
5. В CubeIDE: `File → Import → General → Existing Projects into Workspace` → указать папку проекта

### 2.4 Процедура прошивки через DFU (используется постоянно на протяжении всего проекта)

1. На плате: зажать **B0**, тапнуть **NR** (сброс), подождать ~0.5 сек, отпустить **B0** — плата в режиме USB DFU-бутлоадера
2. STM32CubeProgrammer → Interface: **USB** → Refresh → Connect
3. Erasing & Programming → Browse → указать `.bin` файл (лежит в `Debug/` папке проекта)
4. Start address: `0x08000000`
5. Обязательно галочка **"Run after programming"** — без неё плата остаётся висеть в DFU после записи, и на экране будет видна СТАРАЯ прошивка, пока не нажать NR вручную
6. Start Programming

---

## ЧАСТЬ 3. НАСТРОЙКА ПЕРИФЕРИИ В CUBEMX (пошагово, как было сделано)

### 3.1 RCC / Clock Configuration
- High Speed Clock (HSE) = Crystal/Ceramic Resonator (плата имеет кварц 25 МГц)
- На вкладке Clock Configuration: ввести целевую частоту (400 МГц выбрано для старта, не разгонять сразу до максимума 550 — там нужна отдельная настройка Vcore Overdrive)
- При ошибке "No solution found using current selected sources" → нажать OK, CubeMX сам переключит источник PLL с HSI на HSE
- Итоговый результат: SYSCLK=400МГц, CPU Clocks=400МГц, AXI/HCLK3=200МГц, APB-шины 100-200МГц — все в пределах лимитов (550MHz max / 275MHz max подсказки рядом с полями)

### 3.2 SPI4 (для встроенного дисплея)
- Pinout & Configuration → Connectivity → SPI4
- Mode: **Transmit Only Master** (для дисплея без обратного чтения)
- ⚠️ CubeMX по умолчанию может выбрать пины PE2/PE5/PE6 вместо нужных PE12/PE14 — нужно вручную кликнуть на PE12 на картинке чипа → выбрать SPI4_SCK из выпадающего списка (там реально есть, просто список большой: COMP1_OUT, DFSDM1_DATIN5, FMC_D9, FMC_DA9, LTDC_B4, SAI4_SCK_B, **SPI4_SCK**, TIM1_CH3N...), аналогично PE14 → SPI4_MOSI
- Снятие пина с SPI4 (если ошибся) делается через выбор "GPIO_Input"/"Reset_State" на этом пине, затем Mode периферии автоматически становится Disable, нужно заново выбрать Transmit Only Master
- Data Size: **8 Bits** (по умолчанию стоит 4, обязательно поправить)
- Prescaler: подобран **16** → Baud Rate = 6.25 МБит/с (надёжная скорость для первого теста; исходная скорость 50МБит/с при Prescaler=2 слишком высокая для ST7735)
- CPOL=Low, CPHA=1 Edge

### 3.3 GPIO для CS/DC/подсветки
- PE11 → GPIO_Output → переименовать label в **LCD_CS** (внимание: если ввести "LCD_CS_Pin" вместо "LCD_CS", CubeMX добавит суффикс сам и получится `LCD_CS_Pin_Pin`/`LCD_CS_Pin_GPIO_Port` — путаница, отсюда была ошибка компиляции. Итоговое реальное имя в проекте оказалось `LCD_CS_Pin_Pin`/`LCD_CS_Pin_GPIO_Port` — проверять в `main.h` перед использованием в коде!)
- PE13 → GPIO_Output → label **LCD_DC**
- PE10 → GPIO_Output → label **LCD_BL**
- GPIO output level: High по умолчанию, Mode: Output Push Pull

### 3.4 CRC (обязательная зависимость TouchGFX)
- Расположение в дереве: **Computing → CRC** (НЕ в System Core, как можно было бы подумать)
- Просто поставить галочку **Activated**, без дополнительных параметров
- Без этого TouchGFX Generator показывает красную ошибку "Please Enable CRC IP" в разделе Dependencies

### 3.5 TouchGFX (Software Packs → X-CUBE-TOUCHGFX)
- Mode: галочка **Graphics Application**
- Display → Interface: **Custom**
- Display → Framebuffer Pixel Format: **RGB565**
- Display → Width: **160**, Height: **80**
- Display → Use Larger Framebuffer Stride: No
- Display → Framebuffer Strategy: **Single Buffer** (для маленького экрана хватает; для будущего большого экрана планируется Partial Framebuffer из-за ограничения RAM)
- Display → Buffer Location: By Allocation
- Driver → Application Tick Source: **Custom**
- Driver → Use DMA2D Accelerator (ChromART): **No** (не работает эффективно с Custom-интерфейсом без LTDC; для маленького экрана и не нужен — хотя DMA2D физически присутствует на STM32H723/733, подтверждено по AN5020)
- Driver → Real-Time Operating System: **No OS**
- Additional Features → External Data Reader: Disabled
- Additional Features → Vector Rendering: Disabled
- Video Decoding → Type: Disabled

### 3.6 Генерация и первый запуск Designer
- Project Manager → Generate Code
- В дереве проекта появляется `TouchGFX/App/ApplicationTemplate.touchgfx.part` — это НЕ обычный файл, двойной клик в CubeIDE открывает его как текст (JSON), а не как GUI-редактор
- Правильный способ открыть Designer: правой кнопкой на файле → Open With → должен появиться пункт "TouchGFX Designer" ЕСЛИ программа установлена как отдельное приложение (см. 2.1). Если пункта нет — программа не установлена, искать `.msi` внутри скачанного пакета вручную.
- После установки Designer: открыть его через Пуск → File → Open → указать путь к `ApplicationTemplate.touchgfx.part`

---

## ЧАСТЬ 4. РУЧНОЙ КОД ST7735 (ПЕРВАЯ ИТЕРАЦИЯ, ДО TOUCHGFX)

Этот код писался как учебное упражнение — понять, как работает SPI/HAL "с нуля", без фреймворков. Часть функций сохранена и используется позже TouchGFX-интеграцией.

### 4.1 Код в `main.c` (секция USER CODE BEGIN PV)

```c
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

// ВАЖНО: без static — вызывается из C++ (TouchGFXHAL.cpp)
void LCD_WriteData(uint8_t *data, uint16_t len) {
    LCD_DC_DATA();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi4, data, len, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

static void LCD_WriteData8(uint8_t d) { LCD_WriteData(&d, 1); }

static void ST7735_Init(void) {
    uint8_t buf[16];

    LCD_WriteCmd(0x01); HAL_Delay(150);      // SWRESET
    LCD_WriteCmd(0x11); HAL_Delay(255);      // SLPOUT

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
    LCD_WriteCmd(0x21);                        // INVON

    LCD_WriteCmd(0x36); LCD_WriteData8(0x78);  // MADCTL
    LCD_WriteCmd(0x3A); LCD_WriteData8(0x05);  // COLMOD RGB565

    LCD_WriteCmd(0x13); HAL_Delay(10);         // NORON
    LCD_WriteCmd(0x29); HAL_Delay(100);        // DISPON

    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET); // подсветка ON (active LOW)
}

// ВАЖНО: без static — вызывается из C++ (TouchGFXHAL.cpp)
void ST7735_SetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    uint8_t buf[4];
    buf[0]=0x00; buf[1]=x0+1; buf[2]=0x00; buf[3]=x1+1;
    LCD_WriteCmd(0x2A); LCD_WriteData(buf,4); // CASET

    buf[0]=0x00; buf[1]=y0+26; buf[2]=0x00; buf[3]=y1+26;
    LCD_WriteCmd(0x2B); LCD_WriteData(buf,4); // RASET

    LCD_WriteCmd(0x2C); // RAMWR
}
```

### 4.2 Что было удалено при переходе на TouchGFX
Раньше существовали (теперь удалены, заменены логикой TouchGFX):
- `ST7735_FillScreen(uint16_t color)` — заливка всего экрана одним цветом
- `font5x7[][5]` — самодельный битовый шрифт 5×7 для нескольких букв (H,E,L,O + space), координаты хранились по столбцам, бит = строка
- `ST7735_DrawChar(...)` — ручная отрисовка одного символа побитово через множество вызовов SetAddrWindow на каждый пиксель

### 4.3 Вызов в main() (актуальная версия)
```c
/* USER CODE BEGIN 2 */
ST7735_Init();
/* USER CODE END 2 */
```
(TouchGFX сам берёт на себя всю отрисовку дальше, через `MX_TouchGFX_Init()` и `MX_TouchGFX_Process()` в главном цикле — эти вызовы генерируются автоматически)

---

## ЧАСТЬ 5. ИНТЕГРАЦИЯ TOUCHGFX С РУЧНЫМ ДИСПЛЕЕМ

### 5.1 Почему нужна ручная связка
CubeMX Display Interface = Custom означает: TouchGFX ничего не знает про физический ST7735/SPI. Он рисует всё в **RAM framebuffer**, а "вытолкнуть" это на реальный экран нужно дописать самостоятельно.

### 5.2 Файл `TouchGFX/target/TouchGFXHAL.cpp` — что дописано

```cpp
#include <TouchGFXHAL.hpp>
#include <MyButtonController.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */

extern "C" {
    void LCD_WriteData(uint8_t *data, uint16_t len);
    void ST7735_SetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
}

using namespace touchgfx;

void TouchGFXHAL::initialize()
{
    TouchGFXGeneratedHAL::initialize();

    static MyButtonController myButtonController;
    getInstance()->setButtonController(&myButtonController);
}

uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    return TouchGFXGeneratedHAL::getTFTFrameBuffer();
}

void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    TouchGFXGeneratedHAL::setTFTFrameBuffer(address);
}

void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    const uint16_t screenWidth = 160;
    uint16_t* fb = getTFTFrameBuffer();

    ST7735_SetAddrWindow(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);

    static uint8_t lineBuf[160 * 2];
    for (int row = 0; row < rect.height; row++)
    {
        uint16_t* srcRow = fb + (rect.y + row) * screenWidth + rect.x;
        for (int col = 0; col < rect.width; col++)
        {
            uint16_t px = srcRow[col];
            lineBuf[col * 2]     = (uint8_t)(px >> 8);
            lineBuf[col * 2 + 1] = (uint8_t)(px & 0xFF);
        }
        LCD_WriteData(lineBuf, rect.width * 2);
    }

    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

// остальные методы (blockCopy, configureInterrupts, enableInterrupts,
// disableInterrupts, enableLCDControllerInterrupt, beginFrame, endFrame)
// оставлены как сгенерированы — просто вызывают TouchGFXGeneratedHAL::

/* USER CODE END TouchGFXHAL.cpp */
```

### 5.3 Файл `TouchGFX/target/generated/TouchGFXGeneratedHAL.cpp` (авто-сгенерированный, НЕ редактировать, но важен для понимания)

```cpp
namespace
{
LOCATION_PRAGMA_NOLOAD("TouchGFX_Framebuffer")
uint32_t frameBuf[(160 * 80 * 2 + 3) / 4] LOCATION_ATTRIBUTE_NOLOAD("TouchGFX_Framebuffer");
}

void TouchGFXGeneratedHAL::initialize()
{
    HAL::initialize();
    registerEventListener(*(Application::getInstance()));
    enableLCDControllerInterrupt();
    enableInterrupts();
    setFrameBufferStartAddresses((void*)frameBuf, (void*)0, (void*)0);
}

uint16_t* TouchGFXGeneratedHAL::getTFTFrameBuffer() const
{
    return (uint16_t*)frameBuf;
}
```

Важно: framebuffer лежит в специальной **NOLOAD**-секции с именем `TouchGFX_Framebuffer` — это требует соответствующей записи в linker script (см. Часть 6).

### 5.4 Собственный ButtonController — `Core/Inc/MyButtonController.hpp`

```cpp
#ifndef MYBUTTONCONTROLLER_HPP_
#define MYBUTTONCONTROLLER_HPP_

#include <platform/driver/button/ButtonController.hpp>

class MyButtonController : public touchgfx::ButtonController
{
public:
    virtual void init();
    virtual bool sample(uint8_t& key);
private:
    uint8_t previousState;
};

#endif
```

### 5.5 `Core/Src/MyButtonController.cpp`

```cpp
#include <MyButtonController.hpp>
#include <main.h>
#include <touchgfx/hal/HAL.hpp>

void MyButtonController::init()
{
    previousState = 0x00;
}

bool MyButtonController::sample(uint8_t& key)
{
    if ((HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) && previousState == 0x00)
    {
        previousState = 0xFF;
        key = 0;
        return true;
    }
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET)
    {
        previousState = 0x00;
    }
    return false;
}
```

Зачем нужен: TouchGFX Designer позволяет настроить Interaction с триггером "Hardware button is clicked", но САМ механизм чтения физической кнопки не генерируется автоматически (в отличие от готовых демо-плат ST) — нужно писать свой класс-мост между железом и абстрактным "номером кнопки" в TouchGFX.

### 5.6 Важный факт про Interactions в Designer

- Trigger "X is clicked" появляется в списке **только для настоящих виджетов-кнопок** (Button, Button With Label, Toggle Button и т.д.) — они регистрируют этот триггер автоматически
- **Box + Mixin ClickListener НЕ дают такой триггер** — ClickListener это низкоуровневый C++ callback, не подключённый к визуальной системе Interactions
- На экране без тача (как наш маленький ST7735) это не проблема — используем Trigger "Hardware button is clicked" вместо клика по виджету
- В симуляторе (на компьютере, в самом Designer) "Hardware button" эмулируется клавишами клавиатуры, но по **ASCII-коду**, не по логике "0=клавиша 0". Если в поле "Choose button key" стоит значение 0 — это ASCII-код 0 (невидимый управляющий символ), а не клавиша "0" на клавиатуре (у неё ASCII 48). Для теста в симуляторе нужно либо вписать правильный ASCII-код нужной клавиши, либо просто тестировать на реальном железе (что и было сделано в итоге)

---

## ЧАСТЬ 6. ПРОБЛЕМА С LINKER SCRIPT (РЕШЕНА)

### 6.1 Симптом
После первой успешной генерации TouchGFX-проекта, сборка (`Build Project`) выдавала:
```
Error: Cannot find the specified linker script. Check the linker settings in the build configuration.
make[1]: *** [makefile:140: fail-specified-linker-script-missing] Error 2
```

### 6.2 Причина
Ссылка на `.ld`-файл в настройках проекта сбилась при первой генерации TouchGFX-кода через CubeMX (частая ситуация, судя по форумам ST).

### 6.3 Решение (сработало)
Повторная генерация кода из CubeMX (`GENERATE CODE` ещё раз, без изменения настроек) — восстановила корректную ссылку. После этого линковка прошла успешно.

### 6.4 Вторая проблема — секция framebuffer не была создана

После успешной сборки экран показывал не интерфейс, а **шум/"снег"**. Причина: TouchGFX Generator должен был автоматически дописать в `.ld`-файл секцию для размещения `TouchGFX_Framebuffer`, но НЕ сделал этого (потерялось при первой генерации/восстановлении linker script).

**Решение**: в файле `STM32H723VGTX_FLASH.ld`, в блоке `SECTIONS`, сразу после `.bss`, добавлено вручную:

```
.TouchGFX_Framebuffer (NOLOAD) :
{
  . = ALIGN(4);
  *(.TouchGFX_Framebuffer)
} >RAM_D1
```

(RAM_D1 выбран, т.к. в MEMORY-блоке этого файла: `RAM_D1 (xrw) : ORIGIN = 0x24000000, LENGTH = 320K` — с большим запасом относительно нужных 25КБ framebuffer)

### 6.5 Важный урок про пересборку

После правки `.ld`-файла **обычный** `Build Project` (incremental build) может НЕ подхватить изменение — линкер не перезапускается, если исходный код (.c/.cpp) не менялся. Обязательно нужно: `Project → Clean...` → затем `Project → Build Project` (полная пересборка). Проверка: в логе полной пересборки должна быть строка вида `arm-none-eabi-g++ -o "Project1.elf" @"objects.list" ...` (линковка) — если её нет, значит пересборка была неполной/из кэша.

### 6.6 Подтверждение через .map файл

`Project1.map` (генерируется автоматически при линковке) содержит честную карту памяти. Поиск по `TouchGFX_Framebuffer` в этом файле после исправления показал:
```
TouchGFX_Framebuffer   0x24000078   0x6400   load address 0x080140a8
./TouchGFX/target/generated/TouchGFXGeneratedHAL.o
```
`0x6400` в hex = 25600 байт = ровно 160×80×2 — секция создана правильно, память реально выделена по корректному адресу в RAM_D1.

**Важно**: команда `arm-none-eabi-size` при этом продолжает показывать неизменный `.bss` — это нормально, NOLOAD-секции не всегда попадают в этот подсчёт, это не признак ошибки.

---

## ЧАСТЬ 7. ПРОБЛЕМА СО "СНЕГОМ" НА ЭКРАНЕ (РЕШЕНА, см. Часть 7bis)

### 7.1 Симптом
После прошивки (успешной, verified, "Run after programming" включено, плата реально перезапустилась и выполняет новый код) — на экране **шум/"снег"**: хаотичные разноцветные пиксели по всей площади 160×80. НЕ старая прошивка (это исключено — старый статичный "HELLO" пропал). НЕ чёрный экран (что было бы, если SPI/инициализация вообще не работали).

### 7.2 Что уже проверено и подтверждено рабочим
- ✅ SPI4 физически передаёт данные (иначе не было бы "шума", а был бы либо чёрный экран, либо повторяющийся паттерн)
- ✅ ST7735_Init() выполняется (подсветка горит, экран "живой")
- ✅ Framebuffer выделен в правильном месте RAM (подтверждено в .map, адрес 0x24000078, размер 0x6400 в RAM_D1)
- ✅ Сборка полностью чистая, 0 errors 0 warnings
- ✅ flushFrameBuffer() код синтаксически корректен, компилируется без ошибок

### 7.3 Гипотезы, ещё не проверенные до конца
1. **Данные во framebuffer в момент вызова flushFrameBuffer — не то, что нарисовал TouchGFX**. Возможно, чтение происходит до завершения фактической отрисовки (race condition), или указатель `fb` в конкретный момент вызова указывает не туда
2. **D-Cache (Cortex-M7) не инвалидирован**. Framebuffer в RAM_D1 может быть в кэшируемой области. Если CPU пишет туда через кэш, а мы читаем "мимо кэша" (или наоборот) — можем видеть неконсистентные/устаревшие данные. В `TouchGFXGeneratedHAL.cpp` есть функции `InvalidateCache()`/`FlushCache()`, использующие `SCB_CleanInvalidateDCache()` — нужно проверить, вызываются ли они в нужный момент относительно нашего ручного `flushFrameBuffer`, и включена ли настройка "CPU Cache" для Cortex-M7 в CubeMX (System Core → CORTEX_M7)
3. **Несовпадение порядка/формата байт**. RGB565 записывается в framebuffer как обычный `uint16_t` (порядок байт зависит от endianness ARM — Cortex-M обычно little-endian). Наш код явно разбивает на hi/lo (`px >> 8` и `px & 0xFF`) в правильном порядке для SPI-дисплея (MSB первый), но стоит перепроверить, не является ли сам `px`, прочитанный из framebuffer, уже "перепутанным" из-за того, как TouchGFX сам пишет пиксели туда (LCD16bpp формат должен совпадать)
4. **Проблема синхронизации beginFrame/endFrame**. `TouchGFXGeneratedHAL::endFrame()` вызывает `HAL::endFrame()` и `OSWrappers::signalRenderingDone()` — без RTOS (у нас No OS) эта синхронизация работает иначе, возможно наш `flushFrameBuffer` вызывается в неправильной последовательности относительно фактического заполнения буфера

### 7.4 Рекомендованный план диагностики для новой сессии
1. Временно упростить: в `flushFrameBuffer` перед реальной логикой сделать `HAL_Delay(500)` и залить экран ОДНИМ фиксированным цветом напрямую через `ST7735_SetAddrWindow`+`LCD_WriteData` (не читая framebuffer вообще) — убедиться, что базовая связка "флаг вызван → рисование происходит" работает предсказуемо
2. Затем: прочитать ОДНО известное значение из `fb[0]` сразу после вызова `getTFTFrameBuffer()` и вывести его на экран каким-то заметным способом (например, если значение "похоже на осмысленный цвет UI" типа 0x0000 черный или конкретный зеленый из палитры Hello — значит буфер содержит правильные данные, и проблема в способе чтения/отправки, не в буфере)
3. Проверить настройку CPU Cache: `System Core → CORTEX_M7` в CubeMX — включена ли, и стоит ли принудительно вызвать `SCB_CleanInvalidateDCache()` перед началом чтения `fb` в `flushFrameBuffer`
4. Проверить реальный порядок вызовов: добавить временные GPIO-toggle или UART-вывод (если настроен) в `beginFrame`/`endFrame`/`flushFrameBuffer`, чтобы увидеть фактическую последовательность вызовов при работе

---

## ЧАСТЬ 7bis. РЕШЕНИЕ "СНЕГА" — ПОЛНЫЙ РАЗБОР ДЛЯ КОНСПЕКТА

Этот раздел написан детально и педагогично — специально для переноса в тетрадь и изучения. Разбирает: что мы проверяли, что каждая проверка показала, в чём была настоящая причина, и что конкретно изменено в коде (с объяснением "зачем" на каждый кусок).

### 7bis.1 Общая стратегия диагностики

Когда неизвестно, где именно в цепочке "framebuffer → HAL → SPI → дисплей" находится баг, самый надёжный подход — **делить цепочку пополам и проверять по кускам**, а не гадать сразу про самую сложную причину (кэш, DMA и т.п.). Мы шли от простого к сложному:

1. Сначала проверили САМЫЙ нижний уровень (SPI + дисплей), полностью убрав TouchGFX из уравнения.
2. Потом проверили, вызывается ли верхний уровень (framework) вообще.
3. Только когда стало ясно, ЧТО именно не работает — начали чинить.

Это универсальный приём при отладке embedded-систем: **не чини то, что не доказано сломанным**.

### 7bis.2 Диагностика — шаг за шагом

**Шаг 1. Гипотеза: может, дело в D-Cache?**
Проверили `Core/Src/main.c` — вызовов `SCB_EnableICache()`/`SCB_EnableDCache()` НЕТ. Кэш физически выключен. Значит все гипотезы про "кэш хранит старую копию framebuffer" отпадают сразу — там просто нечему рассинхронизироваться. Это заняло 1 grep-запрос и сразу отсекло целую ветку гипотез.

**Шаг 2. Тест: залить `rect` от TouchGFX одним цветом, не читая framebuffer.**
Временно переписали `TouchGFXHAL::flushFrameBuffer()` в файле `TouchGFX/target/TouchGFXHAL.cpp` так, чтобы она игнорировала содержимое framebuffer и просто слала зелёный цвет через уже существующие `ST7735_SetAddrWindow()` + `LCD_WriteData()`. Код, который временно вставили (потом полностью убрали):
```cpp
void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    ST7735_SetAddrWindow(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);

    static uint8_t lineBuf[160 * 2];
    for (int col = 0; col < rect.width; col++)
    {
        lineBuf[col * 2]     = 0x07; // RGB565 зелёный, старший байт
        lineBuf[col * 2 + 1] = 0xE0; // младший байт
    }
    for (int row = 0; row < rect.height; row++)
    {
        LCD_WriteData(lineBuf, rect.width * 2);
    }

    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}
```
Логика: `rect` — это прямоугольник, который framework передаёт нам и говорит "вот эту область экрана нужно перерисовать". Мы игнорируем реальные пиксели (не трогаем `fb`/`getTFTFrameBuffer()`), просто заполняем буфер строки одним и тем же цветом и шлём его `rect.height` раз.
- **Результат: всё равно снег.**
- Вывод на тот момент (оказался неполным, см. Шаг 3): либо дело не в framebuffer, либо `flushFrameBuffer` в принципе вызывается с "мусорными" координатами `rect`, из-за чего сам тест не является чистым.

**Шаг 3. Более прямой тест: залить экран ДО старта TouchGFX, прямо в `main()`.**
Вставили в `Core/Src/main.c`, прямо после `ST7735_Init()` (то есть вообще без участия TouchGFX/framebuffer/`flushFrameBuffer`) заливку всего экрана цветом + задержку. Первая версия (просто чтобы проверить факт заливки) была зелёной:
```c
/* USER CODE BEGIN 2 */
ST7735_Init();

// ДИАГНОСТИКА: прямая заливка всего экрана, полностью в обход TouchGFX
{
    ST7735_SetAddrWindow(0, 0, 159, 79);
    static uint8_t fillBuf[160 * 2];
    for (int i = 0; i < 160; i++) {
        fillBuf[i * 2]     = 0x07;
        fillBuf[i * 2 + 1] = 0xE0;
    }
    for (int row = 0; row < 80; row++) {
        LCD_WriteData(fillBuf, sizeof(fillBuf));
    }
    HAL_Delay(3000); // держим цвет 3 секунды
}
/* USER CODE END 2 */
```
- **Результат: экран стал идеально равномерно зелёным.** Ни единого шумового пикселя.
- **Это ключевой момент.** Он доказал: SPI4, `ST7735_Init()`, `LCD_WriteData()`, `ST7735_SetAddrWindow()` — всё работает идеально. Проблема гарантированно НЕ в аппаратной части и не в низкоуровневом ST7735-коде.
- Но тут вскрылась методологическая ошибка: в Шаге 2 диагностическая версия `flushFrameBuffer` тоже красила в зелёный. Значит "стало зелёным" не доказывает, работает ли именно TouchGFX — экран мог остаться зелёным и от заливки из `main()`, и от заливки из `flushFrameBuffer`, разницы на глаз никакой. Тест ничего не разделял.

**Шаг 4. Уточнённый тест: разные цвета до/после, чтобы отличить "TouchGFX не запускается" от "TouchGFX запускается, но рисует неправильно".**
Поменяли цвет заливки в `main.c` на **красный** (0xF800 в RGB565), оставив диагностическую `flushFrameBuffer` из Шага 2 **зелёной** (0x07E0). Изменили только один байтовый паттерн:
```c
for (int i = 0; i < 160; i++) {
    fillBuf[i * 2]     = 0xF8; // RGB565 красный, старший байт
    fillBuf[i * 2 + 1] = 0x00; // младший байт
}
```
Теперь смысл теста однозначный: main.c заливает КРАСНЫЙ на 3 секунды → потом стартует `while(1)` с `MX_TouchGFX_Process()`. Если TouchGFX реально запускает рендер — экран переключится на ЗЕЛЁНЫЙ (из диагностической `flushFrameBuffer`). Если не запускает — останется красным навсегда.
- **Результат: красный навсегда, зелёный не появился.**
- Вывод: TouchGFX **вообще не вызывает `flushFrameBuffer`** — framework не просто рисует неправильные данные, он не рисует НИЧЕГО. Значит баг — на уровне "что запускает рендер", а не "что дисплей делает с готовыми пикселями".

**Шаг 5. Поиск причины: почему framework не запускает рендер?**
Поискали по всему проекту вызовы `HAL::getInstance()->tick()` (это функция TouchGFX, которая должна периодически "подталкивать" внутреннюю машину состояний framework — таймеры анимаций, проверка "надо ли перерисовать", и в конце цепочки — вызов `flushFrameBuffer`).
- **Нашли: такого вызова нет вообще.** Ни в одном файле проекта.
- Причина в конфигурации CubeMX: `Driver → Application Tick Source: Custom` (см. Часть 3.5). Значение "Custom" означает буквально "я сам подключу источник тиков" — CubeMX в этом режиме НЕ генерирует автоматический вызов, в отличие от готовых демоплат ST, где эту работу делает LTDC VSYNC-прерывание.

**Шаг 6. Первая попытка исправить — неправильная (ошибка компиляции).**
Попытались вызвать `HAL::getInstance()->tick()` напрямую из обычной C-функции. Получили ошибку компилятора:
```
'virtual void touchgfx::HAL::tick()' is protected within this context
```
`tick()` объявлен `protected` в классе `HAL` — вызвать его можно только из кода, который сам является методом класса-наследника (или самого `HAL`), а не из произвольной внешней функции, даже если у нас есть указатель на объект.

**Шаг 7. Изучили заголовочный файл `HAL.hpp`, чтобы понять правильный API.**
Оказалось, что `tick()` **не предназначен для вызова снаружи вообще**. Он вызывается ИЗНУТРИ другого метода — `backPorchExited()`:
```cpp
virtual void backPorchExited()
{
    swapFrameBuffers();
    tick();          // <-- вот кто на самом деле должен звать tick()
}
```
А `backPorchExited()` в свою очередь вызывается автоматически из сгенерированного `touchgfx_taskEntry()` (в `TouchGFXConfiguration.cpp`), но только если `OSWrappers::isVSyncAvailable()` возвращает `true`:
```cpp
void touchgfx_taskEntry()
{
    if (OSWrappers::isVSyncAvailable())
    {
        hal.backPorchExited();
    }
}
```
А `isVSyncAvailable()` — это просто проверка одного флага (`vsync_sem`), который выставляется в `1` функцией `OSWrappers::signalVSync()`. И вот `signalVSync()` — это **публичный** метод, и в его описании буквально написано: "This function is called from an ISR" (эта функция должна вызываться из прерывания).

Получилась полная картина цепочки вызовов (сверху вниз — кто кого зовёт):
```
[SysTick прерывание, каждую 1мс]
        │
        ▼
touchgfx_tick()                          ← наша новая функция-обёртка
        │
        ├─► HAL::getInstance()->vSync()          (публичный метод — просто счётчик кадров)
        └─► OSWrappers::signalVSync()            (публичный метод — ставит флаг vsync_sem=1)
                    │
                    ▼  (на следующей итерации главного цикла while(1))
touchgfx_taskEntry()  →  проверяет isVSyncAvailable() (флаг vsync_sem)
        │
        ▼ если флаг=1
hal.backPorchExited()
        │
        ├─► swapFrameBuffers()
        └─► tick()  ← вот тот самый protected-метод, который двигает всю логику
                    отрисовки виджетов и в итоге зовёт flushFrameBuffer(rect)
```

### 7bis.2b Какие конфигурации/настройки проверили и что там оказалось

Для полноты — список всего, что мы сверили в конфигурации проекта (через `grep` по исходникам, без открытия CubeMX), и что каждая проверка показала:

| Что проверяли | Где смотрели | Что нашли |
|---|---|---|
| Включён ли D-Cache (Cortex-M7) | `Core/Src/main.c`, поиск `SCB_EnableDCache`/`SCB_EnableICache` | Не найдено — кэш выключен. Значит гипотезы про "кэш хранит устаревшую копию framebuffer" неприменимы вообще, пока кэш не включат |
| Конфигурация MPU | `Core/Src/main.c`, функция `MPU_Config()` | Настроен только Region 0 (стандартный boilerplate CubeMX — запрет выполнения кода в некоторых системных областях), никакой отдельной MPU-области под RAM_D1/framebuffer нет. Не связано с багом |
| Реальные параметры SPI4 (baud/CPOL/CPHA/DataSize) | `Core/Src/main.c`, функция `MX_SPI4_Init()` | `SPI_MODE_MASTER`, `SPI_DIRECTION_2LINES_TXONLY`, `SPI_DATASIZE_8BIT`, `CLKPolarity = LOW`, `CLKPhase = 1EDGE`, `BaudRatePrescaler = 16` — полностью совпадает с тем, что описано в Части 3.2 как рабочая конфигурация. Ничего не "уехало" со времён ручных тестов |
| Есть ли где-то вызов `HAL::getInstance()->tick()` | Поиск по всему проекту (`grep -r "tick("`) | Не найдено ни одного вызова — это и есть корень проблемы (см. 7bis.3) |
| Порядок `MX_TouchGFX_Init()` vs `ST7735_Init()` в `main()` | `Core/Src/main.c`, функция `main()` | `MX_TouchGFX_Init()` вызывается ДО `ST7735_Init()`. На первый взгляд подозрительно, но не является багом: весь код в `main()` до `while(1)` выполняется последовательно и блокирующе, поэтому к моменту, когда TouchGFX реально начинает слать кадры (внутри `while(1)`), `ST7735_Init()` уже давно отработал |

### 7bis.3 Итоговая причина (коротко)

**TouchGFX сконфигурирован с `Application Tick Source = Custom`, а это требует, чтобы разработчик сам периодически вызывал `OSWrappers::signalVSync()` (обычно из таймерного прерывания). Этот вызов отсутствовал в проекте, поэтому framework никогда не запускал цикл рендеринга, и `flushFrameBuffer()` не вызывался вообще ни разу. То, что было видно на экране ("снег") — это не битые данные и не баг рисования, а просто случайный мусор в видеопамяти (GRAM) самого ST7735, оставшийся там со включения питания: команда SWRESET не гарантирует очистку GRAM, а поскольку TouchGFX никогда не писал в неё ни одного пикселя, этот мусор оставался видимым бесконечно.**

### 7bis.4 Все изменения в коде — построчно, с объяснением "зачем"

#### Изменение 1: `Core/Src/stm32h7xx_it.c` — добавили прототип функции

**Было** (секция `USER CODE BEGIN PFP`, пустая):
```c
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */
```

**Стало:**
```c
/* USER CODE BEGIN PFP */
extern void touchgfx_tick(void);
/* USER CODE END PFP */
```

**Зачем:** `stm32h7xx_it.c` — это обычный Си-файл (не C++), а функцию `touchgfx_tick()` мы определили в C++-файле (`TouchGFXHAL.cpp`). Чтобы Си-код мог вызвать C++-функцию, её нужно объявить как `extern` с Си-совместимой сигнатурой (без `class`/`namespace` — обычная функция). Само определение этой функции в `.cpp`-файле обёрнуто в `extern "C" { ... }`, что говорит компилятору C++ "не делай name mangling для этой функции, оставь имя как в Си" — тогда линковщик сможет связать вызов из `.c`-файла с реализацией в `.cpp`-файле.

#### Изменение 2: `Core/Src/stm32h7xx_it.c` — добавили вызов в `SysTick_Handler`

**Было:**
```c
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}
```

**Стало:**
```c
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  touchgfx_tick();
  /* USER CODE END SysTick_IRQn 1 */
}
```

**Зачем:** `SysTick_Handler` — это аппаратное прерывание, которое срабатывает автоматически каждую 1 миллисекунду (стандартная настройка HAL для системного таймера). Оно уже существовало и вызывало `HAL_IncTick()` (это функция самого HAL, отвечает за системные миллисекундные отметки времени, используется, например, внутри `HAL_Delay()`). Мы добавили туда СВОЙ вызов `touchgfx_tick()`, чтобы получить готовый, стабильный, гарантированно периодический источник "тиков" для TouchGFX — то самое "Custom" источник, который framework ждал от нас все это время. Технически можно было бы использовать отдельный аппаратный таймер вместо SysTick, но раз SysTick уже тикает каждую 1мс и прерывание уже настроено — проще и надёжнее подключиться к нему, чем создавать новый таймер.

#### Изменение 3: `TouchGFX/target/TouchGFXHAL.cpp` — добавили `#include`

**Было:**
```cpp
#include <TouchGFXHAL.hpp>
#include <MyButtonController.hpp>
```

**Стало:**
```cpp
#include <TouchGFXHAL.hpp>
#include <MyButtonController.hpp>
#include <touchgfx/hal/OSWrappers.hpp>
```

**Зачем:** нам нужен класс `touchgfx::OSWrappers` и его метод `signalVSync()` — они объявлены именно в этом заголовочном файле. Без него компилятор не знал бы, что такое `OSWrappers`.

#### Изменение 4: `TouchGFX/target/TouchGFXHAL.cpp` — добавили саму функцию `touchgfx_tick()`

**Было:** такой функции не существовало вообще.

**Стало** (добавлено сразу после `using namespace touchgfx;`):
```cpp
extern "C" void touchgfx_tick(void)
{
    HAL::getInstance()->vSync();
    OSWrappers::signalVSync();
}
```

**Зачем, построчно:**
- `extern "C"` — как объяснено в Изменении 1, чтобы функцию можно было вызвать из обычного Си-файла (`stm32h7xx_it.c`) без проблем с именами C++.
- `HAL::getInstance()` — TouchGFX использует паттерн "синглтон": во всей программе существует ровно один объект класса `HAL` (в нашем случае это фактически объект `TouchGFXHAL`, созданный в `TouchGFXConfiguration.cpp` как `static TouchGFXHAL hal(...)`), и `getInstance()` — это способ получить указатель на этот единственный объект из любого места программы, не таская этот указатель через кучу параметров функций.
- `->vSync()` — публичный метод, который увеличивает внутренний счётчик кадров framework (`vSyncCnt`). Используется framework для подсчёта "пропущенных" кадров и для компенсации частоты кадров (см. `setFrameRateCompensation`). Строго говоря, для того, чтобы просто ЗАСТАВИТЬ экран рисоваться, этот вызов не обязателен — работает и без него — но это "правильный" вызов, который выполняет по документации настоящее аппаратное прерывание VSYNC на платах со встроенным LCD-контроллером (LTDC), и мы его добавили, чтобы наш "самодельный VSYNC" вёл себя максимально похоже на настоящий.
- `OSWrappers::signalVSync()` — это САМОЕ важное: единственная строка, которая реально решает проблему. Она выставляет внутренний флаг (`vsync_sem = 1`), который framework проверяет в `touchgfx_taskEntry()` каждую итерацию главного цикла `while(1)`. Если флаг стоит — framework считает, что "настало время нарисовать новый кадр", и запускает всю цепочку: сравнить, что изменилось на экране → перерисовать изменившиеся виджеты в framebuffer → вызвать `flushFrameBuffer(rect)` с координатами того, что изменилось → наш код в `flushFrameBuffer` отправляет эти пиксели по SPI на физический экран.

#### Итог по объёму изменений

Всего **4 строки нового кода** в 2 файлах (`stm32h7xx_it.c` — 2 строки, `TouchGFXHAL.cpp` — include + 4-строчная функция) полностью решили проблему. Это типичный случай для embedded-разработки: сама "починка" крошечная, но чтобы понять, КАКИЕ 4 строки нужны и КУДА их вставить, потребовалось методично пройти по всей цепочче вызовов внутри библиотеки framework и прочитать документацию прямо в заголовочных файлах (`HAL.hpp`, `OSWrappers.hpp`) — готовых ответов "просто добавь эту строку" в официальной документации ST для конфигурации Custom Tick Source почти нет, это всплывает только на форумах и через собственное чтение исходников.

### 7bis.5 Побочный урок: как правильно тестировать гипотезы в embedded (без дебаггера)

Раз у нас не было SWD-дебаггера (только перепрошивка через DFU), каждая "итерация" — это полный цикл Build → Clean → Flash → посмотреть на экран, и это дорого по времени. Поэтому важно было:
1. **Менять только ОДНУ переменную за тест.** Первая попытка (Шаг 2) была нечистой именно потому, что не развели по цветам "до" и "после" — тест технически ничего не доказал, хотя мы сначала подумали, что доказал.
2. **Выбирать тесты, которые дают однозначный, видимый глазом ответ** — а не "наверное, стало чуть лучше". Заливка сплошным цветом идеальна для этого: либо экран равномерный, либо нет, тут нет пространства для интерпретаций.
3. **Идти от простого/низкоуровневого к сложному/высокоуровневому.** Мы могли сразу полезть разбираться в недрах TouchGFX HAL.hpp, но сначала за 2 быстрых теста исключили аппаратную часть — и только потом стали читать библиотечный код, зная точно, что искать нужно там.



### 8.1 Продолжение обучения на маленьком экране
Освоенные концепции TouchGFX: Screen, Container, Box, TextArea, Button (с ограничением по размеру — стандартные пресеты кнопок минимум ~240×50px, не влезают на 160×80, нужно искать "Small"-пресеты или собирать кастомные кнопки из Box+текст), Interactions (Trigger/Action), Mixins (ClickListener, Draggable, FadeAnimator, MoveAnimator).

### 8.2 Переход на большой экран

**Модуль**: Waveshare 3.5inch RPi LCD (A) — 480×320, резистивный тач (контроллер XPT2046), драйвер экрана SPI (семейство ILI9486-подобных).

Полная распиновка модуля (26-pin разъём):
| № пина | Сигнал |
|---|---|
| 1, 17 | 3.3V (питание логики) |
| 2, 4 | 5V (питание подсветки — ОБЯЗАТЕЛЬНО, отдельно от 3.3V) |
| 6,9,14,20,25 | GND |
| 11 | TP_IRQ (прерывание тача, опционально) |
| 18 | LCD_RS (DC) |
| 19 | LCD_SI/TP_SI (общий MOSI для экрана и тача) |
| 21 | TP_SO (MISO, только для тача — у экрана нет обратного канала) |
| 22 | RST |
| 23 | LCD_SCK/TP_SCK (общий clock) |
| 24 | LCD_CS |
| 26 | TP_CS |

Планируемое подключение к STM32 (SPI2, свободный блок — SPI1 занят флешкой, SPI4 занят маленьким экраном):
- SCK → PB13 (нужно перепроверить через CubeMX/мультиметр — не подтверждено окончательно)
- MISO → PB14 (подтверждено свободен)
- MOSI → PB15 (подтверждено свободен)
- LCD_CS → PC0, LCD_DC → PC1, RST → PC2, TP_CS → PC3, TP_IRQ → PC4 (опционально) — свободные GPIO

Важные отличия конфигурации SPI2 от SPI4:
- Mode должен быть **Full-Duplex Master** (не Transmit Only!) — тачскрину нужно читать координаты через MISO
- TouchGFX Framebuffer Strategy для этого экрана: рекомендуется **Partial Framebuffer** (не Single/Double Buffer целиком) — полный буфер 480×320×2 = ~300КБ не помещается компактно в доступную RAM с учётом разбивки по доменам (AXI SRAM всего 192KB непрерывно)
- STM32H723/733 подтверждён DMA2D (Chrom-ART) в периферии по AN5020 — можно рассмотреть включение ускорения при работе с полноценным framebuffer, но НЕ подходит для Partial Framebuffer сценария так же прямолинейно

### 8.3 Общая архитектура целевого проекта (Bluetooth Audio DSP Hub)

Название рабочее: "Bluetooth-аудио-хаб с DSP-обработкой" (не "синтезатор" — синтезатор генерирует звук с нуля, здесь обработка существующего сигнала).

**Общая идея**: 
- Источник звука (телефон/компьютер) → Bluetooth или USB → STM32 → DSP-эффекты в реальном времени → выход на Bluetooth-колонку/наушники ИЛИ на прямой DAC (PCM5102)
- Управление через большой тач-экран: кнопки переключения эффектов, слайдеры параметров, визуализация (спектроанализатор/осциллограмма)

**Аппаратные компоненты (планируются, не куплены/не подключены на момент письма)**:
- STM32H723VGT6 (эта плата) — центральный контроллер, вся DSP-обработка
- 2× ESP32 (LuaNode32 38-pin и ESP32-S3-WROOM) — для Bluetooth-соединений (один как источник/приём от телефона, второй как выход на колонку/наушники)
- PCM5102 (I2S DAC, 32bit/384kHz) — альтернативный прямой аудио-выход, минуя Bluetooth
- Большой TFT-тач-экран (см. 8.2)

**Причина использования 2 ESP32 вместо одного**: A2DP профиль Bluetooth — только для передачи качественного звука В ОДНУ сторону (слушать музыку). Для двунаправленной связи (например, микрофон гарнитуры на митинге) нужен HFP/HSP профиль, но там принудительно урезанное качество (узкополосный голосовой кодек). Совмещение A2DP+HFP на одном ESP32 одновременно — подтверждённо сложная, недоработанная задача (найдены живые обсуждения разработчиков, "упирающиеся в стену" при попытке).

**Альтернатива для двунаправленного аудио (наушники с микрофоном + компьютер)**: использовать не Bluetooth, а **USB Audio Class** через встроенный USB OTG HS самого STM32 — composite-устройство с раздельными Playback и Recording интерфейсами, что даёт полный дуплекс без ограничений качества Bluetooth-профилей.

**Периферийная архитектура STM32 для аудио**: несколько независимых блоков SAI (Serial Audio Interface, каждый с суб-блоками A/B) позволяют держать несколько одновременных I2S-потоков (вход от одного источника + выход на другое устройство параллельно, без программного мультиплексирования одного порта).

### 8.4 Список DSP-эффектов (два уровня приоритета обсуждались)

**"Практический/audiophile" набор** (для качественного прослушивания, если бы был такой фокус):
1. Параметрический эквалайзер (5-10 полос)
2. Bass Boost с настраиваемой частотой среза
3. Loudness Compensation (тонкомпенсация, ISO 226)
4. Баланс L/R
5. Лимитер
6. **Crossfeed** (для наушников — имитация "перетекания" звука между каналами, как на колонках; недооценённая, но важная фича)
7. Auto-EQ по модели наушников (база AutoEQ на GitHub)
8. Жанровые пресеты EQ
9. Компрессор / "ночной режим"
10. Stereo Widener

**"Вау-эффект" набор** (финально выбранный фокус проекта — впечатляющие эффекты, не тонкая аудиофильская коррекция):

Пространственные: Cathedral/Hall Reverb, 8D Audio (вращающийся звук), Binaural 3D (HRTF)

Модуляционные: Chorus, Flanger, Phaser, Tremolo, Ring Modulator

Питч/время: Pitch Shifter, Vocoder, Slowed+Reverb ("chopped & screwed"), Auto-Tune

Лоу-фай/искажение: Bitcrusher, Telephone/Radio Filter, Vinyl/Tape Saturation, Distortion/Fuzz

Ритмические: Auto-Wah, Stutter/Glitch, Reverse Echo

Визуализация (для экрана): Реактивная на музыку частичная визуализация (particle/bar reactive), не просто статичные столбики — с DMA2D-ускорением анимации

**Особый интерес — Audio Enhancer/Exciter**: детально обсуждён механизм (highpass фильтр → waveshaper нелинейность → микс с оригиналом в малой пропорции), варианты waveshaper-функций (tanh для чётных/тёплых гармоник, кубическая для нечётных/резких, диодный клиппер), multiband-версия (разделение на 2-3 полосы с разной интенсивностью, как BBE Sonic Maximizer), адаптивная версия (реагирует на громкость входного сигнала). Параметры для UI: Drive/Amount, Crossover Frequency, Mix (Dry/Wet), Character (Tube↔Digital), Dynamic Response.

### 8.5 UI-требования для большого экрана (изначальный запрос)
- Нижний ряд крупных кнопок (переключение эффектов)
- Верхний правый угол — маленькие кнопки входа в настройки
- Левая колонка на всю высоту — маленькие иконки (индикаторы подключения Bluetooth вход/выход, вкл/выкл звука)
- Самый большой слой — визуализация (спектр/осциллограмма)
- Отдельные экраны (TouchGFX Screens) для настроек каждого эффекта, с слайдерами параметров

---

## ЧАСТЬ 9. ПРОЧИЕ ТЕХНИЧЕСКИЕ ЗАМЕТКИ И УРОКИ

1. STM32H723/H743/H750/H7B0 используют общий дизайн платы у WeAct (один и тот же PCB под разные чипы) — распиновка периферии (SPI4 для LCD и т.д.) идентична между вариантами, что позволило найти точную схему через плату-сестру на H743, когда прямая схема для H723-версии была недоступна из-за блокировки автоматического доступа к папке Hardware на GitHub
2. Cortex-M7 (в отличие от более простых MCU): суперскалярность (до 2 инструкций/такт), I/D-cache 16KB каждый, FPU с double precision — редкость для микроконтроллеров
3. DMA — критичен для производительности при работе с большими буферами (SPI/I2S/ADC), но требует внимания к кэш-когерентности на H7 (в отличие от F1/F4 без кэша) — типичный баг новичков: DMA пишет мимо кэша, ядро читает устаревшую копию из кэша
4. SPI шина — можно вешать несколько устройств (общий SCK/MOSI/MISO, отдельный CS на каждое), но для надёжности и разных скоростей лучше разносить на разные аппаратные SPI-блоки, если такие свободны
5. Bluetooth: A2DP (качественная музыка, одна сторона) и HFP/HSP (голос, обе стороны, урезанное качество) — принципиально разные профили, нельзя просто "сделать A2DP двунаправленным"
