/*
 * IOT MORSE SYSTEM (VISUAL MONITOR)
 * Скорость: 150ms (Для ручного ввода)
 * Протокол: [START -.-.-] MSG [END ...-.-]
 */

// --- КОНФИГУРАЦИЯ ---
const int PIN_RX = 2;       // Вход (Прерывание)
const int PIN_TX = 3;       // Выход
const int PIN_SR_DATA = 4;  // 74HC595 Data
const int PIN_SR_LATCH = 5; // 74HC595 Latch
const int PIN_SR_CLOCK = 6; // 74HC595 Clock
const int PIN_BTN = 7;      // Кнопка

// --- ЭКРАН ---
byte digits[] = {
  0b11111010, 0b01100000, 0b11011100, 0b11110100, // 0-3
  0b01100110, 0b10110110, 0b11101110, 0b11100000, // 4-7
  0b11111110, 0b11110110                          // 8-9
};
byte CHAR_BLANK = 0b00000000;
byte CHAR_WAIT  = 0b00000010; // Центральная черта
byte CHAR_ERR   = 0b00000001; // Точка

// --- СЛОВАРЬ ---
struct MorseChar { char letter; const char* code; };

const int DICTIONARY_SIZE = 14;
MorseChar dictionary[DICTIONARY_SIZE] = {
  {'S', "..."},   {'O', "---"},
  {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, 
  {'3', "...--"}, {'4', "....-"}, {'5', "....."}, 
  {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, 
  {'9', "----."},
  {'<', "-.-.-"},  // START (KA)
  {'>', "...-.-"}  // END (SK)
};

// --- ТАЙМИНГИ (150ms) ---
const unsigned long UNIT_TIME = 150; 
const unsigned long DOT_MAX = 250;      // < 250ms = Точка
const unsigned long DASH_MIN = 300;     // > 300ms = Тире
const unsigned long CHAR_TIMEOUT = 600; // Пауза > 600ms = Конец буквы

// --- ПЕРЕМЕННЫЕ ---
// Кнопка
unsigned long lastDebounceTime = 0;
bool lastButtonState = LOW;
bool currentButtonState = LOW;

// Прием (ISR Safe)
volatile unsigned long rxStartTime = 0;
volatile bool rxActive = false;
const int ISR_BUF_SIZE = 50;
volatile char isrBuffer[ISR_BUF_SIZE];
volatile int isrHead = 0;
volatile int isrTail = 0;

// Логика
String incomingSignal = "";
unsigned long lastRxSignalTime = 0;
String txQueue = "";
int txCharIndex = 0;
String txCurrentCode = "";
int txSignalIndex = 0;
unsigned long txNextTime = 0;
bool txLevel = LOW;

// FSM Состояния
enum TxState { TX_IDLE, TX_AUTO_SIGNAL, TX_AUTO_GAP };
TxState txState = TX_IDLE;

enum RxProtocolState { RX_WAIT_START, RX_READ_PAYLOAD };
RxProtocolState rxState = RX_WAIT_START;

// --- ПРЕРЫВАНИЕ (RX) ---
void rxInterrupt() {
  bool pinState = digitalRead(PIN_RX);
  unsigned long now = millis();
  
  if (pinState == HIGH && !rxActive) {
    rxStartTime = now;
    rxActive = true;
  } 
  else if (pinState == LOW && rxActive) {
    unsigned long duration = now - rxStartTime;
    rxActive = false;
    
    char sym = 0;
    if (duration > 20) { // Фильтр помех
      if (duration <= DOT_MAX) sym = '.';
      else if (duration >= DASH_MIN) sym = '-';
    }
    
    if (sym != 0) {
      int next = (isrHead + 1) % ISR_BUF_SIZE;
      if (next != isrTail) {
        isrBuffer[isrHead] = sym;
        isrHead = next;
      }
    }
  }
}

// --- ФУНКЦИИ ---
void showByte(byte data) {
  digitalWrite(PIN_SR_LATCH, LOW);
  shiftOut(PIN_SR_DATA, PIN_SR_CLOCK, LSBFIRST, data);
  digitalWrite(PIN_SR_LATCH, HIGH);
}

char decodeMorse(String code) {
  for (int i = 0; i < DICTIONARY_SIZE; i++) {
    if (String(dictionary[i].code) == code) return dictionary[i].letter;
  }
  return '?';
}

String encodeMorse(char letter) {
  for (int i = 0; i < DICTIONARY_SIZE; i++) {
      if (dictionary[i].letter == letter) return String(dictionary[i].code);
  }
  return "";
}

// Обработка символа после таймаута
void processRxChar(char decodedChar) {
  Serial.print(" -> "); // Разделитель после точек/тире
  
  if (rxState == RX_WAIT_START) {
    if (decodedChar == '<') {
      rxState = RX_READ_PAYLOAD;
      Serial.println("[SESSION OPEN]");
      showByte(CHAR_BLANK);
    } else {
      Serial.println("(Ignored)");
      showByte(CHAR_WAIT);
    }
  } 
  else if (rxState == RX_READ_PAYLOAD) {
    if (decodedChar == '>') {
      rxState = RX_WAIT_START;
      Serial.println("[SESSION CLOSED]");
      showByte(CHAR_WAIT);
    } else {
      Serial.println(decodedChar); // Печать буквы
      
      // Вывод на экран
      byte b = CHAR_ERR;
      if (decodedChar >= '0' && decodedChar <= '9') b = digits[decodedChar - '0'];
      else if (decodedChar == 'S') b = digits[5];
      else if (decodedChar == 'O') b = digits[0];
      showByte(b);
    }
  }
}

void setup() {
  pinMode(PIN_RX, INPUT);
  pinMode(PIN_TX, OUTPUT);
  pinMode(PIN_BTN, INPUT);
  
  pinMode(PIN_SR_DATA, OUTPUT);
  pinMode(PIN_SR_LATCH, OUTPUT);
  pinMode(PIN_SR_CLOCK, OUTPUT);
  
  digitalWrite(PIN_TX, LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_RX), rxInterrupt, CHANGE);
  
  Serial.begin(9600);
  Serial.println("SYSTEM READY");
  showByte(CHAR_WAIT);
}

void loop() {
  unsigned long t = millis();

  // 1. ЧТЕНИЕ ИЗ БУФЕРА (Real-time print)
  while (isrHead != isrTail) {
    char c = isrBuffer[isrTail];
    isrTail = (isrTail + 1) % ISR_BUF_SIZE;
    incomingSignal += c;
    lastRxSignalTime = t;
    Serial.print(c); // Печатаем точку/тире сразу при получении
  }

  // 2. ТАЙМАУТ ПРИЕМА
  if (incomingSignal.length() > 0 && (t - lastRxSignalTime > CHAR_TIMEOUT)) {
    char decoded = decodeMorse(incomingSignal);
    processRxChar(decoded);
    incomingSignal = "";
  }

  // 3. ФОТОРЕЗИСТОР (Raw Input)
  bool btnRaw = !digitalRead(PIN_BTN);
  if (btnRaw != lastButtonState) lastDebounceTime = t;
  if ((t - lastDebounceTime) > 30) currentButtonState = btnRaw;
  lastButtonState = btnRaw;

  // 4. SERIAL ВВОД (Auto Input)
  if (Serial.available() > 0 && txState == TX_IDLE) {
    String input = Serial.readStringUntil('\n');
    input.trim(); input.toUpperCase();
    if (input.length() > 0) {
      txQueue = "<" + input + ">"; // Обертка в протокол
      txState = TX_AUTO_GAP;
      txCharIndex = 0;
      txNextTime = t;
      Serial.print("Auto-TX: "); Serial.println(txQueue);
    }
  }

  // 5. FSM ПЕРЕДАЧИ
  switch (txState) {
    case TX_IDLE:
      digitalWrite(PIN_TX, currentButtonState); // Прямая трансляция кнопки
      break;

    case TX_AUTO_GAP:
      if (t >= txNextTime) {
        if (txCharIndex >= txQueue.length()) {
          txState = TX_IDLE;
        } else {
          char c = txQueue[txCharIndex];
          txCurrentCode = encodeMorse(c);
          txSignalIndex = 0;
          if (txCurrentCode == "") txCharIndex++;
          else txState = TX_AUTO_SIGNAL;
        }
      }
      break;

    case TX_AUTO_SIGNAL:
      if (t >= txNextTime) {
        if (txLevel == LOW) { // Начало импульса
          if (txSignalIndex >= txCurrentCode.length()) { // Конец буквы
            digitalWrite(PIN_TX, LOW);
            txLevel = LOW;
            txNextTime = t + (UNIT_TIME * 3);
            txState = TX_AUTO_GAP;
            txCharIndex++;
          } else { // Точка или Тире
            digitalWrite(PIN_TX, HIGH);
            txLevel = HIGH;
            char s = txCurrentCode[txSignalIndex];
            txNextTime = t + (s == '.' ? UNIT_TIME : (UNIT_TIME * 3));
            txSignalIndex++;
          }
        } else { // Пауза внутри буквы
          digitalWrite(PIN_TX, LOW);
          txLevel = LOW;
          txNextTime = t + UNIT_TIME;
        }
      }
      break;
  }
}
