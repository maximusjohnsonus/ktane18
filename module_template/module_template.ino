#include <Wire.h>
#include "pins_arduino.h"

#define SN_LEN 6
#define MODEL_LEN 4
#define NUM_MODULES 1

// SPI Commands
const byte PING = 0x00;
const byte START = 0xFF;
const byte TX_RAND = 0xF0;
const byte TIME = 0x30;
const byte STRIKE = 0x50;

// SPI responses
const byte READY = 0xC0;
volatile int rand_idx;

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

enum state_t {READY_S, RX_RAND_S, RUN_S};
state_t state;


void setup (void) {
  Serial.begin(9600);

  // Don't grab MISO until selectesd
  pinMode(MISO, OUTPUT); //todo input
  pinMode(7, OUTPUT);

  // turn on SPI in slave mode
  SPCR |= _BV(SPE);

  // turn on interrupts
  SPCR |= _BV(SPIE);

  rand_idx = 0;
  state = READY_S;
  SPDR = READY;
}


// SPI interrupt routine
ISR (SPI_STC_vect) {
  byte c = SPDR;
  SPDR = READY;

  if (state == RX_RAND_S) {
    ((byte *)&game_rand)[rand_idx] = c;
    rand_idx++;
    if (rand_idx == sizeof(game_rand_t)) {
      state = READY_S;
    }
  } else if (c == TX_RAND) {
    state = RX_RAND_S;
    rand_idx = 0;
  } else if (c == START) {
    state = RUN_S;
  }
}

void loop (void) {
  if (state == READY_S) {
    if (rand_idx == sizeof(game_rand)) {
      game_rand.print_rand();
      rand_idx = 0;
    } else {
      Serial.print(".");
      delay(1000);
    }
  } else if (state == RX_RAND_S) {
    Serial.print("Waiting for rand... ");
    Serial.println(rand_idx);
    delay(1000);
  } else if (state == RUN_S) {
    Serial.println("Running");
    delay(1000);
  }/*else if (state == BLINK) {
  }
    while (1) {
      digitalWrite(7, HIGH);
      delay(1000);
      digitalWrite(7, LOW);
      delay(1000);
    }
  }*/
}
