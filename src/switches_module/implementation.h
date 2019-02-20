// Whatever imports you need

// Whatever variables you need
const byte SWITCHES_SW_PINS[5]   = {3, 4, 5, 6, 7};
// TODO: why 7?
const byte SWITCHES_LED_PINS[7]  = {A5, A4, A3, A2, A1, 8, 9};
const byte SWITCHES_PATTERNS[10] = {1, 3, 5, 7, 8, 12, 13, 16, 20, 27}; // Bitwise illegal positions
byte switches_led[5];
byte switches_sw[5];

// Returns true if the switches are in a valid position, false otherwise.
bool check_pattern(byte a[5]) {
    byte b = (a[0] << 4) | (a[1] << 3) | (a[2] << 2) | (a[1] << 1) | a[0];
    for (int i=0; i <10; i++) {
        if (b == SWITCHES_PATTERNS[i]) return false;
    }
    return true;
}

// Called once when this module is first connected to power
// Put your hardware setup here (pinMode OUTPUT and all that)
void module_hardware_setup() {
    for (int i=0; i<5; i++) {
        pinMode(SWITCHES_SW_PINS[i], INPUT);
    }

    for (int i=0; i<7; i++) {
        pinMode(SWITCHES_LED_PINS[i], OUTPUT);
    }

}

// Called once at the start of each game
// Set up variables you use in the main loop here
void module_game_setup() {
    do {
        for (int i=0; i<5; i++) {
            switches_led[i] = random(0, 2);
            Serial.print("Switch LED:");
            Serial.print(i);
            Serial.println(switches_led[i]);
        }
    } while (check_pattern(switches_led)); // TODO: why is this not not-ed?

    for (int i=0; i<5; i++) {
        digitalWrite(SWITCHES_LED_PINS[i], switches_led[i]);
    }

    // If necessary, wait until game is ready and in a valid physical state

    // TODO: should do this
}

// called constantly while the game is active
// This function should not delay or hang
// (It should take no time so communication does not hang)
void module_loop(unsigned long local_time, // millis left on clock
                 bool *striked,            // Set to true when striked
                 bool *solved              // Set to true when solved
                 ) {
    for (int i=0; i<5; i++) {
        switches_sw[i] = digitalRead(SWITCHES_SW_PINS[i]);
    }

    if (!check_pattern(switches_sw)) {
        *striked = true;
    }

    bool my_solved = true;
    for (int i=1; i<5; i++) {
        if (switches_sw[i] != switches_led[i]) my_solved = false;
    }

    *solved = my_solved;
}
