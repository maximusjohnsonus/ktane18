// Mega pins
// MOSI: 51 or ICSP-4
// MISO: 50 or ICSP-1
// SCK:  52 or ICSP-3
// SS (slave): 10 



#include <SPI.h>
#include <Wire.h>
#include "pins_arduino.h"

#define SN_LEN 6
#define MODEL_LEN 4
#define NUM_MODULES 1

// SPI Commands
const byte PING_ = 0x11;
const byte TX_RAND = 0x22;
const byte TIME = 0x44; // First TIME cmd will start each module
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
    Serial.write((unsigned char*)sn, SN_LEN);
    Serial.write(", Model:");
    Serial.write((unsigned char*)model, MODEL_LEN);
    Serial.write(", IND:");
    Serial.println((int)indicators, BIN);
  }
};
game_rand_t game_rand;

// How many strikes the game has had
byte strikes;

// Entire time of the game
unsigned long game_time;

// Pins
// Pins for SS of each slave
int slave_pins[NUM_MODULES] = {10};

void setup (void) {
  // Put SCK, MOSI, SS pins into output mode
  // also put SCK, MOSI into LOW state, and SS into HIGH state.
  // Then put SPI hardware into Master mode and turn SPI on
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);

  Serial.begin(9600);
  Serial.println("HEAD v0.01 alpha");
}

void gen_rand() {
  // TODO: seed random better (repeated readings? low-order bits?)
  randomSeed(analogRead(A0));

  for (int i = 0; i < SN_LEN; i++) {
    char c;
    if (random(2) == 0) {
      // Do a letter
      c = random('A', 'Z' + 1);
    } else {
      // Do a number
      c = random('0', '9' + 1);
    }
    game_rand.sn[i] = c;
  }
  for (int i = 0; i < MODEL_LEN; i++) {
    char c;
    if (random(2) == 0) {
      // Do a letter
      c = random('A', 'Z' + 1);
    } else {
      // Do a number
      c = random('0', '9' + 1);
    }
    game_rand.model[i] = c;
  }
  game_rand.indicators = random(256);

  game_rand.print_rand();
}

void tx_rand(int slave_idx) {
  // Copy rand data to buffer so as not to mutilate it
  game_rand_t out_buf;
  memcpy(&out_buf, &game_rand, sizeof(game_rand_t));

  digitalWrite(slave_pins[slave_idx], LOW);
  byte resp = SPI.transfer(TX_RAND);
  SPI.transfer(&out_buf, sizeof(game_rand_t));
  digitalWrite(slave_pins[slave_idx], HIGH);
  game_rand.print_rand();
}

void loop (void) {
  bool slave_on[NUM_MODULES] = {false};
  byte num_strikes[NUM_MODULES] = {0};

  gen_rand();
  strikes = 0;
  game_time = 300000; // 5 minutes

  Serial.println("Establishing connections");
  // Try to establish connection with each module a few times a second
  while (!Serial.available() or Serial.read() != 's') {
    delay(100);

    for (int i = 0; i < NUM_MODULES; i++) {
      digitalWrite(slave_pins[i], LOW);
      byte resp = SPI.transfer(PING_);
      digitalWrite(slave_pins[i], HIGH);

      if (slave_on[i] != (resp == READY)) {
        Serial.print("Slave ");
        Serial.print(i);
        Serial.println(resp == READY ? " connected" : " disconnected");
      }
      if (!slave_on[i] && (resp == READY)) {
        Serial.print("Transmitting game_rand to ");
        Serial.println(i);
        // Newly connected, need to send randomization data
        tx_rand(i);
        Serial.println("Transmitted");
      }
      slave_on[i] = (resp == READY);
    }
  }

  Serial.println("Starting");
  unsigned long start_time = millis();
  // Time left in the game
  unsigned long millis_left = start_time;

  while (1) {
    delay(100);

    // Update each module's timer and check their status
    for (int i = 0; i < NUM_MODULES; i++) {
      digitalWrite(slave_pins[i], LOW);
      millis_left = game_time - (millis() - start_time);
      byte resp = SPI.transfer(TIME);
      SPI.transfer(&millis_left, sizeof(long));
      digitalWrite(slave_pins[i], HIGH);

      byte mod_strikes = resp & STRIKE_MASK;
      if (mod_strikes < num_strikes[i]) {
        // Strikes decreased, error or mistransmission
        Serial.print("Error: strikes decreased for module ");
        Serial.println(i);
      } else if (mod_strikes > num_strikes[i]) {
        strikes += mod_strikes - num_strikes[i];
        Serial.print("Module ");
        Serial.print(i);
        Serial.println(" striked");
      }
    }

    if (millis_left <= 0) {
      Serial.println("Ran out of time");
    } else if (strikes >= 3) {
      Serial.println("Striked out");
    }
  }
}
