#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_HX8357.h> // Драйвер для чипов ILI9486 находится здесь

// Точные имена пинов согласно ядру для вашей платы WeAct H723 (через подчеркивание)
#define TFT_SCK   PB_13
#define TFT_MISO  PB_14
#define TFT_MOSI  PB_15

#define TFT_CS    PC_0
#define TFT_DC    PC_1  // LCD_DC
#define TFT_RST   PC_2  // LCD_RST
#define TFT_BL    PC_3  // LCD_BL

// Явно создаем объект для работы со второй аппаратной шиной SPI
SPIClass SPI_2(TFT_MOSI, TFT_MISO, TFT_SCK);

// Инициализируем дисплей с использованием верного класса библиотеки
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // Настраиваем и включаем подсветку дисплея
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); 

  // Жесткий аппаратный сброс экрана перед стартом
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);

  // Запускаем аппаратную шину SPI2
  SPI_2.begin();

  // Ограничиваем скорость до безопасных 8 МГц для старта
  SPI_2.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  // Запуск дисплея в режиме совместимости с ILI9486 (команда HX8357B)
  tft.begin(HX8357B); 
  tft.setRotation(1); // Альбомная ориентация
  
  // Тестовая заливка синим цветом из новой библиотеки
  tft.fillScreen(HX8357_BLUE);
}

void loop() {
  tft.setCursor(20, 20);
  tft.setTextColor(HX8357_WHITE);
  tft.setTextSize(3);
  tft.println("SPI2 Connected!");
  
  tft.setCursor(20, 70);
  tft.setTextColor(HX8357_GREEN);
  tft.setTextSize(2);
  tft.println("WeAct STM32H723 OK");
  delay(1000);
}
