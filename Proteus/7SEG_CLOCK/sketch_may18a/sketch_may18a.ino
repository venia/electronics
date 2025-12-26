#include <SPI.h> 
#include <RDA5807.h>
#include <RtcDS1302.h>
#include <Adafruit_BMP085.h>

// Пины для сдвиговых регистров 74HC595
#define LATCH_PIN 9   // STCP (latch), можно выбрать любой пин
// MOSI (пин 11) и SCLK (пин 13) используются аппаратно через SPI
#define SET_HOURS_MINUTES_PIN 12
#define SET_INCREMENT_PIN 10

// Пины для DS1302
#define K_CE_PIN 7  // RST (Chip Enable)
#define K_IO_PIN 6  // DAT (Input/Output)
#define K_SCLK_PIN 5  // CLK (Serial Clock)

ThreeWire myWire(K_IO_PIN, K_SCLK_PIN, K_CE_PIN); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

Adafruit_BMP085 bmp;    // I2C

RDA5807 rx; // Radio

// Определение enum для состояний
enum SystemState {
  NORMAL,
  SET_HOURS,
  SET_MINUTES,
  TEMPERATURE,
  PRESSURE_PA,
  PRESSURE_HG,
  RADIO
};

volatile uint8_t TIMER1_COMPA_COUNTER = 0;

int lastHoursMinutesState = HIGH; // Последнее состояние кнопки
int buttonHoursMinutesState; // Текущее состояние кнопки
unsigned long lastHoursMinutesDebounceTime = 0; // Время последнего изменения состояния

int lastIncrementState = HIGH; 
int buttonIncrementState; 
unsigned long lastIncremenDebounceTime = 0; 

const unsigned long DEBOUNCE_DELAY = 50; // Задержка для устранения дребезга

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
  0b01100110 | 0b10000000, // 4: B,C,F,G=1, declarationsA,D,E=0, DP=1
  0b01101101 | 0b10000000, // 5: A,C,D,F,G=1, B,E=0, DP=1
  0b01111101 | 0b10000000, // 6: A,C,D,E,F,G=1, B=0, DP=1
  0b00000111 | 0b10000000, // 7: A,B,C=1, D,E,F,G=0, DP=1
  0b01111111 | 0b10000000, // 8: A–G=1, DP=1
  0b01101111 | 0b10000000, // 9: A,B,C,D,F,G=1, E=0, DP=1
  0b00000000 | 0b10000000  // OFF: A-G=0, DP=0
};

// Управление цифрами (низкий уровень включает)
const byte digitControl[8] = {
  0b11111110, // DIGIT1: Q0=0, Q1-Q7=1
  0b11111101, // DIGIT2: Q1=0, Q0,Q2-Q7=1
  0b11111011, // DIGIT3: Q2=0, Q0,Q1,Q3-Q7=1
  0b11110111, // DIGIT4: Q3=0, Q0-Q2,Q4-Q7=1
  0b11101111, // DIGIT5
  0b11011111, // DIGIT6
  0b10111111, // DIGIT7
  0b01111111  // DIGIT8
};

const byte imageClockLeftPatterns[8] = {
  0b00111001, // Symbol C 
  0b00111001, // Symbol C
  0b00111001, // Symbol C
  0b00111001, // Symbol C
  
  0b00110001, 
  0b00101001,
  0b00011001,
  0b00111000
};

const byte imageClockRightPatterns[8] = {
  0b00001110,
  0b00001101,
  0b00001011,
  0b00000111,

  0b00001111, // Symbol Э
  0b00001111, // Symbol Э
  0b00001111, // Symbol Э
  0b00001111 // Symbol Э
};

byte currentImageClockDigit = 0;

// Выключение всех цифр
const byte allDigitsOff = 0b11111111; // Все Q0-Q7=1

// Переменные для времени
unsigned long lastUpdateSeconds = 0;
int hours = 12, minutes = 0, seconds = 0;

