#include <TimerOne.h>

const bool USE_SERIAL = true;
const long int SERIAL_BPS = 115200;

const int PIN_LED = 13;
const int PIN_CONTROL_STEP = 1; // rojo
const int PIN_CONTROL_DIRECTION = 0; // azul
const int NUMBER_OF_FLOPPIES = 7;
const int MAX_POSITION = 80; // fd has 80 tracks
const unsigned int INITIAL_MAX_MOTOR_MOVEMENT = 2; // 160 produces full movement, 2 makes louder sound
unsigned int maxMotorMovement = INITIAL_MAX_MOTOR_MOVEMENT;

const int DIRECTION_BACKWARDS = HIGH;
const int DIRECTION_FORWARDS = LOW;

const int REST = 0;   // silence
const int A_5  = 21;  // 880 Hz

const int Ab_5 = 22;  // ? Hz
const int G_5  = 24;  // ? Hz
const int Gb_5 = 25;  // ? Hz
const int F_5  = 27;  // ? Hz
const int E_5  = 29;  // ? Hz
const int Eb_5 = 30;  // ? Hz
const int D_5  = 32;  // ? Hz
const int Db_5 = 34;  // ? Hz
const int C_5  = 36;  // ? Hz

const int B_4  = 38;  // 494 Hz
const int Bb_4 = 40;  // 466 Hz
const int A_4  = 42;  // 440 Hz
const int Ab_4 = 45;  // 415 Hz
const int G_4  = 48;  // 392 Hz
const int Gb_4 = 51;  // 370 Hz
const int F_4  = 54;  // 349 Hz
const int E_4  = 57;  // 329 Hz
const int Eb_4 = 60;  // 311 Hz
const int D_4  = 64;  // 294 Hz
const int Db_4 = 68;  // 277 Hz
const int C_4  = 72;  // 262 Hz
const int B_3  = 76;  // 247 Hz
const int Bb_3 = 80;  // 233 Hz
const int A_3  = 84;  // 220 Hz

const int Ab_3 = 90;  // ? Hz
const int G_3  = 96;  // ? Hz
const int Gb_3 = 102;  // ? Hz
const int F_3  = 108;  // ? Hz
const int E_3  = 114;  // ? Hz
const int Eb_3 = 120;  // ? Hz
const int D_3  = 128;  // ? Hz
const int Db_3 = 136;  // ? Hz
const int C_3  = 144;  // ? Hz
const int B_2  = 152;  // ? Hz
const int Bb_2 = 160;  // ? Hz

const int A_2  = 168; // 110 Hz
const int END = 1000;  // loop

const bool INITIAL_AUTO_PLAY = true;
bool autoPlay = INITIAL_AUTO_PLAY;

/*
const int NOTE_LIST[] = {
  A_3, END
};*/

const int NOTE_LIST[] = {
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, E_4, REST, D_4, REST,
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, D_4, REST, C_4, REST,
  D_4, REST, E_4, C_4, D_4, F_4, E_4, C_4, D_4, F_4, E_4, C_4, D_4, E_4, G_4, REST,
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, D_4, REST, C_4, REST,
  REST, REST, END
};

unsigned int noteListIndex = 0;

const unsigned int CYCLE_PERIOD = 27; // microseconds
const unsigned int INITIAL_NOTE_DURATION_MS = 170; // ms
const unsigned long INITIAL_NOTE_CHANGE_THRESHOLD = INITIAL_NOTE_DURATION_MS * 1000UL / CYCLE_PERIOD;
unsigned int noteDurationMs = INITIAL_NOTE_DURATION_MS;
unsigned long noteChangeThreshold = INITIAL_NOTE_CHANGE_THRESHOLD;

volatile unsigned int currentPosition = 0;
volatile bool currentDirection = false;
volatile bool currentStep = false;
volatile unsigned int tick = 0;
volatile unsigned int currentPeriod = 0;
bool playCurrentNote = false;
bool autoPlayingNote = false;

