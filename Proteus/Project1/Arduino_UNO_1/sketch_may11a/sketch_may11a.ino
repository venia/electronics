// Пины Arduino для подключения к 74HC595
int dataPin = 12;   // DS (Serial Data) - Pin 14 на 74HC595
int latchPin = 8;   // ST_CP (Storage Clock) - Pin 12 на 74HC595
int clockPin = 11;  // SH_CP (Shift Clock) - Pin 11 на 74HC595

void setup() {
  // Настройка пинов как выходов
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
}

void loop() {
  // Бегущие огни: один светодиод загорается по очереди
  for (int i = 0; i < 16; i++) { // 16 светодиодов (2 регистра по 8 выходов)
    // Сбрасываем все биты
    uint16_t pattern = 0;
    // Устанавливаем 1 в нужной позиции
    pattern = (1 << i);
    
    // Отправляем данные в регистры
    digitalWrite(latchPin, LOW); // Открываем защёлку
    // Отправляем 16 бит (2 байта)
    shiftOut(dataPin, clockPin, MSBFIRST, (pattern >> 8)); // Старший байт
    shiftOut(dataPin, clockPin, MSBFIRST, pattern);        // Младший байт
    digitalWrite(latchPin, HIGH); // Закрываем защёлку
    
    delay(100); // Задержка для видимости эффекта
  }
}