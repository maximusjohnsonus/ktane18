// Uno pins 
// MOSI: 11 or ICSP-4
// MISO: 12 or ICSP-1
// SCK:  13 or ICSP-3
// SS (slave): 10 

#include <Wire.h>
#include "pins_arduino.h"
#include "ktane.h"

game_rand_t game_rand;
volatile int rand_idx;

// How many strikes the module has had
byte strikes;

// Status to send to head node
byte my_status; // Arduino has the worst reserved keywords

// Time left on the timer (in ms)
unsigned long millis_left;
volatile int time_idx;

enum state_t {READY_S, RX_RAND_S, RX_TIME_S, RUN_S};
state_t state;
bool new_state = false;

void setup (void) {
  Serial.begin(9600);
  Serial.println("MODULE v0.01 alpha");

  // Don't grab MISO until selectesd
  pinMode(MISO, OUTPUT); //todo input
  pinMode(7, OUTPUT);

  // turn on SPI in slave mode
  SPCR |= _BV(SPE);

  // turn on interrupts
  SPCR |= _BV(SPIE);
}

inline void update_spdr() {
  SPDR = my_status | strikes;
}

// SPI interrupt routine
ISR (SPI_STC_vect) {
  byte c = SPDR;
  update_spdr();

  if (state == RX_RAND_S) {
    ((byte *)&game_rand)[rand_idx] = c;
    rand_idx++;
    if (rand_idx == sizeof(game_rand_t)) {
      state = READY_S;
      new_state = true;
    }
  } else if (state == RX_TIME_S) {
    ((byte *)&millis_left)[time_idx] = c;
    time_idx++;
    if (time_idx == sizeof(long)) {
      state = RUN_S;
      new_state = true;
    }
  } else if (c == TX_RAND) {
    state = RX_RAND_S;
    new_state = true;
    rand_idx = 0;
  } else if (c == TIME) {
    state = RX_TIME_S;
    new_state = true;
    time_idx = 0;
  }
}

void loop (void) {
  state = READY_S;
  my_status = READY;
  strikes = 0;
  update_spdr();

  while (1) {
    // On a state change
    if (new_state) {
      if (state == READY_S) {
        game_rand.print_rand();
      } /* else if (state == BLINK) {
        while (1) {
          digitalWrite(7, HIGH);
          delay(1000);
          digitalWrite(7, LOW);
          delay(1000);
        }
      } */
    }

    // Always
    if (state == RUN_S) {
      if (Serial.available() and Serial.read() == 'x') {
        Serial.println("Striking");
        strikes++;
        update_spdr();
      }
    }
  }
}
