# GFX-WaveShare: STM32H723VGT6 + Waveshare 3.5" SPI TFT (ILI9486) + TouchGFX

Полная хронология проекта: с нуля в CubeMX, экран в TouchGFX Designer, прошивка через
STM32CubeProgrammer, и разбор всех ошибок/багов по порядку — от "не собирается" до
текущего "белый экран", с шагами аппаратной диагностики осциллографом.

---

## ЧАСТЬ 1. ЖЕЛЕЗО

- **Плата**: STM32H723VGT6 Mini Core Board (WeAct-совместимая, Cortex-M7 550МГц max,
  RAM 564KB, ROM 1024KB), подключена к экрану проводами (не готовый шилд).
- **Экран**: Waveshare 3.5" SPI TFT LCD, 480×320, резистивный тач (контроллер XPT2046),
  драйвер экрана семейства ILI9486, разъём — тот же, что у "Waveshare 3.5inch RPi LCD (A)".

### 1.1 Финальная таблица пинов (SPI2)

| Waveshare (экран) | STM32 (нога) | Назначение |
|---|---|---|
| 5V | 5V платы | питание модуля (обязательно 5V, не 3.3V — подсветка) |
| GND | GND (любой) | земля |
| SCLK | PB13 | SPI2_SCK |
| MISO | PB14 | SPI2_MISO (нужен для тача) |
| MOSI | PB15 | SPI2_MOSI |
| LCD_CS | PC0 | выбор чипа экрана |
| LCD_DC | PC1 | команда/данные |
| LCD_RST | PC2 | сброс экрана |
| LCD_BL | PC3 | подсветка |
| TP_CS | PC4 | выбор чипа тача |
| TP_IRQ | PC5 | прерывание тача (можно не подключать, опрашивать вручную) |

SD-карта и TP_BUSY на модуле есть, но не критичны для старта — не подключены.

---

## ЧАСТЬ 2. STM32CubeMX — пошагово, как было сделано

### 2.1 Создание проекта
1. `File → New Project → MCU Selector` → ввести `STM32H723VGTx` → выбрать корпус LQFP100 → `Start Project`

### 2.2 Pinout & Configuration — периферия
- **SPI2** → Mode: **Full Duplex Master** (не Transmit Only! — MISO нужен для тача XPT2046)
  - На картинке чипа кликнуть PB13→SPI2_SCK, PB14→SPI2_MISO, PB15→SPI2_MOSI
- **GPIO** (System Core → GPIO), выставить вручную и переименовать (User Label):
  - PC0 → GPIO_Output → **LCD_CS**
  - PC1 → GPIO_Output → **LCD_DC**
  - PC2 (в этом пакете подписан как **PC2_C**) → GPIO_Output → **LCD_RST**
  - PC3 (подписан как **PC3_C**) → GPIO_Output → **LCD_BL**
  - PC4 → GPIO_Output → **TP_CS**
  - PC5 → GPIO_Input → **TP_IRQ**
  - Для всех Output-пинов: Output level = High, Mode = Output Push-Pull, Pull = No pull, Speed = Low
  - ⚠️ PC2_C/PC3_C — на STM32H7 это пины с внутренним аналоговым свичом (общая площадка с
    компаратором/ОУ). CubeMX сам добавляет в `MX_GPIO_Init()` два вызова
    `HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PC2/PC3, ..._CLOSE)` — без этого GPIO может
    не работать как обычный вывод. Проверено — CubeMX сделал это автоматически, ничего
    руками добавлять не пришлось.
- **CRC** (Computing → CRC) — просто поставить галочку **Activated** (обязательная
  зависимость TouchGFX, без неё генератор ругается "Please Enable CRC IP")

### 2.3 Clock Configuration
- HSI как источник (HSE не разводили) → PLL: M=4, N=25, P=1, Q=2, R=2
- Итог: **SYSCLK = 400МГц**, CPU/Systick = 400МГц, AXI/HCLK3 = 200МГц, APB-шины 100-200МГц
- Отдельно важно: **SPI1,2,3 Clock Mux → PLL1Q = 200МГц** (источник тактирования для SPI2)

