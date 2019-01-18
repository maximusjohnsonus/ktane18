// Global constants and info

#ifndef KTANE_H
#define KTANE_H

#define SN_LEN 6
#define MODEL_LEN 4
#define NUM_MODULES 2

// SPI Commands
const byte CMD_PING = 0x01;
const byte CMD_INIT = 0x02;
const byte CMD_INFO = 0x03;
const byte CMD_LOST = 0x04;
const byte CMD_WON  = 0x05;

// SPI responses
const byte RSP_UNREADY = 0x10;
const byte RSP_READY   = 0x20;
const byte RSP_ACTIVE  = 0x30;
const byte RSP_NEEDY   = 0x40;
const byte RSP_SOLVED  = 0x50;
const byte RSP_DEBUG   = 0xF0;


// Useful others
const byte STRIKE_MASK = 0x03;

// All the random components of this game
struct game_rand_t {
    char sn[SN_LEN+1];
    char model[MODEL_LEN+1];
    byte indicators;

    void print_rand() {
        Serial.write("SN:");
        Serial.write((unsigned char*)sn, SN_LEN);
        Serial.write(", Model:");
        Serial.write((unsigned char*)model, MODEL_LEN);
        Serial.write(", IND:");
        Serial.println((int)indicators, BIN);
    }

    void gen_rand() {
        // TODO: seed random better (repeated readings? low-order bits?)
        randomSeed(analogRead(A0));

        for (int i = 0; i < SN_LEN; i++) {
            sn[i] = random(2) ? random('0', '9'+1) 
                              : random('A', 'Z'+1);
        }
        sn[SN_LEN] = '\0';

        for (int i = 0; i < MODEL_LEN; i++) {
            model[i] = random(2) ? random('0', '9'+1)
                                 : random('A', 'Z'+1);
        }
        model[MODEL_LEN] = '\0';

        indicators = random(256);
        print_rand();
    }
};

struct game_info_t {
    byte strikes;            // How many strikes the game has had
    unsigned long game_time; // Time left on the game timer (ms)

    void print_info() {
        Serial.print("Strikes:");
        Serial.print(strikes);
        Serial.print(", time:");
        Serial.println(game_time, HEX);
    }
};

enum module_type_t {
    MODULE_SAMPLE,
    MODULE_KNOCK,
    MODULE_SWITCHES,
};


#endif
