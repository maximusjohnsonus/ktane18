#include <Wire.h>
#include "pins_arduino.h"

#define SN_LEN 6
#define MODEL_LEN 4
#define NUM_MODULES 1

// SPI Commands
const byte PING = 0x11;
const byte START = 0xFF;
const byte TX_RAND = 0x22;
const byte TIME = 0x44;
const byte STATUS = 0x80;

// SPI responses
const byte READY = 0x30;
const byte ACTIVE = 0x60;
const byte SOLVED = 0xC0;
const byte STRIKE_MASK = 0x03;

// All the random components of this game
struct game_rand_t {
  char sn[SN_LEN];
  char model[MODEL_LEN];
  byte indicators;

  void print_rand() {
    Serial.write("SN:");
    Serial.write(sn, SN_LEN);
    Serial.write(", Model:");
    Serial.write(model, MODEL_LEN);
    Serial.write(", IND:");
    Serial.println((int)indicators, BIN);
  }
};
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

void get_miso(){
  if(digitalRead(SS) == LOW){
    pinMode(MISO, OUTPUT);
  } else {
    pinMode(MISO, INPUT);
  }
}
void setup (void) {
  Serial.begin(9600);

  // Don't grab MISO until selectesd
  pinMode(MISO, OUTPUT); //todo input
  // TODO: ISR to set MISO to OUTPUT when SS goes LOW
  //attachInterrupt(digitalPinToInterrupt(ss_pin), get_miso, CHANGE);

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
      } else if (state == RUN_S){
        // Something with time
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
