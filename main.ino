// Integration Project 1.2 - Arduino Simon Says Game

// #####   EXPLANATION   ########################################################

/*

  The code is split into 5 sections:

  1. EXPLANATION
    - this section
  
  2. INIT
    - declare all used pins
    - initialize all the variables and objects
    - store all of the melody charts

  3. FUNCTIONS
    - declare all the functions that will be used in the game logic

  4. GAME
    - the game logic itself, split into 5 functions, representing the 5 game states:
      - START:          start()
      - AWAIT_START:    awaitStart()
      - SHOW_SEQUENCE:  showSequence()
      - AWAIT_SEQUENCE: awaitSequence()
      - END:            end()
      - AWAIT_RESTART:  awaitRestart()

  5. MAIN
    - the main arduino setup() and loop() functions

  ---------------------------------------------------------------------------

  # ARDUINO PINS
    - 1:                SERVO
    - 2, 3, 4, 5, 6, 7: LCD SCREEN
    - 8, 9, 10, 11:     LEDS
    - 12:               BUTTON
    - 13:               BUZZER
    - A0, A1, A2, A3:   TOUCH SENSORS

    Note: since the servo is connected to pin 1, we cannot use the serial monitor, so we commented out the serial monitor code!

  # GAME LOGIC STRUCTURE
    - The setup() function initializes the lcd screen, (the serial monitor,) and the pin modes
    - The loop() function will update our input states, and call the appropriate game state function based on the current game state
      - we have 3 states where the game does something and then immediately goes to the next state because otherwise it would loop infinitely
        - START          -> AWAIT_START
        - SHOW_SEQUENCE  -> AWAIT_SEQUENCE
        - END            -> AWAIT_RESTART
      - these next 3 states are where the game waits for the player to do something, hence the name AWAIT
        - we want to loop infinitely in these states until the player does something

  # GAME STATES
    - START
      - the game starts in this state
      - the game will play the start melody, and then go to the AWAIT_START state
    - AWAIT_START
      - the game will wait for the player to press the button
      - if the player presses the button, the game will go to the SHOW_SEQUENCE state
    - SHOW_SEQUENCE
      - the game will indicate the current sequence length with the servo
      - the game will show the player the sequence of notes
      - the game will play the sequence of notes, and then go to the AWAIT_SEQUENCE state
    - AWAIT_SEQUENCE
      - the game will wait for the player to press the correct sequence of notes
      - if the player presses the correct sequence of notes, the game will go to the SHOW_SEQUENCE state
      - if the player presses the wrong sequence of notes, they lose and the game goes to the END state
      - if the player presses the correct sequence of notes, and the sequence length is 16, they win and the game goes to the END state
    - END
      - the game will play the win or lose melody, and then go to the AWAIT_RESTART state
    - AWAIT_RESTART
      - the game will wait for the player to press the button
      - if the player presses the button, the game will go to the START state

*/



// #####   INIT   ###############################################################

#include <LiquidCrystal.h>
#include <Servo.h>

// pins
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // re en d4 d5 d6 d7
Servo servo;

const int servoPin = 1;
const int buttonPin = 12;
const int buzzerPin = 13;

const int touchPins[4] = {A0, A1, A2, A3};

const int ledPins[4] = {8, 9, 10, 11};

// input states
int buttonState = LOW;
int buttonStatePrevious = LOW;

int touchStates[4] = {LOW, LOW, LOW, LOW};
int touchStatesPrevious[4] = {LOW, LOW, LOW, LOW};

// pressed states, these will be updated every loop, and will be used to determine if the button or touch sensor was just pressed
bool buttonPressed = false;
bool touchPressed[4] = {false, false, false, false};

// lcd strings
String linesPrevious[] = {"", ""};

// buzzer frequencies and note duration (c3, f3, a3, c4)
int frequencies[4] = {130, 175, 220, 261};
unsigned long noteDuration = 500;