// Переменные для мультиплексирования
unsigned long lastDisplayUpdate = 0;
byte currentDigit = 0;
const unsigned long MULTIPLEXER_INTERVAL = 2; // Интервал мультиплексирования, мс

// BMP180
// Температура в °C
float temperature;
// Давление в Паскалях
long pressure_pa;
// Давление в мм рт. ст.
float pressure_mmHg;
// Высота над уровнем моря (приближенно, стандартное давление 1013.25 гПа)
float altitude;
// BMP180 flag for enable update
volatile bool flag60sec = false;  // флаг, что прошла минута
// ===========================================================================================[SETUP]==========================================================================================
void setup() {
  Serial.begin(9600);

   while (!Serial);

  // Timer1 configure A & B 
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << CS11) | (1 << CS10); // Предделитель 64
  OCR1A = 62500; // 0.25 с
  TCCR1B |= (1 << WGM12); // Режим CTC
  TIMSK1 |= (1 << OCIE1A); 
  interrupts();

  // Настройка пинов
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(SET_HOURS_MINUTES_PIN, INPUT_PULLUP);
  pinMode(SET_INCREMENT_PIN, INPUT_PULLUP);

  // Инициализация SPI
  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); // 8 МГц, старший бит первый, режим 0

  // Запускаем модуль DS1302
  Rtc.Begin();
  RtcDateTime newTime = RtcDateTime(2025, 6, 5, 16, 44, 0); // Установка времени: 16:44:00, 5 июня 2025
  if (!Rtc.IsDateTimeValid()) {
    Serial.println("RTC lost time");
    Rtc.SetDateTime(newTime);
  }
  if (Rtc.GetIsWriteProtected()) {
    Rtc.SetIsWriteProtected(false);
  }
  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true);
  }

  // Инициализация времени
  DS1302UpdateGlobalHourMinuteSecondTime();
  Serial.println(String("HOURS = ") + String(hours));
  Serial.println(String("MINUTES = ") + String(minutes));
  Serial.println(String("SECONDS = ") + String(seconds));

  if (!bmp.begin()) {
    Serial.println("Not found BMP180!");
    while (1); // зависаем
  }
  
  Serial.println("BMP180 Success!");

  readBMP180Data();
  if (flag60sec) {
    flag60sec = false;
    readBMP180Data();
    DS1302UpdateGlobalHourMinuteSecondTime();
  }

  // Radio
  rx.setup();                   // Инициализация
  rx.setSpace(0);               // Шаг 100 кГц (0 = 100 кГц для Европы/России; 1 = 200 кГц для США)
  rx.setBand(0);                // Диапазон 87–108 МГц (0 = стандартный)
  rx.setFrequency(10100);       // Старт с 87.5 МГц (8750 = 87.5 * 100)
  rx.setVolume(15);             // Громкость 0–15
  rx.setSeekThreshold(30);      // Порог поиска (выше = только сильные станции)
  rx.setMute(false);
}
// ===========================================================================================[SETUP]==========================================================================================

// ============================================================================================[ISR]===========================================================================================
ISR(TIMER1_COMPA_vect) { // 0.25 s
  // Every 0.25 sec
  TIMER1_COMPA_COUNTER++; 

  timer250MillSecondsFunction();
  
  // 0.5 с: каждые 2 прерывания (2 * 0.25 с = 0.5 с)
  if (TIMER1_COMPA_COUNTER % 2 == 0) {
    timer500MillSecondsFunction();
  }
  
  // 10 с: каждые 40 прерываний (40 * 0.25 с = 10 с)
  if (TIMER1_COMPA_COUNTER % 40 == 0) {
    timer10SeccondsFunction();
  }
  
  // 60 с: каждые 240 прерываний (240 * 0.25 с = 60 с)
  if (TIMER1_COMPA_COUNTER >= 240) {
    timer60SeccondsFunction();
    TIMER1_COMPA_COUNTER = 0;
  }
}
// ============================================================================================[ISR]===========================================================================================

