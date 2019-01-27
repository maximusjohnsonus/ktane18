// Whatever imports you need

// Whatever variables you need
unsigned long print_time;

// Called once when this module is first connected to power
// Put your hardware setup here (pinMode OUTPUT and all that)
void module_hardware_setup() {

}

// Called once at the start of each game
// Set up variables you use in the main loop here
void module_game_setup() {
    print_time = millis() + 2000;

    // If necessary, wait until game is ready and in a valid physical state

}

// Called constantly while the game is active
// This function should not delay or hang
// (It should take no time so communication does not hang)
void module_loop(unsigned long local_time, // millis left on clock
                 bool *striked,            // Set to true when striked
                 bool *solved              // Set to true when solved
                 ) {
    char c = Serial.read();

    *striked = c == 'x';
    *solved  = c == 'y';

    if (millis() >= print_time) {
        Serial.println("This happens once every two seconds!");
        print_time = millis() + 2000;
    }

}
