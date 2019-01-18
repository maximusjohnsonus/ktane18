// Pin Assignments
#define Y 0
#define R 1
#define G 2
#define B 3

// Connections
int statLED = 8;
int yLED = A3;
int ySW = 3;
int rLED = A4;
int rSW = 4;
int gLED = 6;
int gSW = 7;
int bLED = A5;
int bSW = 5;


// Sequence
int seq[4];

// Sequence Pressed
int pressed[4] = {1, 1, 1, 1}; 

//Data on rounds
int curr_round = 1;
int max_round = 3;
//So, if curr_round > max_round, you have won the game
int input_num = 1;
//So, if input_num > curr_round; round completed

//Input value of switches
int yVal, rVal, gVal, bVal;

//Dummy information
String dummy_SN = "J0HNDU";
byte dummy_strikes = 0;

//States
bool set = true;
bool disp1 = false;
bool detect = false;
bool check1 = false;
bool disp2 = false;
bool disp3 = false;
bool win = false;
bool lose = false;
bool check2 = false;
//TBD: More States

//Other variables
bool vowel;
bool from_detect;

//If SN has a vowel
int vowel_soln[3][4] = {
// Y  R  G  B
  {G, B, Y, R},
  {R, Y, B, G},
  {B, G, Y, R}
};

int noVowel_soln[3][4] = {
  {R, B, G, Y},
  {G, R, Y, B},
  {R, Y, B, G}
};

//HELPER FUNCTIONS

//Sets sequence 
void set_sequence() {
  for(int i = 0; i < max_round; i++)
    seq[i] = random(4);
}

//Checks for vowel in sequence
bool check_vowel(String str) {
  for(int i = 0; i < str.length(); i++) {
    if(str.charAt(i) == 'A' or str.charAt(i) == 'E' or str.charAt(i) == 'I' or
       str.charAt(i) == 'O' or str.charAt(i) == 'U')
       return true;
  }
  return false;
}

//Lights up LED
void lumos(int input) {
  if (input == Y) digitalWrite(yLED, HIGH);
  if (input == R) digitalWrite(rLED, HIGH);
  if (input == G) digitalWrite(gLED, HIGH);
  if (input == B) digitalWrite(bLED, HIGH);
}

//Puts LED off
void nox(int input) {
  if (input == Y) digitalWrite(yLED, LOW);
  if (input == R) digitalWrite(rLED, LOW);
  if (input == G) digitalWrite(gLED, LOW);
  if (input == B) digitalWrite(bLED, LOW);
}

//goes to appropriate state
void gotodisp() {
  if (curr_round == 1) disp1 = true;
  else if (curr_round == 2) disp2 = true;
  else if (curr_round == 3) disp3 = true;
  else if (curr_round == 4) win = true;
}

void setup() {
   randomSeed(analogRead(0));
  Serial.begin(9600);
  
  pinMode(yLED, OUTPUT);
  pinMode(ySW, INPUT);
  
  pinMode(rLED, OUTPUT);
  pinMode(rSW, INPUT);
  
  pinMode(gLED, OUTPUT);
  pinMode(gSW, INPUT);
  
  pinMode(bLED, OUTPUT);
  pinMode(bSW, INPUT);

  pinMode(statLED, OUTPUT);
}

