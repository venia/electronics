// Пины для сдвиговых регистров 74HC595
#define DATA_PIN  8   // DS (serial data)
#define LATCH_PIN 9   // ST_CP (latch)
#define CLOCK_PIN 10  // SH_CP (clock)

#define SET_HOURS_MINUTES_PIN 7
#define SET_INCREMENT_PIN 6

// Определение enum для состояний
enum SystemState {
  NORMAL,
  SET_HOURS,
  SET_MINUTES
};

int lastHoursMinutesState = HIGH; // Последнее состояние кнопки
int buttonHoursMinutesState; // Текущее состояние кнопки
unsigned long lastHoursMinutesDebounceTime = 0; // Время последнего изменения состояния
const unsigned long debounceDelay = 50; // Задержка для устранения дребезга

int lastIncrementState = HIGH; 
int buttonIncrementState; 
unsigned long lastIncremenDebounceTime = 0; 

SystemState currentSetStateEnum = SystemState::NORMAL;


unsigned long lastTrueFalseToggleTime = 0; 
const unsigned long trueFalseInterval = 500; 
bool trueFalseState = false; 

// Массив для отображения цифр 0-9 без точки (Q7=A, Q6=B, Q5=C, Q4=D, Q3=E, Q2=F, Q1=G, Q0=DP)
const byte digitPatterns[11] = {
  0b00111111, // 0: A–F=1, G,DP=0
  0b00000110, // 1: B,C=1, A,D,E,F,G,DP=0
  0b01011011, // 2: A,B,D,E,G=1, C,F,DP=0
  0b01001111, // 3: A,B,C,D,G=1, E,F,DP=0
  0b01100110, // 4: B,C,F,G=1, A,D,E,DP=0
  0b01101101, // 5: A,C,D,F,G=1, B,E,DP=0
  0b01111101, // 6: A,C,D,E,F,G=1, B,DP=0
  0b00000111, // 7: A,B,C=1, D,E,F,G,DP=0
  0b01111111, // 8: A–G=1, DP=0
  0b01101111, // 9: A,B,C,D,F,G=1, E,DP=0
  0b00000000  // OFF: A-G=0, DP=0
};

// Массив для отображения цифр 0-9 с точкой (Q7=DP=1)
const byte digitPatternsWithDot[11] = {
  0b00111111 | 0b10000000, // 0: A–F=1, G=0, DP=1
  0b00000110 | 0b10000000, // 1: B,C=1, A,D,E,F,G=0, DP=1
  0b01011011 | 0b10000000, // 2: A,B,D,E,G=1, C,F=0, DP=1
  0b01001111 | 0b10000000, // 3: A,B,C,D,G=1, E,F=0, DP=1
  0b01100110 | 0b10000000, // 4: B,C,F,G=1, A,D,E=0, DP=1
  0b01101101 | 0b10000000, // 5: A,C,D,F,G=1, B,E=0, DP=1
  0b01111101 | 0b10000000, // 6: A,C,D,E,F,G=1, B=0, DP=1
  0b00000111 | 0b10000000, // 7: A,B,C=1, D,E,F,G=0, DP=1
  0b01111111 | 0b10000000, // 8: A–G=1, DP=1
  0b01101111 | 0b10000000,  // 9: A,B,C,D,F,G=1, E=0, DP=1
  0b00000000 | 0b10000000  // OFF: A-G=0, DP=0
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
  pinMode(SET_HOURS_MINUTES_PIN, INPUT_PULLUP);
  pinMode(SET_INCREMENT_PIN, INPUT_PULLUP);

  Serial.begin(9600);
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

  handleSetButton();
  handle500TrueFalse();
  handleIncrementButton();
}

void displayTime() {
  // Разбиваем время на цифры
  byte digit1 = (currentSetStateEnum == SystemState::SET_HOURS && trueFalseState) ? 10 : (hours / 10);    // Десятки часов
  byte digit2 = (currentSetStateEnum == SystemState::SET_HOURS && trueFalseState) ? 10 : (hours % 10);    // Единицы часов
  byte digit3 = (currentSetStateEnum == SystemState::SET_MINUTES && trueFalseState) ? 10 : (minutes / 10);  // Десятки минут
  byte digit4 = (currentSetStateEnum == SystemState::SET_MINUTES && trueFalseState) ? 10 : (minutes % 10);  // Единицы минут

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

void handleSetButton() {
  int reading = digitalRead(SET_HOURS_MINUTES_PIN);

  if (reading != lastHoursMinutesState) {
    lastHoursMinutesDebounceTime = millis();
  }

  if (millis() - lastHoursMinutesDebounceTime >= debounceDelay) {
    if (reading != buttonHoursMinutesState) {
      buttonHoursMinutesState = reading;
      if (buttonHoursMinutesState == LOW) {
        nextState();
        Serial.println("Click!");
      }
    }
  }

  lastHoursMinutesState = reading;
}

void handleIncrementButton() {
  int reading = digitalRead(SET_INCREMENT_PIN);

  if (reading != lastIncrementState) {
    lastIncremenDebounceTime = millis();
  }

  if (millis() - lastIncremenDebounceTime >= debounceDelay) {
    if (reading != buttonIncrementState) {
      buttonIncrementState = reading;
      if (buttonIncrementState == LOW) {
        incrementHoursOrMinutes();
        Serial.println("Click!");
      }
    }
  }

  lastIncrementState = reading;
}

void incrementHoursOrMinutes() {
  if(currentSetStateEnum == SystemState::SET_HOURS) {
    hours++;
    if (hours >= 24) {
      hours = 0;
    }
  } else if (currentSetStateEnum == SystemState::SET_MINUTES) {
    minutes++;
    if (minutes >= 60) {
      minutes = 0;
    }
  }
}

// Функция для переключения на следующее состояние
void nextState() {
  switch (currentSetStateEnum) {
    case SystemState::NORMAL:
      currentSetStateEnum = SystemState::SET_HOURS;
      break;
    case SystemState::SET_HOURS:
      currentSetStateEnum = SystemState::SET_MINUTES;
      break;
    case SystemState::SET_MINUTES:
      currentSetStateEnum = SystemState::NORMAL;
      break;
  }
}

void handle500TrueFalse() {
  if (millis() - lastTrueFalseToggleTime >= trueFalseInterval) {
    trueFalseState = !trueFalseState; // Переключаем состояние 
    lastTrueFalseToggleTime = millis(); // Обновляем время
  }
}