// ===========================================================================================[LOOP]==========================================================================================
void loop() {
  if (millis() - lastUpdateSeconds >= 1000) { // Update time for every seconds
    lastUpdateSeconds = millis();
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

  // Мультиплексирование дисплея (приоритет)
  if (millis() - lastDisplayUpdate >= MULTIPLEXER_INTERVAL) {
    lastDisplayUpdate = millis();
    displayModule();
    handleSetButton();
    handleIncrementButton();
  }
}
// ===========================================================================================[LOOP]==========================================================================================

void displayModule() {
  if (currentSetStateEnum == SystemState::RADIO) {
    uint16_t freq = rx.getFrequency();      // Частота в единицах 10 кГц
    float freqMHz = freq / 100.0;

    byte digit1 = freqMHz / 100;
    byte digit2 = ((int)freqMHz % 100) / 10; 
    byte digit3 = ((int)freqMHz % 10); 
    byte digit4 = (int)(freqMHz * 10) % 10;   
    byte digit5 = 10;
    byte digit6 = 10;
    byte digit7 = 0b01110111;
    byte digit8 = 0b01011100;

    // Показываем одну цифру за раз
    switch (currentDigit) {
      case 0:
        showDigit(0, digit1, false); // DIGIT1, без точки
        break;
      case 1:
        showDigit(1, digit2, false); // DIGIT2, с мигающей точкой
        break;
      case 2:
        showDigit(2, digit3, true); // DIGIT3, без точки
        break;
      case 3:
        showDigit(3, digit4, false); // DIGIT4, без точки
        break;
      case 4:
        showDigit(4, digit5, false); // DIGIT5, без точки
        break;
      case 5:
        showDigit(5, digit6, false); // DIGIT6, без точки
        break;
      case 6:
        showSymbol(6, digit7); // IMAGE
        break;
      case 7:
        showSymbol(7, digit8); // IMAGE
        break;
    }

  } else if (currentSetStateEnum == SystemState::TEMPERATURE) {
    byte digit1 = 10;
    byte digit2 = ((int)temperature / 10); 
    byte digit3 = ((int)temperature % 10); 
    byte digit4 = (int)(temperature * 10) % 10;  
    byte digit5 = 10;
    byte digit6 = 10;
    byte digit7 = 0b00000111;
    byte digit8 = 0b00110001;

    // Показываем одну цифру за раз
    switch (currentDigit) {
      case 0:
        showDigit(0, digit1, false); // DIGIT1, без точки
        break;
      case 1:
        showDigit(1, digit2, false); // DIGIT2, с мигающей точкой
        break;
      case 2:
        showDigit(2, digit3, true); // DIGIT3, без точки
        break;
      case 3:
        showDigit(3, digit4, false); // DIGIT4, без точки
        break;
      case 4:
        showDigit(4, digit5, false); // DIGIT5, без точки
        break;
      case 5:
        showDigit(5, digit6, false); // DIGIT6, без точки
        break;
      case 6:
        showSymbol(6, digit7); // IMAGE
        break;
      case 7:
        showSymbol(7, digit8); // IMAGE
        break;
    }
  } else if (currentSetStateEnum == SystemState::PRESSURE_PA) {
    // Serial.println(String("PRESSURE = ") + String(pressure_pa));
    byte digit1 = pressure_pa / 100000;
    byte digit2 = (pressure_pa % 100000) / 10000; 
    byte digit3 = (pressure_pa % 10000) / 1000; 
    byte digit4 = (pressure_pa % 1000) / 100;  
    byte digit5 = (pressure_pa % 100) / 10;
    byte digit6 = (pressure_pa % 10);
    byte digit7 = 0b01110011;
    byte digit8 = 0b01011100;

    // Показываем одну цифру за раз
    switch (currentDigit) {
      case 0:
        showDigit(0, digit1, false); // DIGIT1, без точки
        break;
      case 1:
        showDigit(1, digit2, false); // DIGIT2, с мигающей точкой
        break;
      case 2:
        showDigit(2, digit3, false); // DIGIT3, без точки
        break;
      case 3:
        showDigit(3, digit4, false); // DIGIT4, без точки
        break;
      case 4:
        showDigit(4, digit5, false); // DIGIT5, без точки
        break;
      case 5:
        showDigit(5, digit6, false); // DIGIT6, без точки
        break;
      case 6:
        showSymbol(6, digit7); // IMAGE
        break;
      case 7:
        showSymbol(7, digit8); // IMAGE
        break;
    }
  } else if (currentSetStateEnum == SystemState::PRESSURE_HG) {
    byte digit1 = pressure_mmHg / 100;
    byte digit2 = ((int)pressure_mmHg % 100) / 10; 
    byte digit3 = ((int)pressure_mmHg % 10); 
    byte digit4 = (int)(pressure_mmHg * 10) % 10;   
    byte digit5 = 10;
    byte digit6 = 10;
    byte digit7 = 0b01110110;
    byte digit8 = 0b00111101;

    // Показываем одну цифру за раз
    switch (currentDigit) {
      case 0:
        showDigit(0, digit1, false); // DIGIT1, без точки
        break;
      case 1:
        showDigit(1, digit2, false); // DIGIT2, с мигающей точкой
        break;
      case 2:
        showDigit(2, digit3, true); // DIGIT3, без точки
        break;
      case 3:
        showDigit(3, digit4, false); // DIGIT4, без точки
        break;
      case 4:
        showDigit(4, digit5, false); // DIGIT5, без точки
        break;
      case 5:
        showDigit(5, digit6, false); // DIGIT6, без точки
        break;
      case 6:
        showSymbol(6, digit7); // IMAGE
        break;
      case 7:
        showSymbol(7, digit8); // IMAGE
        break;
    }
  } else {
    // Разбиваем время на цифры
    byte digit1 = (currentSetStateEnum == SystemState::SET_HOURS && trueFalseState) ? 10 : (hours / 10);    // Десятки часов
    byte digit2 = (currentSetStateEnum == SystemState::SET_HOURS && trueFalseState) ? 10 : (hours % 10);    // Единицы часов
    byte digit3 = (currentSetStateEnum == SystemState::SET_MINUTES && trueFalseState) ? 10 : (minutes / 10);  // Десятки минут
    byte digit4 = (currentSetStateEnum == SystemState::SET_MINUTES && trueFalseState) ? 10 : (minutes % 10);  // Единицы минут
    byte digit5 = (seconds / 10); // Десятки секунд
    byte digit6 = (seconds % 10); // Единицы секунд

    byte digit7 = imageClockLeftPatterns[currentImageClockDigit];
    byte digit8 = imageClockRightPatterns[currentImageClockDigit];

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
      case 4:
        showDigit(4, digit5, false); // DIGIT5, без точки
        break;
      case 5:
        showDigit(5, digit6, false); // DIGIT6, без точки
        break;
      case 6:
        showSymbol(6, digit7); // IMAGE
        break;
      case 7:
        showSymbol(7, digit8); // IMAGE
        break;
    }

    // // Переключаемся на следующую цифру
    // currentDigit = (currentDigit + 1) % 8;
  }

  // Переключаемся на следующую цифру
  currentDigit = (currentDigit + 1) % 8;
}