volatile unsigned long noteChangeCounter = 0;
unsigned int nextNote = 0;

byte incomingByte = 0;
byte specialByte = 0;
unsigned int incomingInt = 0;
unsigned int nextPeriod = 0;

char debug_string_buffer[20];
// sprintf + serial of 20 bytes takes ~200us
// sprintf + serial of 10 bytes takes ~144us
// sprintf + serial of  5 bytes takes ~108us
#define debug(formato, valor) \
  sprintf(debug_string_buffer, formato, valor); \
  Serial.print(debug_string_buffer)

void setup() {
  if (USE_SERIAL) {
    Serial.begin(SERIAL_BPS);
    Serial.setTimeout(10); // parseInt uses this timeout
  }
  
  //randomSeed(analogRead(0)); // init random seed
  
  pinMode(PIN_LED, OUTPUT);
  for (int f = 0; f < NUMBER_OF_FLOPPIES; f++) {
    pinMode(PIN_CONTROL_STEP + f * 2, OUTPUT);
    pinMode(PIN_CONTROL_DIRECTION + f * 2, OUTPUT);
  }
  
  //delay(1000);
  digitalWrite(PIN_LED, HIGH);
  
  // reset position: set direction to backwards until the 0 position 
  // is reached
  for (int f = 0; f < NUMBER_OF_FLOPPIES; f++) {
    digitalWrite(PIN_CONTROL_DIRECTION + f * 2, DIRECTION_BACKWARDS);
    for (int i = 0; i < MAX_POSITION; i++) {
      digitalWrite(PIN_CONTROL_STEP + f * 2, HIGH);
      delay(5);
      digitalWrite(PIN_CONTROL_STEP + f * 2, LOW);
      delay(5);
    }
  }
  // set direction to forwards until the max position, and a bit beyond
  for (int f = 0; f < NUMBER_OF_FLOPPIES; f++) {
    digitalWrite(PIN_CONTROL_DIRECTION + f * 2, DIRECTION_FORWARDS);
    for (int i = 0; i < MAX_POSITION + 2; i++) {
     digitalWrite(PIN_CONTROL_STEP + f * 2, HIGH);
     delay(5);
     digitalWrite(PIN_CONTROL_STEP + f * 2, LOW);
     delay(5);
    }
  }
  digitalWrite(PIN_LED, LOW);

  currentPeriod = 0;
  
  //delay(2000);
  
  Timer1.initialize(CYCLE_PERIOD);
  Timer1.attachInterrupt(noteSetter);
  Timer1.start();

}

void noteSetter() {
  if (playCurrentNote) {
    // move motor every n ticks
    tick++;
    if (tick == currentPeriod) {
      for (int f = 0; f < NUMBER_OF_FLOPPIES; f++) {
        digitalWrite(PIN_CONTROL_STEP + f * 2, currentStep);
        digitalWrite(PIN_CONTROL_DIRECTION + f * 2, currentDirection);
      }
      currentStep = !currentStep;
      currentPosition++;
      tick = 0;
    }
    // toggle position
    if (currentPosition == maxMotorMovement) {
      currentDirection = !currentDirection;
      currentPosition = 0;
    }
  }
  noteChangeCounter++;
}

