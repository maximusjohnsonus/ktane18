// Slave module
// Uno pins
// MOSI: 11 or ICSP-4
// MISO: 12 or ICSP-1
// SCK:  13 or ICSP-3
// SS (slave): 10
// Interrupt: 2 (connect directly to 10)


#include <Wire.h>
#include "pins_arduino.h"
#include "ktane.h"

// Module to import

#include "test_module.h"

// Pins
#define SS_ISR_PIN 2
#define WIN_PIN 8

game_rand_t game_rand;
game_info_t game_info; // Do not carelessly use game_time; it may be mid-update
                       // (Instead use last_game_time)
volatile int pos; // Position into whatever buffer master is sending data to

byte strikes; // How many strikes the module has had
bool solved;  // Whether this module is solved

// Result of millis() when last info was completed
volatile unsigned long last_info_time;
// Copy of game_info.game_time that will never be partially-updated
volatile unsigned long last_game_time;

enum state_t {
    STATE_UNREADY,     // Game need some set up
    STATE_READY,       // Game hasn't started
    STATE_RUN,         // Game running
    STATE_READ_INIT,   // Reading init data from master (e.g. SN)
    STATE_READ_INFO,   // Reading info from master (time + strikes)
    STATE_SOLVED,      // Slave solved
    STATE_GAME_OVER    // Game is over
};

volatile state_t state;
volatile bool interrupt_called;
volatile bool print_info;

struct interrupt_debug_t {
    byte recv;
    void init() { recv = -1; }

    void print_interrupt() {
        Serial.print("Received SPI byte ");
        Serial.println(recv, BIN);
    }
} int_debug;


void get_miso() {
    if (digitalRead(SS) == LOW) {
        pinMode(MISO, OUTPUT);
    } else {
        pinMode(MISO, INPUT);
    }
}


// Update state and SPDR
void set_state_spdr(state_t new_state){
    state = new_state;
    // Use spdr_new b/c reading SPDR changes it
    byte spdr_new = RSP_DEBUG;

    switch(state){
        case STATE_UNREADY:
            spdr_new = RSP_UNREADY;
            break;
        case STATE_READY:
            spdr_new = RSP_READY;
            break;
        case STATE_RUN:
            spdr_new = RSP_ACTIVE;
            break;
        case STATE_READ_INIT:
            spdr_new = RSP_READY;
            break;
        case STATE_READ_INFO:
            spdr_new = RSP_ACTIVE;
            break;
        case STATE_SOLVED:
            spdr_new = RSP_SOLVED;
            break;
        case STATE_GAME_OVER:
            spdr_new = RSP_UNREADY;
            break;
    }
    spdr_new |= strikes;

    SPDR = spdr_new;
}

// SPI interrupt routine
// Serial communication can mess up interrupts, so store debug info elsewhere
ISR (SPI_STC_vect) {
    byte c = SPDR;
    SPDR = RSP_DEBUG; // We should ensure that RSP_DEBUG never stays in SPDR
                      // because we should always update it to the correct value

    int_debug.recv = c;
    if (state == STATE_UNREADY) {
        set_state_spdr(STATE_UNREADY);

    } else if (state == STATE_READY) {
        if (c == CMD_INIT) {
            set_state_spdr(STATE_READ_INIT);
            pos = 0;
        } else if (c == CMD_INFO) {
        digitalWrite(WIN_PIN, LOW); // Not win
            set_state_spdr(STATE_READ_INFO);
            pos = 0;
        } else if (c == CMD_PING) {
            set_state_spdr(STATE_READY);
        }

    } else if (state == STATE_RUN) {
        if (c == CMD_INFO) {
            set_state_spdr(STATE_READ_INFO);
            pos = 0;
        } else if ((c == CMD_WON) or (c == CMD_LOST)) {
            set_state_spdr(STATE_GAME_OVER);
        }

    } else if (state == STATE_READ_INIT){
        // Copy over bytes to game_rand struct
        ((byte *)&game_rand)[pos] = c;
        pos++;

        if (pos == sizeof(game_rand_t)) {
            set_state_spdr(STATE_READY);
            pos = 0;
            print_info = true;
        } else {
            set_state_spdr(STATE_READ_INIT);
        }

    } else if (state == STATE_READ_INFO){
        // Copy over bytes to game_info struct
        ((byte *)&game_info)[pos] = c;
        pos++;

        if (pos == sizeof(game_info_t)) {
            set_state_spdr(solved ? STATE_SOLVED : STATE_RUN);

            // Update our timing globals
            last_game_time = game_info.game_time;
            last_info_time = millis();

            pos = 0;
            print_info = true;
        } else {
            set_state_spdr(STATE_READ_INFO);
        }
    } else if (state == STATE_SOLVED) {
        if (c == CMD_INFO) {
            set_state_spdr(STATE_READ_INFO);
            pos = 0;
        } else if ((c == CMD_WON) or (c == CMD_LOST)) {
            set_state_spdr(STATE_GAME_OVER);
        } else {
            set_state_spdr(STATE_SOLVED);
        }
    } else if (state == STATE_GAME_OVER) {
        set_state_spdr(STATE_GAME_OVER);
    }

    interrupt_called = true;
}