void loop() {
  Serial.println(dummy_strikes);

  //Getting inputs at the start and executing based on these
  rVal = digitalRead(rSW);
  yVal = digitalRead(ySW);
  gVal = digitalRead(gSW);
  bVal = digitalRead(bSW);

  //If switch is pressed, illuminate
  if (rVal == 0) digitalWrite(rLED, 1);
  else digitalWrite(rLED, 0);
  if (gVal == 0) digitalWrite(gLED, 1);
  else digitalWrite(gLED, 0);
  if (bVal == 0) digitalWrite(bLED, 1);
  else digitalWrite(bLED, 0);
  if (yVal == 0) digitalWrite(yLED, 1);
  else digitalWrite(yLED, 0);
    
  //States NOTE: Take notes in notepad
  //Setup
  if(set) {
    vowel = check_vowel(dummy_SN);
    set_sequence();
    curr_round = 1;
    pressed[Y] = 1;
    pressed[R] = 1;
    pressed[G] = 1;
    pressed[B] = 1;
    set = false;
    disp1 = true;
  }
  
  //Display for round 1
  if (disp1) {
    //Flash sequence for round 1
    if (millis()%2500 < 500) 
      lumos(seq[0]);

    //Check if a button was pressed
    if(rVal + yVal + bVal + gVal == 3) {
      //Go to next state
      disp1 = false;
      detect = true;
      
      //Input which one was pressed
      pressed[R] = rVal;
      pressed[G] = gVal;
      pressed[B] = bVal;
      pressed[Y] = yVal;
    }
  }
  
  //Detects a button pressed
  if(detect) {
    from_detect = true;
    if(pressed[R] != rVal  or pressed[G] != gVal or 
       pressed[B] != bVal or pressed[Y] != yVal) {
       detect = false;
          
       //Checks which state to go to
       if(curr_round == 1) check1 = true;
       if(curr_round == 2) check2 = true;
       if(curr_round == 3) check2 = true;
       if(curr_round == 4) win = true;
    }
  }
  //Check button pressed for round1
  if(check1) {
    if(vowel) 
      if(pressed[vowel_soln[dummy_strikes][seq[0]]] == 0) curr_round++;
      else dummy_strikes++;
    else
      if(pressed[noVowel_soln[dummy_strikes][seq[0]]] == 0) curr_round++;
      else dummy_strikes++;
  //Reset Pressed Buttons
  pressed[R] = 1;
  pressed[G] = 1;
  pressed[B] = 1;
  pressed[Y] = 1;
  check1 = false;
 
  if(curr_round > 1)
    disp2 = true;
  else if(dummy_strikes >= 3)
    lose = true;
  else
    disp1 = true;
  }
  //Displays input and then checks for the first press
  if(disp2) {
    input_num = 1;
    if(millis()%3000 > 500 and millis()%3000 < 1000) 
      lumos(seq[0]);
    if(millis()%3000 > 1200 and millis()%3000 < 1700) 
      lumos(seq[1]);

    if(rVal + yVal + bVal + gVal == 3) {
      //Go to next state
      disp2 = false;
      detect = true;

      //Input which one was pressed
      pressed[R] = rVal;
      pressed[G] = gVal;
      pressed[B] = bVal;
      pressed[Y] = yVal;
    }
  }
  
  if(check2) {
    bool wrong = false;
    
    if(from_detect) { 
      from_detect = false;
      if(vowel) { 
        if(pressed[vowel_soln[dummy_strikes][seq[input_num-1]]] == 0) input_num++;
        else {
          wrong = true;
        }
      }
      else {
        if(pressed[noVowel_soln[dummy_strikes][seq[input_num-1]]] == 0) input_num++;
        else {
          wrong = true;
        }
      }
      //Reset Pressed Buttons
      pressed[R] = 1;
      pressed[G] = 1;
      pressed[B] = 1;
      pressed[Y] = 1;
    }
    
    if(input_num > curr_round) {
      curr_round++;
      check2 = false;
      gotodisp();
      input_num = 1;
    }
    else if(wrong) {
      check2 = false;
      dummy_strikes++;
      if(dummy_strikes >= 3)
        lose = true;
      else
        gotodisp();
    }
    else if (rVal + yVal + bVal +gVal == 3) {
      check2 = false;
      detect = true;
    
      pressed[R] = rVal;
      pressed[G] = gVal;
      pressed[B] = bVal;
      pressed[Y] = yVal;
    }
  }
  if(disp3) {
    if(millis()%3500 > 200 and millis()%3500 < 600) 
      lumos(seq[0]);
    if(millis()%3500 > 800 and millis()%3500 < 1200) 
      lumos(seq[1]);
    if(millis()%3500 > 1400 and millis()%3500 < 1800) 
      lumos(seq[2]);

    if(rVal + yVal + bVal + gVal == 3) {
      //Go to next state
      disp3 = false;
      detect = true;

      //Input which one was pressed
      pressed[R] = rVal;
      pressed[G] = gVal;
      pressed[B] = bVal;
      pressed[Y] = yVal;
    }  
  }
  if(win) {
    digitalWrite(statLED, HIGH);
  }
  if(lose) {
    nox(R);
    nox(G);
    nox(B);
    nox(Y);
  }
}
