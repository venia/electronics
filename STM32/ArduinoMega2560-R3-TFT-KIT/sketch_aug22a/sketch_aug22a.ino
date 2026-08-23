#include <UTFT.h>

// Подключаем встроенные шрифты библиотеки UTFT
extern uint8_t SmallFont[];
extern uint8_t BigFont[];

// ВАЖНО: Модель 'ILI9481' на 16-битном шилде Mega2560 
// жестко инициализируется через встроенные пины 38, 39, 40, 41.
UTFT myGLCD(ILI9481, 38, 39, 40, 41); 

void setup() {
  // Инициализация дисплея в альбомном (горизонтальном) режиме
  myGLCD.InitLCD(LANDSCAPE);
  
  // Очищаем экран (заливаем черным цветом)
  myGLCD.clrScr();
}

void loop() {
  // --- 1. ВЫВОД ТЕКСТА ---
  myGLCD.setFont(BigFont);
  
  // Ярко-красный текст (R, G, B) от 0 до 255
  myGLCD.setColor(255, 0, 0); 
  myGLCD.print("HELLO ARDUINO MEGA!", CENTER, 30); 

  // Зеленый текст поменьше
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("HUGA 3.2 TFT (ILI9481) WORK!", CENTER, 70);


  // --- 2. РИСОВАНИЕ ГЕОМЕТРИЧЕСКИХ ФИГУР ---

  // Синяя горизонтальная линия: drawLine(x1, y1, x2, y2)
  myGLCD.setColor(0, 0, 255); 
  myGLCD.drawLine(20, 110, 460, 110);

  // Желтая пустая рамка прямоугольника: drawRect(x1, y1, x2, y2)
  myGLCD.setColor(255, 255, 0); 
  myGLCD.drawRect(40, 140, 160, 220);

  // Сплошной фиолетовый прямоугольник: fillRect(x1, y1, x2, y2)
  myGLCD.setColor(200, 0, 200); 
  myGLCD.fillRect(200, 140, 320, 220);

  // Бирюзовый круг (контур): drawCircle(x, y, radius)
  myGLCD.setColor(0, 255, 255); 
  myGLCD.drawCircle(90, 270, 30);

  // Оранжевый закрашенный круг: fillCircle(x, y, radius)
  myGLCD.setColor(255, 128, 0); 
  myGLCD.fillCircle(260, 270, 30);

  // Держим картинку на экране 10 секунд
  delay(10000);
  
  // Перезапуск цикла с очисткой
  myGLCD.clrScr();
  delay(500);
}