void setup (void) {
    Serial.begin(9600);
    Serial.println("MODULE v0.01 alpha");

    // Don't grab MISO until selectesd
    pinMode(MISO, INPUT);
    pinMode(SS, INPUT);
    pinMode(SS_ISR_PIN, INPUT);
    pinMode(WIN_PIN, OUTPUT);

    // turn on SPI in slave mode
    SPCR |= _BV(SPE);

    // turn on interrupts
    SPCR |= _BV(SPIE);

    // Set MISO to OUTPUT when SS goes LOW
    // Docs say only pins 2 and 3 are usable
    attachInterrupt(digitalPinToInterrupt(SS_ISR_PIN), get_miso, CHANGE);

    // Set up module hardware
    module_hardware_setup();

    // Seed random with analog reads
    seed_rand();
}


// Each iteration of loop will be one game
void loop (void) {
    Serial.println("Starting run");

    // Set up for each run
    strikes = 0;
    pos = 0;
    state = STATE_UNREADY;
    interrupt_called = false;
    print_info = false;
    solved = false;
    bool striked = false;

    // Set up for the run
    module_game_setup();

    // Done with set up, let master know
    set_state_spdr(STATE_READY);

    Serial.println("Done with setup, polling for start command");

    // Wait for start command
    while (state != STATE_RUN) {
        if (interrupt_called) {
            Serial.print("State: ");
            Serial.println(state);

            int_debug.print_interrupt();
            int_debug.init();
            interrupt_called = false;

            if (print_info) {
                game_rand.print_rand();
                print_info = false;
            }
        }
    }


    // Play the game
    Serial.println("Starting game");

    while (state != STATE_GAME_OVER) {
        // Time left on the game timer according to this module (ms)
        const unsigned long local_time = last_game_time - (millis() - last_info_time);

        // Call module code
        module_loop(local_time, &striked, &solved);

        // Deal with strikes
        if (striked) {
            Serial.println("Striking");
            strikes++;
            striked = false;
            set_state_spdr(state); // Update strikes in SPDR immediately
        }

        if (solved) {
            Serial.println("Solved!");
            set_state_spdr(STATE_SOLVED);
            digitalWrite(WIN_PIN, HIGH);
            break;
        }

        // Debug stuff
        if (interrupt_called) {
            int_debug.print_interrupt();
            int_debug.init();
            interrupt_called = false;

            if (print_info) {
                game_info.print_info();
                print_info = false;
            }
        }

    }

    // Wait for a game end signal
    if (solved) {
        Serial.println("Waiting for the game to end...");
        while (state != STATE_GAME_OVER);
    } else {
        Serial.println("You Lost :(");
    }

    Serial.println("Game over");
    delay(10000);
}