### 2.4 Software Packs → X-CUBE-TOUCHGFX
- Mode: галочка **Graphics Application**
- Display:
  - Interface: **Custom**
  - Framebuffer Pixel Format: **RGB565**
  - Width: **480**, Height: **320**
  - **Framebuffer Strategy: "Partial Buffer - GRAM display"** ⚠️ ключевой выбор —
    именно этот вариант (не Single/Double Buffer, не просто "Partial Framebuffer")
    предназначен под дисплеи с собственным GRAM и адресацией через CASET/RASET, как ILI9486
  - Number of Blocks: **3**, Block Size: **2048 bytes**
- Driver:
  - Application Tick Source: **Custom** (см. Часть 4.2 — почему это важно)
  - Use DMA2D Accelerator (ChromART): **No**
  - Real-Time Operating System: **No OS**
- Additional Features: Partial Framebuffer VSync/External Data Reader/Vector Rendering — все Disabled
- Video Decoding: Disabled

### 2.5 Project Manager
- Toolchain/IDE: **STM32CubeIDE**
- **GENERATE CODE**

Что CubeMX реально генерирует этим шагом: `Core/`, `Drivers/`, `Middlewares/ST/touchgfx/*`
(framework, 3rdparty/libjpeg, lib), `TouchGFX/target/generated/*` (glue-код HAL),
`TouchGFX/App/*`, файл-заготовку `TouchGFX/ApplicationTemplate.touchgfx.part`.

⚠️ **Известный баг, с которым уже сталкивались**: один раз CubeMX **пропустил копирование
`Middlewares/ST/touchgfx`** целиком (тысячи файлов фреймворка) — пришлось вручную
скопировать эту папку из соседнего проекта (Project1, та же версия пакета 4.26.1).
Механизм сбоя не выяснен (похоже на нестабильность самого CubeMX/кэша пакетов), но
итоговый воркэраунд — просто взять `Middlewares/ST/touchgfx` из рабочего проекта той же
версии TouchGFX.

---

## ЧАСТЬ 3. TouchGFX Designer 4.26.1 — пошагово

Это **отдельный шаг от CubeMX Generate Code** — этого не было очевидно с первого раза,
и именно пропуск этого шага стал первой причиной ошибок сборки (см. Часть 5.1).

1. Найти `TouchGFX/ApplicationTemplate.touchgfx.part` в дереве проекта в CubeIDE
2. Открыть его — CubeIDE не умеет его отрисовывать как GUI (открывает как текст/JSON),
   правильный способ: правой кнопкой → **Open With → TouchGFX Designer** (или открыть
   Designer отдельно, `File → Open`, указать путь к этому файлу)
   - Приложение "TouchGFX Designer" — отдельная программа, устанавливается из
     `.msi`-инсталлятора, который лежит внутри скачанного пакета CubeMX
     (`Utilities/PC_Software/TouchGFXDesigner/TouchGFX-4.26.1.msi`), не ставится сама
     по себе через CubeMX
