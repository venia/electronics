// Проверка экрана Waveshare 3.5" (ILI9486 + 74HC4094) на Arduino Uno через
// официальную библиотеку "Waveshare ILI9486" (M Hotchin) — в обход всего
// нашего STM32-кода. Цель: доказать, жива ли сама панель, или дело точно
// не в железе экрана.
//
// Перед заливкой: переключатель "SPI Config" на плате экрана должен стоять
// в положении D13/D12/D11 (это распиновка Uno, которую использует
// библиотека через аппаратный SPI).

#include <Adafruit_GFX.h>
#include <Waveshare_ILI9486.h>

Waveshare_ILI9486 tft;

void setup() {
  tft.begin();          // инициализация ILI9486 + подсветка на полную яркость
  tft.setRotation(1);   // альбомная ориентация 480x320, как и в нашем STM32-проекте
}

void loop() {
  // 1) Сплошная заливка красным/синим — тот же тест, что мы делали на STM32
  tft.fillScreen(0xF800); // красный (RGB565)
  delay(1500);
  tft.fillScreen(0x001F); // синий (RGB565)
  delay(1500);

  // 2) Простая графика через Adafruit_GFX (прямоугольники, текст) —
  // если библиотека рисует это корректно, значит и адресация GRAM,
  // и шрифты, и весь низкоуровневый протокол через 74HC4094 у неё рабочие
  tft.fillScreen(0x0000); // чёрный фон
  tft.fillRect(20, 20, 150, 100, 0x07E0);   // зелёный прямоугольник
  tft.drawRect(200, 20, 150, 100, 0xFFFF);  // белая рамка
  tft.setTextColor(0xFFFF);
  tft.setTextSize(3);
  tft.setCursor(20, 150);
  tft.print("WAVESHARE OK");
  delay(3000);

  // 3) Инверсия цвета — тот же тест, что и на STM32 (Display Inversion ON/OFF)
  tft.invertDisplay(true);
  delay(1500);
  tft.invertDisplay(false);
  delay(1500);
}