void loop() {
  
  // every noteChangeThreshold ms, change note
  if (autoPlay) {
    if (!autoPlayingNote && noteChangeCounter >= noteChangeThreshold) {
      digitalWrite(PIN_LED, HIGH);
      
      autoPlayingNote = true;
      playCurrentNote = true;

      currentPeriod = NOTE_LIST[noteListIndex];
      tick = 0;
      
      noteListIndex++;
      if (NOTE_LIST[noteListIndex - 1] == END) {
        noteListIndex = 0;
        currentPeriod = NOTE_LIST[noteListIndex];
        noteListIndex++;
      }
      
      printCurrentNote();
    }
  
    // if a note is playing, every noteChangeThreshold ms, silence note
    if (autoPlayingNote && noteChangeCounter >= noteChangeThreshold * 2) {
      digitalWrite(PIN_LED, LOW);
      
      autoPlayingNote = false;
      playCurrentNote = false;
      currentPeriod = 0;
      tick = 0;
      
      noteChangeCounter = 0;
      
      if (USE_SERIAL) {
        Serial.println("stop");
      }
    }
    
  }

  if (USE_SERIAL) {
    if (Serial.available() > 0) {
      incomingByte = Serial.read();
      
      switch (incomingByte) {
        case '>':
        case '<':
          specialByte = Serial.read();
          switch (specialByte) {
            case 'p': nextPeriod = E_5;   break;
            case '0': nextPeriod = Eb_5;  break;
            case 'o': nextPeriod = D_5;   break;
            case '9': nextPeriod = Db_5;  break;
            case 'i': nextPeriod = C_5;   break;
            case 'u': nextPeriod = B_4;   break; // right values v
            case '7': nextPeriod = Bb_4;  break;
            case 'y': nextPeriod = A_4;   break;
            case '6': nextPeriod = Ab_4;  break;
            case 't': nextPeriod = G_4;   break;
            case '5': nextPeriod = Gb_4;  break;
            case 'r': nextPeriod = F_4;   break;
            case 'e': nextPeriod = E_4;   break;
            case '3': nextPeriod = Eb_4;  break;
            case 'w': nextPeriod = D_4;   break;
            case '2': nextPeriod = Db_4;  break;
            case 'q': nextPeriod = C_4;   break; // C4
            
            case '-': nextPeriod = E_4;   break;
            case '\'': nextPeriod = Eb_4; break;
            case '.': nextPeriod = D_4;   break;
            case 'l': nextPeriod = Db_4;  break;
            case ',': nextPeriod = C_4;   break; // C4
            case 'm': nextPeriod = B_3;   break;
            case 'j': nextPeriod = Bb_3;  break;
            case 'n': nextPeriod = A_3;   break; // right values ^
            case 'h': nextPeriod = Ab_3;  break;
            case 'b': nextPeriod = G_3;   break;
            case 'g': nextPeriod = Gb_3;  break;
            case 'v': nextPeriod = F_3;   break;
            case 'c': nextPeriod = E_3;   break;
            case 'd': nextPeriod = Eb_3;  break;
            case 'x': nextPeriod = D_3;   break;
            case 's': nextPeriod = Db_3;  break;
            case 'z': nextPeriod = C_3;   break; // C3
            
            default: playCurrentNote = false; break;
          }
          if (incomingByte == '>' && nextPeriod != REST) {
            playCurrentNote = true;
            currentPeriod = nextPeriod;
            tick = 0;
            printCurrentNote();
          }
          if (incomingByte == '<' && nextPeriod == currentPeriod) {
              playCurrentNote = false;
          }
          nextPeriod = REST;
          digitalWrite(PIN_LED, playCurrentNote);
          break;
        case '!':
          specialByte = Serial.read();
          switch (specialByte) {
            case 'b': // move motor backwards 20 steps
              for (int f = 0; f < NUMBER_OF_FLOPPIES; f++) {
                digitalWrite(PIN_CONTROL_DIRECTION + f * 2, DIRECTION_BACKWARDS);
                for (int i = 0; i < 20; i++) {
                  digitalWrite(PIN_CONTROL_STEP + f * 2, HIGH);
                  delay(5);
                  digitalWrite(PIN_CONTROL_STEP + f * 2, LOW);
                  delay(5);
                }
              }
              Serial.println("move motor backwards");
              break;
            case 'f': // move motor forwards
              for (int f = 0; f < NUMBER_OF_FLOPPIES; f++) {
                digitalWrite(PIN_CONTROL_DIRECTION + f * 2, DIRECTION_FORWARDS);
                for (int i = 0; i < 20; i++) {
                  digitalWrite(PIN_CONTROL_STEP + f * 2, HIGH);
                  delay(5);
                  digitalWrite(PIN_CONTROL_STEP + f * 2, LOW);
                  delay(5);
                }
              }
              Serial.println("move motor forwards");
              break;
            case 'm': // set the max motor movement
              incomingInt = Serial.parseInt();
              if (incomingInt > 0) {
                maxMotorMovement = incomingInt;
                Serial.print("maxMotorMovement: ");
                Serial.println(maxMotorMovement);
                currentPosition = 0;
              }
              break;
            case 'p': // play / pause
              if (autoPlay) {
                Serial.println("pause");
                autoPlay = false;
                playCurrentNote = false;
              } else {
                Serial.println("play");
                autoPlay = true;
                autoPlayingNote = false;
                noteChangeCounter = 0;
              }
              break;
            case 's': // stop
              Serial.println("stop");
              autoPlay = false;
              playCurrentNote = false;
              noteListIndex = 0;
              break;
            case '+': // faster playing
              if (noteDurationMs - 10 > 0) {
                Serial.println("faster");
                noteDurationMs = noteDurationMs - 10;
                noteChangeThreshold = noteDurationMs * 1000UL / CYCLE_PERIOD;
              } else {
                Serial.println("no faster");
              }
              break;
            case '-': // slower playing
              if (noteDurationMs + 10 < 32000) {
                Serial.println("slower");
                noteDurationMs = noteDurationMs + 10;
                noteChangeThreshold = noteDurationMs * 1000UL / CYCLE_PERIOD;
              } else {
                Serial.println("no slower");
              }
              break;
          }
          break;
      }
    }
  }
  
}

