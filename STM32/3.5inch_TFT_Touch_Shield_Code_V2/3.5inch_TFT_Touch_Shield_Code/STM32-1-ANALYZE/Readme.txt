Привет

я хочу подключить к плате STM32H723VGT6 Mini Core Board LCD Налагоджувальна плата экранчик 3.5" TFT LCD 480x320 резистивный сенсорный экран 
от Waveshare по SPI

я уже роспаял контакты - D:\PROJECTS\electronics\STM32\3.5inch_TFT_Touch_Shield_Code_V2\3.5inch_TFT_Touch_Shield_Code\STM32-1-ANALYZE\ESP-GPIO.jpg
D:\PROJECTS\electronics\STM32\3.5inch_TFT_Touch_Shield_Code_V2\3.5inch_TFT_Touch_Shield_Code\STM32-1-ANALYZE - наш проект будет хранится для начала тут 

ничего пока не делай а будем пошагово разрабытывать инструкцию (без графического фреймворка)

WeActStudio/WeActStudio.MiniSTM32H723

https://github.com/WeActStudio/WeActStudio.MiniSTM32H723

D:\PROJECTS\electronics\STM32\Project1 - проект с маленьким экранчиком на плате  - какие проблемы встречались и как мы их решили
D:\PROJECTS\electronics\STM32\Project1\.settings\claude\sessions\stm32h723_touchgfx_full_context.md - а тут реальное описание пошаговое
как мы настраивали экранчик и выводили данные как мы реали проблемы


я проверил на ардуино все работает
D:\PROJECTS\electronics\STM32\3.5inch_TFT_Touch_Shield_Code_V2\3.5inch_TFT_Touch_Shield_Code\Arduino\LCD_Show
D:\PROJECTS\electronics\STM32\3.5inch_TFT_Touch_Shield_Code_V2\3.5inch_TFT_Touch_Shield_Code\Arduino\LCD_Touch

давай пошагово спроектируем нужные пины STM32CubeMX
опиши каждый шаг как надо делать

потом перейдем к программированию
STM32CubeIDE 2.2.0

я подключил экранчик и он засветился белым через все  - значит питание работает D:\PROJECTS\electronics\STM32\3.5inch_TFT_Touch_Shield_Code_V2\3.5inch_TFT_Touch_Shield_Code\STM32-1-ANALYZE\my_project1.jpg

всю документацию и инструкции сохраняй в D:\PROJECTS\electronics\STM32\3.5inch_TFT_Touch_Shield_Code_V2\3.5inch_TFT_Touch_Shield_Code\STM32-1-ANALYZE\Readme.md
####################################################################################################################################################
Что я нашёл

Проверил три источника: твою распайку (ESP-GPIO.jpg), схему платы WeAct из прошлого проекта (Project1, там уже разобрано, какие пины заняты 
SPI-флешкой/QSPI/SD/камерой/USB/SWD) и официальный рендер платы (STM32H7xx_2.jpg), где физически видно, что PB13/PB14/PB15 выведены на гребёнку. 
Твоя таблица пинов корректна и не пересекается ни с чем занятым на плате — можно смело по ней конфигурировать CubeMX.

Важное на память: в прошлой попытке (проект GFX-WaveShare, тот же git branch) экран не заводился не из-за брака, а из-за асимметричного протокола 
SPI у ILI9486: команда = 1 сырой байт, а параметр = 2 байта (0x00 + значение). Это сейчас не трогаем — только фиксирую в Readme, пригодится на этапе 
программирования.

Пины (SPI2, полный дуплекс — MISO нужен для тача)

┌─────────┬──────┬─────────────┐
│ Сигнал  │ Пин  │    Режим    │
├─────────┼──────┼─────────────┤
│ SCLK    │ PB13 │ SPI2_SCK    │
├─────────┼──────┼─────────────┤
│ MISO    │ PB14 │ SPI2_MISO   │
├─────────┼──────┼─────────────┤
│ MOSI    │ PB15 │ SPI2_MOSI   │
├─────────┼──────┼─────────────┤
│ LCD_CS  │ PC0  │ GPIO_Output │
├─────────┼──────┼─────────────┤
│ LCD_DC  │ PC1  │ GPIO_Output │
├─────────┼──────┼─────────────┤
│ LCD_RST │ PC2  │ GPIO_Output │
├─────────┼──────┼─────────────┤
│ LCD_BL  │ PC3  │ GPIO_Output │
├─────────┼──────┼─────────────┤
│ TP_CS   │ PC4  │ GPIO_Output │
├─────────┼──────┼─────────────┤
│ TP_IRQ  │ PC5  │ GPIO_Input  │
└─────────┴──────┴─────────────┘

Шаги в CubeMX (только Pinout, код пока не генерируем)

1. New Project → MCU Selector → STM32H723VGT6 → Start Project
2. RCC → HSE = Crystal/Ceramic Resonator (на плате реальный кварц 25 МГц)
3. SYS → Debug = Serial Wire
4. Connectivity → SPI2 → Mode = Full-Duplex Master, затем проверить, что CubeMX реально выбрал PB13/14/15 (а не другой альтернативный набор пинов SPI2)
— если нет, кликнуть по каждому пину на картинке чипа и выбрать вручную
5. Клик по PC0, PC1, PC2, PC3 → GPIO_Output
6. Клик по PC4 → GPIO_Output, PC5 → GPIO_Input
7. User Label для каждого: LCD_CS, LCD_DC, LCD_RST, LCD_BL, TP_CS, TP_IRQ — без суффикса _Pin (иначе получится задвоение, как уже было в Project1)
8. PC0-PC4: Output Push Pull, Low/Medium speed; PC5: Input, Pull-up
9. Убедиться, что ни один пин не подсвечен как конфликтующий
10. Save Project as… → в папку STM32-1-ANALYZE. Generate Code пока не нажимать.

Полная версия со всеми деталями и пояснениями сохранена в Readme.md в этой папке.

Сделай эти шаги в CubeMX и покажи скриншот (или просто скажи "готово") — дальше перейдём к Clock Tree и генерации кода под STM32CubeIDE.