// Global constants and info 

#ifndef KTANE_H
#define KTANE_H

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

#endif 
