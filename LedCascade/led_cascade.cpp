const uint8_t ledPins[6] = {2, 3, 4, 5, 6, 7};
const uint8_t LED_PERIODS[6] = {2, 4, 6, 8, 10, 12};
volatile uint8_t counters[6] = {0, 0, 0, 0, 0, 0};
volatile uint8_t ledStates[6] = {1, 1, 1, 1, 1, 1};

void setup() {
  DDRD |= (1 << PD2);
  DDRD |= (1 << PD3);
  DDRD |= (1 << PD4);
  DDRD |= (1 << PD5);
  DDRD |= (1 << PD6);
  DDRD |= (1 << PD7);
  
  PORTD |= (1 << PD2);
  PORTD |= (1 << PD3);
  PORTD |= (1 << PD4);
  PORTD |= (1 << PD5);
  PORTD |= (1 << PD6);
  PORTD |= (1 << PD7);
  
  cli();
  
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);
  OCR2A = 155;
  TIMSK2 |= (1 << OCIE2A);
  
  sei();
}

ISR(TIMER2_COMPA_vect) {
  for (uint8_t i = 0; i < 6; i++) {
    counters[i]++;
    
    if (counters[i] >= LED_PERIODS[i]) {
      counters[i] = 0;
      ledStates[i] ^= 1;
      
      if (ledStates[i]) {
        PORTD |= (1 << (PD2 + i));
      } else {
        PORTD &= ~(1 << (PD2 + i));
      }
    }
  }
}

void loop() {
}