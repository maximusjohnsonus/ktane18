// Global constants and info 

#ifndef KTANE_H
#define KTANE_H

#define SN_LEN 6
#define MODEL_LEN 4
#define NUM_MODULES 1

// SPI Commands
const byte CMD_PING = 0x01;
const byte CMD_INIT = 0x02;
const byte CMD_INFO = 0x03;
//const byte STATUS = 0x40;

// SPI responses
const byte RSP_READY = 0x10;
const byte RSP_ACTIVE = 0x20;
const byte RSP_SOLVED = 0x30;
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