3. В Designer: `Screens → screen` — накидать виджеты на канвас (для первого теста —
   три `Box` на весь экран разными цветами: **box1** красный (Y=0, H=107),
   **box2** жёлтый (Y=107, H=107), **box3** зелёный (Y=214, H=106), в сумме закрывают
   всю высоту 320
4. **`Code → Generate Code`** (пункт **меню**, вверху окна — НЕ кнопки Play/`</>`
   внизу справа канваса, те относятся к симулятору/предпросмотру и ничего не генерируют
   для целевой платы!)
   - Прогресс-бар генерации может идти долго (шрифты/тексты компилируются в бинарный вид)
   - Результат — создаются папки, которых раньше не было:
     `TouchGFX/generated/`, `TouchGFX/gui/`, `TouchGFX/simulator/`, `TouchGFX/config/`,
     файл `TouchGFX/target.config`
5. Сохранить проект в Designer (`.touchgfx` файл обновится)

После этого шага можно пересобирать в CubeIDE.

---

## ЧАСТЬ 4. КОД — что дописано руками, и зачем

### 4.1 `Core/Src/main.c` — драйвер ILI9486 (секция `USER CODE BEGIN PV`)

CubeMX сам такой код не генерирует — это ручная реализация протокола ILI9486 поверх
HAL SPI:

```c
#define LCD_CS_LOW()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define LCD_DC_CMD()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_DATA()  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

static void LCD_WriteCmd(uint8_t cmd) {
    LCD_DC_CMD();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

// БЕЗ static — вызывается из C++ (TouchGFXHAL.cpp)
void LCD_WriteData(uint8_t *data, uint16_t len) {
    LCD_DC_DATA();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, data, len, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}
```
Логика: DC-пин переключает контроллер между режимом "это команда" (LOW) и "это данные"
(HIGH) — стандартная конвенция для большинства SPI TFT-контроллеров (ILI93xx/94xx серии).
CS обрамляет каждую транзакцию отдельно (bit-banged, не аппаратный NSS — `SPI_NSS_SOFT`
в конфиге SPI2, чтобы самим управлять CS без привязки к таймингам периферии).

```c
static void ILI9486_Init(void) {
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(150);

    LCD_WriteCmd(0x01); HAL_Delay(120);   // SWRESET
    LCD_WriteCmd(0x11); HAL_Delay(120);   // Sleep out
    LCD_WriteCmd(0x3A); LCD_WriteData8(0x55); // Pixel format RGB565
    LCD_WriteCmd(0xC0); ... // Power Control 1/2/3, VCOM, Gamma — параметры "по даташиту примера"
    LCD_WriteCmd(0x21);                    // INVON (инверсия цвета для этой матрицы)
    LCD_WriteCmd(0x36); LCD_WriteData8(0x48); // MADCTL — ориентация, НЕ проверена на железе
    LCD_WriteCmd(0x29); HAL_Delay(150);    // Display ON
    HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET); // подсветка, полярность НЕ проверена
}

// БЕЗ static — вызывается из C++
void ILI9486_SetAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t buf[4];
    buf[0]=(x0>>8)&0xFF; buf[1]=x0&0xFF; buf[2]=(x1>>8)&0xFF; buf[3]=x1&0xFF;
    LCD_WriteCmd(0x2A); LCD_WriteData(buf,4); // CASET — столбцы
    buf[0]=(y0>>8)&0xFF; buf[1]=y0&0xFF; buf[2]=(y1>>8)&0xFF; buf[3]=y1&0xFF;
    LCD_WriteCmd(0x2B); LCD_WriteData(buf,4); // RASET — строки
    LCD_WriteCmd(0x2C); // RAMWR — после этой команды все последующие data-байты идут в GRAM
}
```
`ILI9486_Init()` вызывается в `main()` один раз, сразу после `MX_TouchGFX_Init()`
(в секции `USER CODE BEGIN 2`).

Пометки в самом коде "проверим на практике" (MADCTL=0x48, полярность подсветки) —
это НЕ подтверждённые значения, взяты как отправная точка из типового примера, реальная
проверка на конкретной матрице ещё не проводилась (см. Часть 6, текущий баг "белый экран"
может быть с этим не связан, но после решения текущей проблемы это первое, что стоит
перепроверить).

### 4.2 `Core/Src/stm32h7xx_it.c` — подключение тика TouchGFX

```c
/* USER CODE BEGIN PFP */
extern void touchgfx_tick(void);
/* USER CODE END PFP */
...
void SysTick_Handler(void)
{
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  touchgfx_tick();
  /* USER CODE END SysTick_IRQn 1 */
}
```
**Зачем**: в CubeMX выбрано `Application Tick Source = Custom` — это значит framework
TouchGFX **не получает** автоматический источник кадровых тиков (в отличие от плат
с LTDC, где это делает VSYNC-прерывание контроллера). Без ручного вызова источника
тиков framework вообще не запускает цикл рендеринга — экран просто никогда не
обновляется (в прошлом проекте на маленьком экране именно это давало эффект
"шума"/"снега" на дисплее). Здесь этот фикс **был перенесён заранее**, ещё до первого
запуска — правильно сделано.

### 4.3 `TouchGFX/target/TouchGFXHAL.cpp` — связка framework ↔ SPI-драйвер

```cpp
extern "C" void touchgfx_tick(void)
{
    HAL::getInstance()->vSync();       // счётчик кадров framework (не обязателен, но "по документации")
    OSWrappers::signalVSync();          // САМАЯ важная строка — говорит framework "пора рисовать кадр"
}
```

Дальше — реализация хуков, обязательных именно для стратегии
**"Partial Buffer - GRAM display"**:

```cpp
namespace touchgfx { void startNewTransfer(); } // форвард-декларация (функция определена в generated-файле)

extern "C" int touchgfxDisplayDriverTransmitActive()
{
    return 0; // HAL_SPI_Transmit блокирующий -> к моменту возврата передача уже завершена
}

extern "C" void touchgfxDisplayDriverTransmitBlock(const uint8_t* pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    ILI9486_SetAddrWindow(x, y, x + w - 1, y + h - 1);

    const uint16_t* src = (const uint16_t*)pixels;
    static uint8_t lineBuf[480 * 2];
    for (int row = 0; row < h; row++)
    {
        for (int col = 0; col < w; col++)
        {
            uint16_t px = src[row * w + col];
            lineBuf[col * 2]     = (uint8_t)(px >> 8);   // RGB565 -> старший байт первым (SPI MSB-first)
            lineBuf[col * 2 + 1] = (uint8_t)(px & 0xFF);
        }
        LCD_WriteData(lineBuf, w * 2);
    }

    startNewTransfer(); // сообщаем аллокатору "блок отправлен, можно брать следующий"
}
```

**Зачем именно так**: при "Partial Buffer" framework сам рисует изменившиеся виджеты в
маленькие блоки (`ManyBlockAllocator<2048, 3, 2>` — 3 блока по 2048 байт = 1024 пикселя
каждый, см. `TouchGFXGeneratedHAL.cpp`), и когда блок готов — сам вызывает
`touchgfxDisplayDriverTransmitBlock(pixels, x, y, w, h)` с готовым указателем на пиксели
именно этого блока и его координатами на экране. Наша задача — просто отправить их по
SPI (с перестановкой байт под RGB565/MSB-first) и вызвать `startNewTransfer()`, чтобы
framework увидел "передача завершена, дай следующий блок, если есть".

```cpp
void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}
```
Просто прокси к generated-версии — именно она внутри себя дергает
`frameBufferAllocator->markBlockReadyForTransfer()` и первый вызов
`touchgfxDisplayDriverTransmitBlock`.

---

## ЧАСТЬ 5. STM32CubeProgrammer — прошивка через DFU (USB)

Отдельного программатора (ST-Link) не использовали — только встроенный USB DFU-бутлоадер.

1. **Перевод платы в DFU**: зажать **BOOT0**, не отпуская — тапнуть **RESET/NRST**,
   подождать ~0.5 сек, отпустить **BOOT0**. Плата виснет в USB DFU-режиме (обычный код
   не выполняется).
2. **STM32CubeProgrammer**: Interface = **USB** → **Refresh** (должно появиться
   "STM32 BOOTLOADER" устройство) → **Connect**
3. Вкладка **Erasing & Programming** → **Browse** → указать файл прошивки.
   - В этом проекте CubeIDE не настроен генерировать `.bin` (нет шага objcopy в
     Build Steps) — прошивали напрямую **`Debug/GFX-WaveShare.elf`**.
     CubeProgrammer прошивает `.elf` так же нормально, как `.bin`/`.hex`, и сам берёт
     адрес старта из файла — вручную вбивать `0x08000000` не нужно (это актуально
     только для чистого `.bin`)
4. Обязательно галочка **"Run after programming"** — без неё плата останется висеть в
   DFU после записи (будет казаться, что "ничего не изменилось", хотя прошивка прошла)
5. **Start Programming**

---

## ЧАСТЬ 6. ХРОНОЛОГИЯ ОШИБОК И ТЕКУЩЕЕ СОСТОЯНИЕ

### 6.1 Ошибка сборки №1 — `fatal error: gui/common/FrontendHeap.hpp: No such file or directory`

**Причина**: `TouchGFX/generated/` и `TouchGFX/gui/` физически не существовали.
CubeMX Generate Code создаёт только glue-код и заготовку, а сам GUI-код (экраны,
шрифты, тексты, `FrontendHeap.hpp`) генерирует **TouchGFX Designer** отдельным шагом
(`Code → Generate Code` в самом Designer, см. Часть 3, шаг 4) — этот шаг ни разу не
выполнялся до этого момента (кнопки Play/`</>` внизу канваса Designer, которые
нажимались, относятся к симулятору/предпросмотру и не создают код для целевой платы).

**Решение**: выполнить `Code → Generate Code` в TouchGFX Designer. После этого
появились `TouchGFX/generated/`, `TouchGFX/gui/`, `TouchGFX/simulator/`,
`TouchGFX/config/`, `TouchGFX/target.config` — компиляция (не линковка) прошла успешно.

### 6.2 Ошибка линковки №2 — сразу после фикса №1

```
undefined reference to `TouchGFXGeneratedHAL::advanceFrameBufferToRect(...)`
undefined reference to `touchgfxDisplayDriverTransmitActive'
undefined reference to `touchgfxDisplayDriverTransmitBlock'
```

**Причины (две независимые)**:
1. `TouchGFXHAL::flushFrameBuffer()` изначально был написан по образцу **Single Buffer**
   стратегии (копия логики из прошлого проекта на маленьком экране) — вызывал
   `getTFTFrameBuffer()`, а generated-код при **Partial Buffer - GRAM display**
   стратегии эта функция всегда возвращает `0` (`// getTFTFrameBuffer() not used for
   selected Frame Buffer Strategy`). Плюс сама `advanceFrameBufferToRect` объявлена
   `inline`, но нигде не используется внутри своего же `.cpp` — компилятор просто не
   генерирует для неё внешний символ, и линковка из другого файла падает.
2. `touchgfxDisplayDriverTransmitActive()`/`touchgfxDisplayDriverTransmitBlock()` —
   обязательные хуки именно для Partial Framebuffer-стратегии (framework их реально
   вызывает, см. `TouchGFXGeneratedHAL.cpp`), но никто их не реализовал — только
   `#warning`-подсказки в generated-шаблоне об этом и говорили.

**Решение**: переписан `TouchGFX/target/TouchGFXHAL.cpp` — `flushFrameBuffer` теперь
просто прокси к generated-версии, а `touchgfxDisplayDriverTransmitActive/Block`
реализованы через уже существующие `ILI9486_SetAddrWindow`/`LCD_WriteData` (см. Часть 4.3).
После этого — **сборка и линковка полностью прошли успешно**.

### 6.3 Прошивка — успешно

DFU-прошивка (Часть 5) прошла без проблем, плата запускается.

### 6.4 Текущий баг — экран сплошной белый (в процессе диагностики)

**Симптом**: после прошивки подсветка включается, но весь экран равномерно белый —
не чёрный, не "снег"/шум, а именно ровный белый цвет, не реагирующий ни на что.

**Уже исключено:**
- ✅ TouchGFX-уровень — временно вставлена **прямая заливка экрана в обход framework**
  (`ILI9486_SetAddrWindow` + `LCD_WriteData` напрямую в `main()`, минуя TouchGFX
  полностью) — экран всё равно остаётся белым → проблема НЕ в связке с TouchGFX,
  а ниже, на уровне SPI/ILI9486-протокола или физики.
- ✅ Скорость SPI — было `SPI_BAUDRATEPRESCALER_8` (25МГц из 200МГц источника),
  снижено до `SPI_BAUDRATEPRESCALER_64` (~3.125МГц) — белый экран остался тем же,
  скорость не является причиной.
  ⚠️ Это изменение сделано прямо в `MX_SPI2_Init()` в `main.c`, **вне блоков
  `USER CODE`** — при следующей генерации кода из CubeMX оно слетит. Если скорость
  окажется важна для финального решения — обязательно продублировать в самом CubeMX
  (Pinout & Configuration → SPI2 → Baud Rate Prescaler).
- ✅ Пайка/непрерывность проводов — мультиметром (прозвонка, питание выключено)
  проверены SCLK/MOSI/CS/DC/GND между платой и экраном — везде норма.

**Диагностика осциллографом (Siglent SDS802X HD) — для удобства прошивка временно
заменена на бесконечный цикл** (см. текущее содержимое `USER CODE BEGIN 2` в `main.c` —
`while(1)` с попеременной заливкой красный/синий раз в секунду, в обход TouchGFX;
**это временный диагностический код, не финальная логика**, TouchGFX/`while(1)` с
`MX_TouchGFX_Process()` ниже по коду сейчас не достигается):

| Сигнал | Результат | Вывод |
|---|---|---|
| **CS (PC0)** | Периодически проседает вниз каждые ~2.48мс во время передачи (320 строк подряд), между кадрами держится высоким — идеально совпадает с ожидаемым по коду поведением | ✅ Работает корректно |
| **SCLK (PB13)** | Чистый меандр, частота 3.12693МГц (ровно 200МГц/64, как и настроено), фронты чистые ~26-27нс, без звона | ✅ Работает корректно |
| **MOSI (PB15)** | Осмысленный сигнал данных, период ~5.1µs — соответствует 16 битам на клоке 3.125МГц (наш байт-паттерн 0xF8/0x00) | ✅ Работает корректно |
| **DC (PC1)** | Первая попытка — timebase был оставлен 1µs/div (после проверки SCLK/MOSI), на таком масштабе редкий провал DC (раз в секунду, доли мс) поймать нельзя — результат "ровная линия" **ничего не доказывает**, тест нужно переделать | ⚠️ **Не проверено корректно — нужно повторить на 2ms/div** |
| **RST (PC2)** | Ещё не проверялось вообще | ❌ **Следующий шаг** |

**На чём остановились**: план — проверить **RST (PC2)** через **Single-триггер**
(timebase 50мс/div, уровень ~1.65В, фронт falling), физически нажав RESET на плате
STM32 в момент вооружения триггера — должен поймать однократный провал ~20мс (по коду
`ILI9486_Init()`: RST LOW на 20мс, потом постоянно HIGH). Если RST не двигается —
контроллер экрана держится в аппаратном сбросе и игнорирует весь (полностью корректный!)
SPI-трафик — это объяснило бы симптом "белый экран всегда, что бы ни слали". После RST
— доделать корректный тест DC на правильном timebase (2ms/div).

**RST (PC2) — проверено осциллографом:** при нажатии физического RESET на плате STM32
уровень на линии проседает как положено → ✅ работает корректно.

**DC (PC1) — перепроверено на правильном timebase (2ms/div):** видны редкие короткие
провалы вниз (моменты отправки команд CASET/RASET/RAMWR) на фоне в основном высокого
уровня → ✅ работает корректно.

**Итог по всем 5 сигналам: CS, SCLK, MOSI, RST, DC — все подтверждены осциллографом
как электрически корректные.** МК гарантированно передаёт правильно оформленный
SPI-протокол. Экран при этом остаётся ровным белым — то есть проблема не в проводке
и не в тайминге сигналов.

### 6.5 Уточнение модели экрана

По фото задней стороны платы экрана выяснилось: это **не** "Waveshare 3.5inch RPi LCD (A)"
(HAT для Raspberry Pi), а **"Waveshare 3.5inch TFT Touch Shield"** — версия для Arduino,
320×480, XPT2046 touch, контроллер **ILI9486** (подтверждено официальной вики Waveshare).
На плате есть блок **"SPI Config"** — DIP-переключатель `SCLK\D13, MISO\D12, MOSI\D11`,
выбирающий, куда внутри платы подключены линии SPI (на выделенные пины заголовка или на
D13/D12/D11 под Arduino Uno).

**Проверено — переключатель SPI Config в обоих положениях** → результат не изменился,
экран всё равно белый. Значит переключатель не является причиной (либо не влияет на
используемые нами пины вообще).

### 6.6 Сверка инициализации с рабочим эталоном

Нашли подтверждённо рабочую библиотеку именно под эту матрицу:
`ImpulseAdventure/Arduino-TFT-Library-ILI9486` на GitHub (сделана под "3.5 inch RPi LCD (A)
320x480 from Waveshare" — та же панель). Сравнили построчно с нашим `ILI9486_Init()`,
нашли и исправили расхождения:
- Убраны лишние команды **0xC0 (Power Control 1)** и **0xC1 (Power Control 2)** —
  в рабочем эталоне их нет вообще
- **0xC2 (Power Control 3)**: было `0x55`, исправлено на `0x44` (как в эталоне)
- Инверсия дисплея: было `0x21` (INVON), исправлено на `0x20` (INVOFF — вариант "(A)")
- Пиксель-формат (0x3A=0x55), гамма (0xE0/0xE1), MADCTL(0x36=0x48) уже совпадали

**Результат после этого фикса — экран остался белым, без изменений.**

### 6.7 Гипотеза "не та ориентация/адресация" — проверено, не подтвердилось

Предположение: TouchGFX/CubeMX настроены на альбомные 480×320, а MADCTL=0x48
(скопированный из эталона) не включает бит **MV** (row/column exchange, бит 5) —
то есть остаётся портретная адресация панели (родной GRAM ILI9486 — 320×480), а мы
шлём CASET/RASET исходя из альбомных координат (0-479 по X), что может уходить в
невалидную область адресации.

**Проверено**: MADCTL изменён с `0x48` на `0x68` (добавлен бит MV) — **экран всё равно
остался полностью белым, без единого изменения**. Гипотеза не подтвердилась (или MV —
не единственное, что нужно было поменять; MX/MY тоже не переподбирались после этого).

### 6.8 Текущий статус — на чём остановились

Все проверенные и **исключённые** причины:
- ❌ Связка с TouchGFX (прямая заливка в обход framework — тот же результат)
- ❌ Скорость SPI (25МГц и 3.125МГц — без разницы)
- ❌ Пайка/непрерывность проводов (CS/SCLK/MOSI/DC/GND — мультиметром чисто)
- ❌ Электрическая корректность CS/SCLK/MOSI/RST/DC (все 5 подтверждены осциллографом)
- ❌ Положение переключателя SPI Config (оба положения — без разницы)
- ❌ Параметры инициализации Power Control/VCOM/Inversion (исправлены на подтверждённо
  рабочие значения из эталонной библиотеки — без разницы)
- ❌ Бит MV в MADCTL (добавлен — без разницы)

**Ещё не проверено — кандидаты на следующую сессию:**
1. **Реальное напряжение 5V под нагрузкой** (не прозвонка, а мультиметр в режиме
   измерения напряжения, во время работы платы) — возможно, 5V проседает и панели не
   хватает питания для внутренних DC-DC/заряд-насосов, генерирующих VGH/VGL/GVDD —
   это дало бы ровно такой симптом (панель физически не может модулировать свет,
   светится "по умолчанию" = белый), независимо от абсолютно корректного SPI
2. **Подсветка (LCD_BL)** — то, что она "визуально горит", не проверено осциллографом/
   логически как факт переключения по нашему GPIO — возможно, подсветка на модуле вообще
   не управляется этим пином (запитана напрямую, всегда горит) — тогда "подсветка
   работает" ничего не подтверждает, и полярность/логика BL не проверена по-настоящему
3. **Разные MX/MY биты MADCTL** — пробовали только добавление MV, но не перебирали
   остальные комбинации (0x28, 0x88, 0xE8 и т.д.) — раз MV не помог, возможно дело не в
   ориентации вообще, и дальше копать MADCTL уже низкий приоритет
4. Возможно стоит попробовать **записать в GRAM не через RAMWR полный кадр, а прочитать
   Read Display Power Mode (0x0A) или Read Display Status (0x09)** через MISO — раз SPI2
   настроен Full Duplex, можно реально прочитать регистр состояния с контроллера и
   получить железное доказательство, что чип вообще отвечает на команды (а не просто
   молча их принимает) — это развяжет "чип получает команды, но физически не может
   отобразить" от "чип в принципе не отвечает/не тот чип/не то питание"
5. Проверить не спутан ли сам модуль — есть шанс, что это клон с другим (несовместимым)
   вариантом ILI9486 или с дефектом самой платы (заводской брак) — стоит поискать
   даташит/маркировку конкретной SPI-микросхемы на плате рядом с шлейфом экрана