// game states
enum GameState {
  START,          // game start
  AWAIT_START,    // wait for player to start game
  SHOW_SEQUENCE,  // show game sequence
  AWAIT_SEQUENCE, // wait for player touch inputs
  END,            // game end
  AWAIT_RESTART   // wait for player to restart game
};

// variable to keep track of the game state
GameState gameState = START;

// game variables
const int MAX_SEQUENCE_LENGTH = 16;
int gameSequence[16] = {0};
int gameSequenceLength = 1;
int gameSequenceIndex = 0;
bool gameWon = false;

// game charts
const String CHART_GENERIC[] = {"e5", "e4"};
const float  BEATS_GENERIC[] = {  1 ,   2 };
const int      BPM_GENERIC   = 220;
const int   LENGTH_GENERIC   = 2;

const String CHART_START[] = {"c4", "f4", "a4", "c5", "A4", "a4", "g4", "a4", "f4"};
const float  BEATS_START[] = {  1 ,   1 ,   1 ,   3 ,   1 ,   1 ,   2 ,   2 ,   4 };
const int      BPM_START   = 420;
const int   LENGTH_START   = 9;

const String CHART_WIN[] = {"g2", "c3", "e3", "g3", "c4", "e4", "g4", "e4", "G2", "c3", "D3", "G3", "c4", "D4", "G4", "D4", "A2", "d3", "f3", "A3", "d4", "f4", "A4", "A4", "A4", "A4", "c5"};
const float  BEATS_WIN[] = {  1 ,   1 ,   1 ,   1 ,   1 ,   1 ,   3 ,   3 ,   1 ,   1 ,   1 ,   1 ,   1 ,   1 ,   3 ,   3 ,   1 ,   1 ,   1 ,   1 ,   1 ,   1 ,   3 ,   1 ,   1 ,   1 ,   6 };
const int      BPM_WIN   = 420;
const int   LENGTH_WIN   = 27;

const String CHART_LOSE[] = {"a4", "c5", "a5", "g5", "e5", "c5", "d5", "c5"};
const float  BEATS_LOSE[] = {  1 ,   1 ,   1 ,   2 ,   1 ,   2 ,   1 ,   3 };
const int      BPM_LOSE   = 220;
const int   LENGTH_LOSE   = 8;

const String CHART_LOSE2[] = {"a5", "f5", "c5", "a5", "e5", "g5", "a5", "d6", "g6"};
const float  BEATS_LOSE2[] = {  2 ,   1 ,   1 ,   2 ,   2 ,   2 ,   1 ,   1 ,   2 };
const int      BPM_LOSE2   = 220;
const int   LENGTH_LOSE2   = 9;



// #####   FUNCTIONS   ##########################################################

// log (DO NOT USE WHEN USING SERVO)
void log(String message) {
  Serial.println(message);
}

// method to log all input states (DO NOT USE WHEN USING SERVO)
void logInputStates() {
  String message = "";
  
  message += "button: " + String(buttonState) + " ";
  message += "touch: ";

  for (int i = 0; i < 4; i++) {
    message += String(touchStates[i]) + " ";
  }

  log(message);
}

// reset game variables
void reset() {
  gameState = START;
  gameSequenceLength = 1;
  gameSequenceIndex = 0;

  // reset servo
  servo.write(0);

  // randomize the seed and generate the sequence
  delay(100);
  randomSeed(analogRead(A5) + millis());
  for (int i = 0; i < MAX_SEQUENCE_LENGTH; i++) {
    gameSequence[i] = random(0, 4);
  }
}

// print to lcd, but prevent reprinting the same lines
void print(String line1 = "", String line2 = "") {
  // if the lines are the same as the previous ones, don't print
  if (line1 == linesPrevious[0] && line2 == linesPrevious[1]) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  
  // update previous lines
  linesPrevious[0] = line1;
  linesPrevious[1] = line2;
}

