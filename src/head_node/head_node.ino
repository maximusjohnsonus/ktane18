// Mega pins
// MOSI: 51 or ICSP-4
// MISO: 50 or ICSP-1
// SCK:  52 or ICSP-3
// SS (slave): 10 



#include <SPI.h>
#include <Wire.h>
#include "pins_arduino.h"
#include "ktane.h"

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
  
  for(int i = 0; i < NUM_MODULES; i++){
    pinMode(slave_pins[i], OUTPUT);
    digitalWrite(slave_pins[i], HIGH);
  }

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
  SPI.transfer(CMD_INIT);
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
  while (true) {
    delay(100);

    for (int i = 0; i < NUM_MODULES; i++) {
      Serial.print("Sending PING to slave ");
      Serial.println(i);

      digitalWrite(slave_pins[i], LOW);
      byte rsp = SPI.transfer(CMD_PING);
      digitalWrite(slave_pins[i], HIGH);
      Serial.print("Received ");
      Serial.println(rsp);

      if (rsp & ~STRIKE_MASK == RSP_READY) {
        Serial.print("Received READY from slave ");
        Serial.println(i);
      }
    }
  }

 
}
