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

enum state_t {
  STATE_READY,       // Game hasn't started
  STATE_RUN,         // Game running
  STATE_READ_INIT,
  STATE_READ_INFO
};

state_t state;
byte pos;
bool interrupt_called;

struct interrupt_debug_t {
  byte recv; 
  void init() {
    recv = -1;
  }

  void print_interrupt() {
    Serial.println("Received SPI byte ");
    Serial.println(recv, BIN);
  }
} int_debug;


void setup (void) {
  Serial.begin(9600);
  Serial.println("MODULE v0.01 alpha");

  // Don't grab MISO until selectesd
  pinMode(MISO, OUTPUT); //todo input

  // turn on SPI in slave mode
  SPCR |= _BV(SPE);

  // turn on interrupts
  SPCR |= _BV(SPIE);

  strikes = 0;
  pos = 0;
  state = STATE_READY;
  interrupt_called = false;
}

inline void update_spdr() {
  SPDR |= strikes;
}

// SPI interrupt routine
// Serial communication can mess up interrupts, so store debug info elsewhere
ISR (SPI_STC_vect) {
  byte c = SPDR;
  int_debug.recv = c;
  if (state == STATE_READY) {
    if (c == CMD_PING) {
      SPDR = RSP_READY;
    }
    if (c == CMD_INIT) {
      state = STATE_READ_INIT;
    }
  }
  else if (state == STATE_READ_INIT){
    ((char[])&game_rand)[pos] = c;
    ++pos;
    if (pos == sizeof(game_rand_t)) {
      pos = 0;
      state = STATE_RUN;
      game_rand.print_rand();
    }
  }
  
  interrupt_called = true;
}

void loop (void) {
  if (interrupt_called) {
    int_debug.print_interrupt();
    int_debug.init();
    interrupt_called = false;
  }
}