void showDigit(byte digitIndex, byte number, bool showDot) {
  // Выключить все цифры
  digitalWrite(LATCH_PIN, LOW);
  SPI.transfer(allDigitsOff); // Второй 74HC595: все выкл
  SPI.transfer(0x00);        // Первый 74HC595: сегменты выкл
  digitalWrite(LATCH_PIN, HIGH);

  // Включить нужную цифру
  digitalWrite(LATCH_PIN, LOW);
  SPI.transfer(digitControl[digitIndex]); // Второй 74HC595: выбор цифры
  SPI.transfer(showDot ? digitPatternsWithDot[number] : digitPatterns[number]); // Первый 74HC595: сегменты
  digitalWrite(LATCH_PIN, HIGH);
}

void showSymbol(byte digitIndex, byte number) {
  // Выключить все цифры
  digitalWrite(LATCH_PIN, LOW);
  SPI.transfer(allDigitsOff); // Второй 74HC595: все выкл
  SPI.transfer(0x00);        // Первый 74HC595: сегменты выкл
  digitalWrite(LATCH_PIN, HIGH);

  // Включить нужную цифру
  digitalWrite(LATCH_PIN, LOW);
  SPI.transfer(digitControl[digitIndex]); // Второй 74HC595: выбор цифры
  SPI.transfer(number); // Первый 74HC595: сегменты
  digitalWrite(LATCH_PIN, HIGH);
}