// method to read buttons and touch sensors, then update pressed booleans
void updateInputs() {
  // update button pressed
  buttonState = digitalRead(buttonPin);
  buttonPressed = buttonState == HIGH && buttonStatePrevious == LOW;
  buttonStatePrevious = buttonState;

  // update touch pressed
  for (int i = 0; i < 4; i++) {
    touchStates[i] = digitalRead(touchPins[i]);
    // touchStates[i] = analogRead(touchPins[i]) > 1000;
    touchPressed[i] = touchStates[i] == HIGH && touchStatesPrevious[i] == LOW;
    touchStatesPrevious[i] = touchStates[i];
  }
}

// method to turn leds and buzzer off
void turnEverythingOff() {
  // turn off leds
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  // turn off buzzer
  noTone(buzzerPin);
}

// method to play a note from a specific touch sensor
void playNote(int index, bool led = false, float duration = noteDuration) {
  tone(buzzerPin, frequencies[index], duration * 0.9f);

  if (led) {
    // first turn off all leds
    for (int i = 0; i < 4; i++) {
      digitalWrite(ledPins[i], LOW);
    }

    // light up the led for half of the duration of the note
    digitalWrite(ledPins[index], HIGH);
    delay(noteDuration * 0.5);
    digitalWrite(ledPins[index], LOW);
    delay(noteDuration * 0.5);
  }
}

// function to return a frequency (in Hz) from a note name
// for example: "g4" -> 392
int getFrequency(String note) {
  // if empty string, return 0
  if (note == "") {
    return 0;
  }

  // order of notes, from c0 to c8, uppercase letters are sharp notes
  String notes = "cCdDefFgGaAb"; // g4 -> octave 4 (12 * 4 = 48) + note 7 -> note 55

  // we get octave and note number
  int noteIndex = notes.indexOf(note.substring(0, 1));
  int octave = note.substring(1, 2).toInt();

  // c0 = 16.3516
  // then we multiply by 2^(1/12) for each note
  // 2^(1/12) = 1.0594630943592953
  return (int) round(16.3516 * pow(1.0594630943592953, octave * 12 + noteIndex)); // g4 -> 16.3516 * (1.0594630943592953) ^ (55) -> 392-ish
}

// function to return the duration of a number of beats at a given tempo, in ms
int getBeatDuration(float beats, int bpm) {
  return (int) (60000 / bpm * beats);
}

// method to play a chart
// it takes an array of notes, an array of beats, a tempo and a length
// will also light up each led sequentially for the duration of the note
// this doesn't use playNote(), since this is specific to melodies
void playChart(const String chart[], const float beats[], const int bpm, const int length) {
  // led index, this one will go from 0 to 3
  int ledIndex = 0;

  // loop through the chart
  for (int i = 0; i < length; i++) {
    // get frequency and duration
    int frequency = getFrequency(chart[i]);
    int duration = getBeatDuration(beats[i], bpm);

    // play the note unless the frequency is 0
    if (frequency > 0) {
      tone(buzzerPin, frequency, duration * 0.9f);
    }

    // light up the led for the duration of the note
    digitalWrite(ledPins[ledIndex], HIGH);
    delay(duration);
    digitalWrite(ledPins[ledIndex], LOW);

    // increment led index
    ledIndex++;
    if (ledIndex > 3) ledIndex = 0;
  }

  // turn off all leds
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  // turn off the buzzer
  noTone(buzzerPin);
}



// ######   GAME   ##############################################################

// STATE: START
void start() {
  // turn off leds and buzzer
  turnEverythingOff();

  // print start message then wait
  print("Welcome to", "Simon Says!");

  // start melody
  playChart(CHART_START, BEATS_START, BPM_START, LENGTH_START);

  // wait a bit
  delay(200);

  // next state
  turnEverythingOff();
  gameState = AWAIT_START;
}

// STATE: AWAIT_START
void awaitStart() {
  print("Press the button", "to start!");
  if (buttonPressed) {
    reset();
    gameState = SHOW_SEQUENCE;
  }
}

