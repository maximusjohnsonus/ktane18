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

// Pins
#define SS_ISR_PIN 2
#define WIN_PIN 8
#define KNOCK_PIN A0 

byte SWITCHES_SW_PINS[5]   = {3, 4, 5, 6, 7};
byte SWITCHES_LED_PINS[7]  = {A5, A4, A3, A2, A1, 8, 9};
byte SWITCHES_PATTERNS[10] = {1, 3, 5, 7, 8, 12, 13, 16, 20, 27};

 
const byte KNOCKS_TO_WIN = 4;
const byte KNOCK_PATTERNS[2][KNOCKS_TO_WIN-1] = 
            {{0, 0, 0}, {0, 1, 0}};
const int knock_mins[2] = {300, 700};
const int knock_maxs[2] = {500, 1500};
byte knock_p;

// Which module is this?
#define MODULE_TYPE MODULE_KNOCK

//#define HEADLESS 

// Constants
#define KNOCK_THRESHOLD 50  

game_rand_t game_rand;
game_info_t game_info; // Do not carelessly use game_time; it may be mid-update
                       // (Instead use last_game_time)
volatile int pos; // Position into whatever buffer master is sending data to

byte strikes; // How many strikes the module has had
bool solved;  // Whether this module is solved

byte switches_led[5];
byte switches_sw[5];
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

// bool representation of invalid patterns
bool check_pattern(byte a[5]) {
    byte b = (a[0] << 4) | (a[1] << 3) | (a[2] << 2) | (a[1] << 1) | a[0];
    for (int i=0; i <10; i++) {
        if (b == SWITCHES_PATTERNS[i]) return false;
    }
    return true;
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

    // Put your hardware setup here (pinMode OUTPUT and all that, set variables later)
    //@TODO: your code
    randomSeed(analogRead(0));


    if (MODULE_TYPE == MODULE_KNOCK)
    {
        pinMode(KNOCK_PIN, INPUT);
    } 

    if (MODULE_TYPE == MODULE_SWITCHES) {


        for (int i=0; i<5; i++) {
            pinMode(SWITCHES_SW_PINS[i], INPUT); 
        }

        for (int i=0; i<7; i++) {
            pinMode(SWITCHES_LED_PINS[i], OUTPUT);
        }
    }
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
    
    unsigned long last_knock = 0;
    byte good_knocks = 0;

    //@TODO: your code

    // If necessary, wait until game is ready and in a valid physical state
    //@TODO: your code

    // Done with set up, let master know
    set_state_spdr(STATE_READY);

    Serial.println("Done with setup, polling for start command");
    
    #ifdef HEADLESS
    //    set_state_spdr(STATE_RUN);
    #endif
    
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

    // Put the variables you need across loop iterations here
    //@TODO: your code
    // example:
    unsigned long print_time = millis() + 2000;

    if (MODULE_TYPE == MODULE_KNOCK) {        // Pick a knock pattern based on SN
        byte knock_p = 
            ('0' <= game_rand.sn[0] && game_rand.sn[0] <= '9');
        knock_p = 1;
        Serial.print("Using knock pattern ");
        Serial.println(knock_p);


        digitalWrite(WIN_PIN, LOW); // Not win
    }

    if (MODULE_TYPE == MODULE_SWITCHES)
    {
        do {
            for (int i=0; i<5; i++) {
                switches_led[i] = random(0, 2);
                Serial.print("Switch LED:");
                Serial.print(i);
                Serial.println(switches_led[i]);
            }
        } while (check_pattern(switches_led));
        
        for (int i=0; i<5; i++) {
            digitalWrite(SWITCHES_LED_PINS[i], switches_led[i]); 
        }
    }


    // Play the game
    Serial.println("Starting game");
    while(state != STATE_GAME_OVER){
        // Time left on the game timer according to this module (ms)
        const unsigned long local_time = last_game_time - (millis() - last_info_time);
        

        // Read from Serial for debugging 
        char c = Serial.read();
        striked = c == 'x';
        solved  = c == 'y';
        
        if (MODULE_TYPE == MODULE_KNOCK)
        {
            if (analogRead(KNOCK_PIN) > KNOCK_THRESHOLD) {
                unsigned long now = millis();
                Serial.print(now);
                Serial.println(" Knock!");
                byte knock_i = KNOCK_PATTERNS[knock_p][good_knocks];
                
                if (now - last_knock > 200) {  // This knock isn't bounce
                    if (now - last_knock < knock_mins[knock_i]) {
                        // Too soon
                        Serial.println("Knock too soon");
                        good_knocks = 0;
                    }
                    else if (now - last_knock < knock_maxs[knock_i]) { 
                        // Good knock
                        good_knocks++;
                        Serial.print("Good knock! You have ");
                        Serial.print(good_knocks);
                        Serial.println(" successful knocks!");
                    }
                    else { 
                        // Knock too late, reset
                        Serial.println("Knock too late");
                        good_knocks = 0;
                    }
                    last_knock = now;
                }    
            }
            
            // Initial knock not counted as successful knock
            if (good_knocks == KNOCKS_TO_WIN-1) {
                Serial.println("Good knocking!");
                solved = true;
            }

        }
       
        if (MODULE_TYPE == MODULE_SWITCHES) { 
            for (int i=0; i<5; i++) {
                switches_sw[i] = digitalRead(SWITCHES_SW_PINS[i]);
            }
            
            if (!check_pattern(switches_sw)) {
                striked = true; 
            }

            solved = true;
            for (int i=1; i<5; i++) {
                if (switches_sw[i] != switches_led[i]) solved = false;
            }

            if (solved) break;
        }

        // Module code here. DO NOT USE DELAY pls I think it will break things
        //@TODO: your code
        // example:
        if (millis() >= print_time) {
            Serial.println("This happens once every two seconds!");
            print_time = millis() + 2000;
        }


        // End module code

        // Deal with strikes
        if (striked) {
            Serial.println("Striking");
            strikes++;
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