void handleSetButton() {
  int reading = digitalRead(SET_HOURS_MINUTES_PIN);

  if (reading != lastHoursMinutesState) {
    lastHoursMinutesDebounceTime = millis();
  }

  if (millis() - lastHoursMinutesDebounceTime >= DEBOUNCE_DELAY) {
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

  if (millis() - lastIncremenDebounceTime >= DEBOUNCE_DELAY) {
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
  RtcDateTime now = Rtc.GetDateTime();
  if (currentSetStateEnum == SystemState::SET_HOURS) {
    hours = now.Hour();
    hours++;
    if (hours >= 24) {
      hours = 0;
    }
    RtcDateTime newTime = RtcDateTime(now.Year(), now.Month(), now.Day(), hours, now.Minute(), now.Second());
    Rtc.SetDateTime(newTime);
  } else if (currentSetStateEnum == SystemState::SET_MINUTES) {
    minutes = now.Minute();
    minutes++;
    if (minutes >= 60) {
      minutes = 0;
    }
    RtcDateTime newTime = RtcDateTime(now.Year(), now.Month(), now.Day(), now.Hour(), minutes, now.Second());
    Rtc.SetDateTime(newTime);
  } else if (currentSetStateEnum == SystemState::RADIO) {
    rx.seek(RDA_SEEK_WRAP, RDA_SEEK_UP);  // Поиск вверх с зацикливанием (RDA_SEEK_WRAP = 0, RDA_SEEK_UP = 1)
  }
}

void nextState() {
  switch (currentSetStateEnum) {
    case SystemState::NORMAL:
      currentSetStateEnum = SystemState::SET_HOURS;
      break;
    case SystemState::SET_HOURS:
      currentSetStateEnum = SystemState::SET_MINUTES;
      break;
    case SystemState::SET_MINUTES:
      currentSetStateEnum = SystemState::TEMPERATURE;
      break;
    case SystemState::TEMPERATURE:
      currentSetStateEnum = SystemState::PRESSURE_PA;
      break;
    case SystemState::PRESSURE_PA:
      currentSetStateEnum = SystemState::PRESSURE_HG;
      break;
    case SystemState::PRESSURE_HG:
      currentSetStateEnum = SystemState::RADIO;
      break;
    case SystemState::RADIO:
      currentSetStateEnum = SystemState::NORMAL;
      break;
  }
}

void incrementCurrentImageClockDigit() {
  currentImageClockDigit++;
  if (currentImageClockDigit >= 8) {
    currentImageClockDigit = 0;
  }
}

void DS1302UpdateGlobalHourMinuteSecondTime() {
  RtcDateTime now = Rtc.GetDateTime();
  hours = now.Hour();
  minutes = now.Minute();
  seconds = now.Second();
}

void timer250MillSecondsFunction() {
  trueFalseState = !trueFalseState; // Переключаем состояние
  incrementCurrentImageClockDigit(); // Поворачиваем рисунок часов
}

void timer10SeccondsFunction() {
  
}

void timer500MillSecondsFunction() {
  
}

void timer60SeccondsFunction() {
  flag60sec = true;
}

void readBMP180Data() {
  temperature = bmp.readTemperature();
  pressure_pa = bmp.readPressure();
  pressure_mmHg = pressure_pa / 133.322f;
  altitude = bmp.readAltitude(101325);
}