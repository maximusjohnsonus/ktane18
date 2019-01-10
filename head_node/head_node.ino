#include <SPI.h>
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

// All the random components of this game
struct game_rand_t {
  char sn[SN_LEN];
  char model[MODEL_LEN];
  byte indicators;

  void print_rand(){
    Serial.write("SN:");
    Serial.write(sn, SN_LEN);
    Serial.write(", Model:");
    Serial.write(model, MODEL_LEN);
    Serial.write(", IND:");
    Serial.println((int)indicators, BIN);
  }
};
game_rand_t game_rand;

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

  gen_rand();

  Serial.println("Establishing connections");
  // Try to establish connection with each module a few times a second
  while (!Serial.available() or Serial.read() != 's') {
    delay(100);

    for (int i = 0; i < NUM_MODULES; i++) {
      digitalWrite(slave_pins[i], LOW);
      byte resp = SPI.transfer(PING);
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

  for (int i = 0; i < NUM_MODULES; i++) {
    digitalWrite(slave_pins[i], LOW);
    byte resp = SPI.transfer(START);
    digitalWrite(slave_pins[i], HIGH);
  }

  Serial.println("Running");
  while(1);
  /*
  //digitalWrite(slave_pins[slave_idx], LOW);
  SPI.transfer(TX_SN);
  delay(1000);
  //Serial.println(sn);
  //SPI.transfer(sn, 6);
  delay(1000);
  SPI.transfer(START);
  //digitalWrite(slave_pins[slave_idx], HIGH);

  Serial.println("Done");
  unsigned long t = millis();
  while (1) {
    bool s = LOW;
    if (millis() - t >= 1000) {
      t += 1000;
      digitalWrite(7, s);
      s = !s;
    }
  }
  */
}
