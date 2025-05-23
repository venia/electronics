// Пины для сдвиговых регистров 74HC595
#define DATA_PIN  8   // DS (serial data)
#define LATCH_PIN 9   // ST_CP (latch)
#define CLOCK_PIN 10  // SH_CP (clock)

// Массив для отображения цифр 0-9 без точки (Q7=A, Q6=B, Q5=C, Q4=D, Q3=E, Q2=F, Q1=G, Q0=DP)
const byte digitPatterns[10] = {
  0b00111111, // 0: A–F=1, G,DP=0
  0b00000110, // 1: B,C=1, A,D,E,F,G,DP=0
  0b01011011, // 2: A,B,D,E,G=1, C,F,DP=0
  0b01001111, // 3: A,B,C,D,G=1, E,F,DP=0
  0b01100110, // 4: B,C,F,G=1, A,D,E,DP=0
  0b01101101, // 5: A,C,D,F,G=1, B,E,DP=0
  0b01111101, // 6: A,C,D,E,F,G=1, B,DP=0
  0b00000111, // 7: A,B,C=1, D,E,F,G,DP=0
  0b01111111, // 8: A–G=1, DP=0
  0b01101111  // 9: A,B,C,D,F,G=1, E,DP=0
};

// Массив для отображения цифр 0-9 с точкой (Q7=DP=1)
const byte digitPatternsWithDot[10] = {
  0b00111111 | 0b10000000, // 0: A–F=1, G=0, DP=1
  0b00000110 | 0b10000000, // 1: B,C=1, A,D,E,F,G=0, DP=1
  0b01011011 | 0b10000000, // 2: A,B,D,E,G=1, C,F=0, DP=1
  0b01001111 | 0b10000000, // 3: A,B,C,D,G=1, E,F=0, DP=1
  0b01100110 | 0b10000000, // 4: B,C,F,G=1, A,D,E=0, DP=1
  0b01101101 | 0b10000000, // 5: A,C,D,F,G=1, B,E=0, DP=1
  0b01111101 | 0b10000000, // 6: A,C,D,E,F,G=1, B=0, DP=1
  0b00000111 | 0b10000000, // 7: A,B,C=1, D,E,F,G=0, DP=1
  0b01111111 | 0b10000000, // 8: A–G=1, DP=1
  0b01101111 | 0b10000000  // 9: A,B,C,D,F,G=1, E=0, DP=1
};

// Управление цифрами (низкий уровень включает)
const byte digitControl[4] = {
  0b11111110, // DIGIT1: Q0=0, Q1-Q7=1
  0b11111101, // DIGIT2: Q1=0, Q0,Q2-Q7=1
  0b11111011, // DIGIT3: Q2=0, Q0,Q1,Q3-Q7=1
  0b11110111  // DIGIT4: Q3=0, Q0-Q2,Q4-Q7=1
};

// Выключение всех цифр
const byte allDigitsOff = 0b11111111; // Все Q0-Q7=1

// Переменные для времени
unsigned long lastUpdate = 0;
byte hours = 12, minutes = 0, seconds = 0;

// Переменные для мультиплексирования
unsigned long lastDisplayUpdate = 0;
byte currentDigit = 0;
const unsigned long displayInterval = 4; // Интервал мультиплексирования, мс

void setup() {
  // Настройка пинов
  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
}

void loop() {
  // Обновление времени каждую секунду
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0;
        }
      }
    }
  }

  // Мультиплексирование дисплея
  if (millis() - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = millis();
    displayTime();
  }
}

void displayTime() {
  // Разбиваем время на цифры
  byte digit1 = hours / 10;    // Десятки часов
  byte digit2 = hours % 10;    // Единицы часов
  byte digit3 = minutes / 10;  // Десятки минут
  byte digit4 = minutes % 10;  // Единицы минут

  // Определяем, показывать ли точку (мигание каждую секунду)
  bool showDot = (seconds % 2 == 0); // Точка горит на чётных секундах

  // Показываем одну цифру за раз
  switch (currentDigit) {
    case 0:
      showDigit(0, digit1, false); // DIGIT1, без точки
      break;
    case 1:
      showDigit(1, digit2, showDot); // DIGIT2, с мигающей точкой
      break;
    case 2:
      showDigit(2, digit3, false); // DIGIT3, без точки
      break;
    case 3:
      showDigit(3, digit4, false); // DIGIT4, без точки
      break;
  }

  // Переключаемся на следующую цифру
  currentDigit = (currentDigit + 1) % 4;
}

void showDigit(byte digitIndex, byte number, bool showDot) {
  // Выключить все цифры
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, allDigitsOff); // Второй 74HC595: все выкл
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0x00);         // Первый 74HC595: сегменты выкл
  digitalWrite(LATCH_PIN, HIGH);

  // Включить нужную цифру
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, digitControl[digitIndex]); // Второй 74HC595
  // Выбираем шаблон с точкой или без
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, showDot ? digitPatternsWithDot[number] : digitPatterns[number]); // Первый 74HC595
  digitalWrite(LATCH_PIN, HIGH);
}