void printCurrentNote() {
  if (USE_SERIAL) {
    switch (currentPeriod) {
      case A_5:  Serial.println("A5");  break;
      case Ab_5: Serial.println("Ab5"); break;
      case G_5:  Serial.println("G5");  break;
      case Gb_5: Serial.println("Gb5"); break;
      case F_5:  Serial.println("F5");  break;
      case E_5:  Serial.println("E5");  break;
      case Eb_5: Serial.println("Eb5"); break;
      case D_5:  Serial.println("D5");  break;
      case Db_5: Serial.println("Db5"); break;
      case C_5:  Serial.println("C5");  break;
      case B_4:  Serial.println("B4");  break;
      case Bb_4: Serial.println("Bb4"); break;
      case A_4:  Serial.println("A4");  break;
      case Ab_4: Serial.println("Ab4"); break;
      case G_4:  Serial.println("G4");  break;
      case Gb_4: Serial.println("Gb4"); break;
      case F_4:  Serial.println("F4");  break;
      case E_4:  Serial.println("E4");  break;
      case Eb_4: Serial.println("Eb4"); break;
      case D_4:  Serial.println("D4");  break;
      case Db_4: Serial.println("Db4"); break;
      case C_4:  Serial.println("C4");  break;
      case B_3:  Serial.println("B3");  break;
      case Bb_3: Serial.println("Bb3"); break;
      case A_3:  Serial.println("A3");  break;
      case Ab_3: Serial.println("Ab3"); break;
      case G_3:  Serial.println("G3");  break;
      case Gb_3: Serial.println("Gb3"); break;
      case F_3:  Serial.println("F3");  break;
      case E_3:  Serial.println("E3");  break;
      case Eb_3: Serial.println("Eb3"); break;
      case D_3:  Serial.println("D3");  break;
      case Db_3: Serial.println("Db3"); break;
      case C_3:  Serial.println("C3");  break;
      case B_2:  Serial.println("B2");  break;
      case Bb_2: Serial.println("Bb2"); break;
      case A_2:  Serial.println("A2");  break;
      
      // special case
      case REST: Serial.println("REST");break;
    }    
  }
}