// STATE: SHOW_SEQUENCE
void showSequence() {
  // a string to display the sequence
  String sequence = "";
  print("Please memorize:", sequence);

  // show game sequence length on the servo, using map
  servo.write(map(gameSequenceLength, 0, 16, 0, 180));

  // wait a bit
  delay(200);
  
  // loop through the sequence
  for (int i = 0; i < gameSequenceLength; i++) {
    // add the note to the sequence string
    sequence += gameSequence[i] + 1;
    print("Please memorize:", sequence);
    playNote(gameSequence[i], true);
  }

  // wait a bit
  delay(500);

  // next state
  turnEverythingOff();
  print("Please repeat:", "");
  gameState = AWAIT_SEQUENCE;
}

// STATE: AWAIT_SEQUENCE
/*

  Process:
  - when a touch sensor is pressed:
    - if the note is correct:
      - increment the sequence index
      - if the sequence index is equal to the sequence length:
        - increment the sequence length
        - reset the sequence index
        - if the sequence length is equal to MAX_SEQUENCE_LENGTH:
          - game won
        - else:
          - show sequence
    - if the note is incorrect:
      - game over

*/
void awaitSequence() {
  // if a touch sensor is pressed, play the corresponding note
  for (int i = 0; i < 4; i++) {
    if (touchPressed[i]) {
      playNote(i, false, 500);

      // if the note is correct
      if (gameSequence[gameSequenceIndex] == i) {
        // increment the sequence index
        gameSequenceIndex++;

        // display the sequence
        String sequence = "";
        for (int i = 0; i < gameSequenceIndex; i++) {
          sequence += gameSequence[i] + 1;
        }
        print("Please repeat:", sequence);

        // if the sequence is complete
        if (gameSequenceIndex == gameSequenceLength) {
          // increment the sequence length
          gameSequenceLength++;

          // reset the sequence index
          gameSequenceIndex = 0;

          // wait a bit
          delay(1000);

          // if the sequence length is greater than the max sequence length
          if (gameSequenceLength > MAX_SEQUENCE_LENGTH) {
            // game won
            turnEverythingOff();
            gameWon = true;
            gameState = END;
          } else {
            // next
            gameState = SHOW_SEQUENCE;
          }
        }
      } else {
        // wrong note, game over
        gameWon = false;
        gameState = END;

        // wait a bit
        delay(500);
      }
    }
  }

  // if button is pressed, cheat and win the game
  if (buttonPressed) {
    print("Cheat mode", "activated!");
    playNote(1, true);
    turnEverythingOff();
    gameWon = true;
    gameState = END;
  }
}

// STATE: END
void end() {
  // play the appropriate chart based on whether the game was won or lost
  if (gameWon) {
    print("Congratulations!", "");
    playChart(CHART_WIN, BEATS_WIN, BPM_WIN, LENGTH_WIN);
    delay(200);
    print("You won!", "Press to restart");
  } else {
    print("Oh no!", "Wrong note...");
    playChart(CHART_LOSE, BEATS_LOSE, BPM_LOSE, LENGTH_LOSE);
    delay(200);
    print("Game over!", "Press to restart");
  }

  turnEverythingOff();
  gameState = AWAIT_RESTART;
}

// STATE: AWAIT_RESTART
void awaitRestart() {
  // if button is pressed, restart the game
  if (buttonPressed) {
    playNote(3, true);
    turnEverythingOff();
    gameState = START;
  }
}



// ######   MAIN   ##############################################################

void setup() {
  // initialize lcd and servo
  lcd.begin(16, 2);
  servo.attach(1);
  // Serial.begin(9600);

  servo.write(0);
  
  // initialize button and buzzer pins
  pinMode(buttonPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // initialize touch sensor and led pins
  for (int i = 0; i < 4; i++) {
    pinMode(touchPins[i], INPUT);
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {
  // update the inputs
  updateInputs();
  // logInputStates();

  // light up the leds for the touch sensors
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], touchStates[i]);
  }
  
  // go to the corresponding function based on the game state
  switch (gameState) {
    case START:          return start();
    case AWAIT_START:    return awaitStart();
    case SHOW_SEQUENCE:  return showSequence();
    case AWAIT_SEQUENCE: return awaitSequence();
    case END:            return end();
    case AWAIT_RESTART:  return awaitRestart();
  }
}
