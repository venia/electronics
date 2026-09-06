# STM32H723VGT6 + Waveshare 3.5" TFT Touch Shield (без графического фреймворка)

Проект (с нуля): подключить Waveshare 3.5" TFT Touch Shield (ILI9486, 480x320, резистивный тач
XPT2046) к плате **WeAct Studio Mini STM32H723VGT6** по SPI, чистым HAL/регистрами, без TouchGFX/LVGL.

Среда: STM32CubeMX (отдельное приложение) + STM32CubeIDE 2.2.0.

## Статус на старте

- Питание проверено: экран подключён, светится ровным белым по всей площади через шлейф →
  подсветка и питание модуля исправны (см. `my_project1.jpg`).
- Контакты уже распаяны согласно таблице ниже (см. `ESP-GPIO.jpg`).
- Ардуино-референс проверен и работает "из коробки": `Arduino\LCD_Show` и `Arduino\LCD_Touch`
  из официального пакета Waveshare (`3.5inch_TFT_Touch_Shield_Code_V2\...\Arduino\`).

## Железо

### Плата WeAct Studio Mini STM32H723VGT6

- Cortex-M7, LQFP100, 82 физических GPIO
- Кварц HSE 25 МГц, кварц LSE 32.768 кГц
- Занятая периферия на плате (НЕ трогать при выборе пинов):

| Периферия | Пины |
|---|---|
| SPI-флешка (W25Q64, обычная) | PB3(CLK), PB4(MISO), PD6(CS), PD7(MOSI) |
| QSPI-флешка (вторая, W25Q64) | PB2(CLK), PB6(NCS), PD11(IO0), PD12(IO1), PE2(IO2), PD13(IO3) |
| MicroSD (SDMMC1) | PC8,PC9,PC10,PC11,PC12,PD2,PD4 |
| Камера (DCMI + I2C1) | PA4,PA6,PA7,PA8,PB7,PC6,PC7,PD3,PE0,PE1,PE4,PE5,PE6,PB8,PB9 |
| Кварцы | PH0,PH1 (HSE 25МГц), PC14,PC15 (LSE) |
| USB | PA11(DN),PA12(DP) |
| SWD | PA13(SWDIO),PA14(SWCLK) |
| Встроенный TFT ST7735 (SPI4, 160x80) | PE10(BL),PE11(CS),PE12(SCK),PE13(DC),PE14(MOSI) |
| Синий LED | PE3 |
| Кнопка K1 | PC13 |

Источник: разбор схемы платы, сверено с официальным рендером платы `STM32H7xx_2.jpg` (там же видно,
что PB13/PB14/PB15 физически выведены на гребёнку).

Важно: выбранные ниже пины (SPI2 + PC0-PC5) **не пересекаются** с встроенным дисплеем (SPI4) — можно
будет позже одновременно использовать оба экрана, если понадобится.

### Waveshare 3.5" TFT Touch Shield — какие сигналы нужны

9 сигналов (без SD-карты и TP_BUSY — они не критичны для старта):
SCLK, MISO (для тача), MOSI, LCD_CS, LCD_DC, LCD_RST, LCD_BL, TP_CS, TP_IRQ.

## Финальная таблица пинов (SPI2)

| Waveshare (экран) | STM32 пин | Назначение | Режим в CubeMX |
|---|---|---|---|
| 5V | 5V платы | питание модуля | - |
| GND | GND | земля | - |
| SCLK | **PB13** | SPI2_SCK | Alternate Function (через выбор SPI2) |
| MISO | **PB14** | SPI2_MISO (нужен для тача) | Alternate Function |
| MOSI | **PB15** | SPI2_MOSI | Alternate Function |
| LCD_CS | **PC0** | выбор чипа экрана | GPIO_Output |
| LCD_DC | **PC1** | команда(0)/данные(1) | GPIO_Output |
| LCD_RST | **PC2** | аппаратный сброс экрана | GPIO_Output |
| LCD_BL | **PC3** | подсветка | GPIO_Output |
| TP_CS | **PC4** | выбор чипа тача | GPIO_Output |
| TP_IRQ | **PC5** | прерывание тача | GPIO_Input |

Не используем пока: SD_CS, SD_CD, TP_BUSY (можно подключить позже при необходимости).

Все пины из таблицы проверены как свободные (нет пересечения с таблицей "занятая периферия" выше).

## Шаги в STM32CubeMX (планирование пинов)

Цель этого прохода — только Pinout & базовый System Core, **без** генерации кода. Генерацию кода и
тонкую настройку Clock Tree делаем отдельным шагом после того, как пины подтверждены.

**Прогресс:**
- [x] 1. Создание проекта (STM32H723VGT6)
- [x] 2. RCC → HSE Crystal/Ceramic Resonator
- [x] 3. Trace and Debug → DEBUG = Serial Wire (подтверждено скриншотом: PA13/PA14 зелёные,
      DEBUG_JTMS-SWDIO/DEBUG_JTCK-SWCLK)
- [x] 4. SPI2 = Full-Duplex Master, пины вручную переназначены на PB13(SCK)/PB14(MISO)/PB15(MOSI)
      (подтверждено скриншотом от 2026-09-06 — все три зелёные с правильными подписями)
- [x] 5-8. GPIO для LCD/тача настроены и подписаны (подтверждено скриншотом от 2026-09-06: PC0=LCD_CS,
      PC1=LCD_DC, PC2=LCD_RST, PC3=LCD_BL, PC4=TP_CS — все Output Push-Pull/No pull/Low speed;
      PC5=TP_IRQ — Input/Pull-up)
- [ ] 9-10. Проверка конфликтов, сохранение проекта — в процессе

1. **Создание проекта**
   - Открыть STM32CubeMX (отдельная программа — в CubeIDE 2.2.0 конфигуратор пинов вынесен наружу)
   - `File → New Project` → вкладка **MCU Selector** → в поиске ввести `STM32H723VGT6` → выбрать чип
     (корпус LQFP100) → **Start Project**

2. **System Core → RCC**
   - High Speed Clock (HSE): **Crystal/Ceramic Resonator** (на плате WeAct реальный кварц 25 МГц)
   - Low Speed Clock (LSE), если нужен RTC в будущем: Crystal/Ceramic Resonator (можно пропустить сейчас)

3. **Trace and Debug → DEBUG** (не в System Core → SYS — в этой версии CubeMX Debug вынесен в отдельную
   категорию "Trace and Debug" в списке слева; SYS теперь содержит только Timebase Source)
   - Mode: **Serial Wire** (оставляет SWDIO/SWCLK для отладчика, не даёт CubeMX случайно занять эти пины
     чем-то другим). Часто это уже стоит по умолчанию — проверить, что PA13/PA14 подсвечены как
     зарезервированные на картинке чипа справа.

4. **Connectivity → SPI2**
   - Mode: **Full-Duplex Master** (не "Transmit Only" — тачу XPT2046 нужен приём данных по MISO)
   - После выбора режима CubeMX подсветит пины на картинке чипа. **Обязательно проверить**, что это
     именно PB13 (SCK) / PB14 (MISO) / PB15 (MOSI) — у SPI2 на H723 есть несколько альтернативных
     наборов пинов, CubeMX может по умолчанию предложить не тот набор.
   - Если подсветились не те пины — кликнуть вручную на PB13 на картинке чипа → в выпадающем списке
     выбрать `SPI2_SCK`, аналогично PB14 → `SPI2_MISO`, PB15 → `SPI2_MOSI`.
   - Если снять с пина назначение SPI (например убрать SCK с неправильного пина) — CubeMX может
     автоматически перевести весь Mode в Disable и заодно освободить MOSI/MISO тоже. Это нормально:
     просто заново выставь Mode = Full-Duplex Master и назначь все три пина заново на правильные ноги.
   - Data Size, Prescaler, CPOL/CPHA на этом шаге **не трогать** — это Configuration, настроим на этапе
     программирования (сейчас по умолчанию стоит Data Size = 4 Bits — это тоже поправим позже, нужно
     будет 8 Bits).

5. **GPIO для управляющих линий дисплея**
   - Кликнуть на PC0 на картинке чипа → `GPIO_Output`
   - PC1 → `GPIO_Output`
   - PC2 → `GPIO_Output`
   - PC3 → `GPIO_Output`

6. **GPIO для тача**
   - PC4 → `GPIO_Output` (CS тача)
   - PC5 → `GPIO_Input` (IRQ тача — пока просто вход, без EXTI/прерывания; опрашивать вручную)

7. **Подписать пины** (System Core → GPIO, вкладка со списком пинов, поле **User Label**)
   - PC0 → `LCD_CS`
   - PC1 → `LCD_DC`
   - PC2 → `LCD_RST`
   - PC3 → `LCD_BL`
   - PC4 → `TP_CS`
   - PC5 → `TP_IRQ`
   - **Важно:** вписывать имя БЕЗ суффикса `_Pin` (например `LCD_CS`, а не `LCD_CS_Pin`) — CubeMX сам
     добавит `_Pin`/`_GPIO_Port` при генерации. Если вписать суффикс вручную, получится задвоение
     (`LCD_CS_Pin_Pin`).

8. **GPIO Mode/Speed для этих пинов** (там же, вкладка GPIO, после клика на конкретный пин)
   - PC0-PC4: **Output Push Pull**, No pull-up/pull-down, **Low** или **Medium** speed (не нужна высокая
     скорость фронтов для CS/DC/RST/BL — меньше звона/помех на проводах)
   - Начальный уровень (GPIO output level) можно оставить по умолчанию — реальную полярность
     (какой уровень означает "активно": LOW или HIGH, особенно для LCD_BL и LCD_RST) уточним
     экспериментально на этапе программирования
   - PC5 (TP_IRQ): **Input**, можно включить **Pull-up** (чтобы вход не "плавал", если провод IRQ
     физически не подключён)

9. **Проверка конфликтов**
   - Убедиться, что ни один из пинов PB13/PB14/PB15/PC0-PC5 не подсвечен CubeMX как уже занятый другой
     периферией — на этой плате они свободны (см. таблицу "занятая периферия" выше)

10. **Сохранить проект, код пока не генерировать**
    - `File → Save Project as…` → указать папку `STM32-1-ANALYZE`
    - `Project Manager` → можно уже сейчас проставить Project Name / Location, но кнопку
      **Generate Code** пока не нажимать — сначала проверяем план пинов вместе

### Дальше (следующие шаги, после подтверждения пинов)

- Clock Tree (подбор частоты SYSCLK)
- Project Manager → Toolchain/IDE = STM32CubeIDE → Generate Code
- Импорт в STM32CubeIDE: `File → Import → General → Existing Projects into Workspace`
- Настройка SPI2 Data Size/Prescaler/CPOL/CPHA под ILI9486
- Написание ручного драйвера LCD_WriteCmd/LCD_WriteData, инициализация ILI9486, затем драйвер тача
  XPT2046

## Процедура прошивки через DFU

См. `instruction-upload.jpg` в этой папке — кратко:
1. Зажать **BOOT0**, тапнуть **RESET/NRST**, подождать ~0.5 сек, отпустить BOOT0 → плата в USB DFU
2. STM32CubeProgrammer → Interface USB → Refresh → Connect
3. Erasing & Programming → Browse → указать `.elf` или `.bin` из папки `Debug/`
4. Обязательно галочка **"Run after programming"**
5. Start Programming
