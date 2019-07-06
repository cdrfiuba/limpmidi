#include <TimerOne.h>
#include <MIDIUSB.h>
#include <pitchToNote.h>

const bool USE_SERIAL = true;
const long int SERIAL_BPS = 115200;
const bool USE_MIDI = false;

const int PIN_LED = 13;
const int PIN_CONTROL_STEP = 1; // rojo
const int PIN_CONTROL_DIRECTION = 0; // azul
const unsigned char NUMBER_OF_FLOPPIES = 4;
const int MAX_POSITION = 80; // fd has 80 tracks
const unsigned int INITIAL_MAX_MOTOR_MOVEMENT = 2; // 160 produces full movement, 2 makes louder sound
unsigned int maxMotorMovement = INITIAL_MAX_MOTOR_MOVEMENT;

const int DIRECTION_BACKWARDS = HIGH;
const int DIRECTION_FORWARDS = LOW;

const unsigned int CYCLE_PERIOD = 31; // microseconds

const unsigned char REST = 0;   // silence

const unsigned char B_4  = 33;  // 493.8833 Hz
const unsigned char Bb_4 = 35;  // 466.1638 Hz
const unsigned char A_4  = 37;  // 440.0000 Hz
const unsigned char Ab_4 = 39;  // 415.3047 Hz
const unsigned char G_4  = 41;  // 391.9954 Hz
const unsigned char Gb_4 = 44;  // 369.9944 Hz
const unsigned char F_4  = 46;  // 349.2282 Hz
const unsigned char E_4  = 49;  // 329.6276 Hz
const unsigned char Eb_4 = 52;  // 311.1270 Hz
const unsigned char D_4  = 55;  // 293.6648 Hz
const unsigned char Db_4 = 58;  // 277.1826 Hz
const unsigned char C_4  = 62;  // 261.6256 Hz

const unsigned char B_3  = B_4 * 2;
const unsigned char Bb_3 = Bb_4 * 2;
const unsigned char A_3  = A_4 * 2;
const unsigned char Ab_3 = Ab_4 * 2;
const unsigned char G_3  = G_4 * 2;
const unsigned char Gb_3 = Gb_4 * 2;
const unsigned char F_3  = F_4 * 2;
const unsigned char E_3  = E_4 * 2;
const unsigned char Eb_3 = Eb_4 * 2;
const unsigned char D_3  = D_4 * 2;
const unsigned char Db_3 = Db_4 * 2;
const unsigned char C_3  = C_4 * 2;

const unsigned char B_2  = B_3 * 2;
const unsigned char Bb_2 = Bb_3 * 2;
const unsigned char A_2  = A_3 * 2;
const unsigned char Ab_2 = Ab_3 * 2;
const unsigned char G_2  = G_3 * 2;
const unsigned char Gb_2 = Gb_3 * 2;
const unsigned char F_2  = F_3 * 2;
const unsigned char E_2  = E_3 * 2;
const unsigned char Eb_2 = Eb_3 * 2;
const unsigned char D_2  = D_3 * 2;
const unsigned char Db_2 = Db_3 * 2;
const unsigned char C_2  = C_3 * 2;

const unsigned char END = 255;  // loop

// midi constants
const unsigned char MESSAGE_NOTE_OFF = 0b1000;
const unsigned char MESSAGE_NOTE_ON = 0b1001;
const unsigned char MASK_MESSAGE_CHANNEL = 0b00001111;

const bool INITIAL_AUTO_PLAY = false;
bool autoPlay = INITIAL_AUTO_PLAY;

const int NOTE_LIST[] = {
  A_3, END
};
/*const int NOTE_LIST[] = {
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, E_4, REST, D_4, REST,
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, D_4, REST, C_4, REST,
  D_4, REST, E_4, C_4, D_4, F_4, E_4, C_4, D_4, F_4, E_4, C_4, D_4, E_4, G_4, REST,
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, D_4, REST, C_4, REST,
  REST, REST, END
};*/
/*const int NOTE_LIST[] = {
  C_3, Eb_3, G_3, Eb_3, C_3, Eb_3, G_3, Eb_3,
  C_3, Eb_3, Ab_3, Eb_3, C_3, Eb_3, Ab_3, Eb_3,
  D_3, F_3, Bb_3, F_3, D_3, F_3, Bb_3, F_3,
  END
};*/
unsigned int noteListIndex = 0;

const unsigned int INITIAL_NOTE_DURATION_MS = 170; // ms
const unsigned long INITIAL_NOTE_CHANGE_THRESHOLD = INITIAL_NOTE_DURATION_MS * 1000UL / CYCLE_PERIOD;
unsigned int noteDurationMs = INITIAL_NOTE_DURATION_MS;
unsigned long noteChangeThreshold = INITIAL_NOTE_CHANGE_THRESHOLD;

volatile bool playCurrentNote[NUMBER_OF_FLOPPIES] = {};
volatile bool currentDirection[NUMBER_OF_FLOPPIES] = {};
volatile bool currentStep[NUMBER_OF_FLOPPIES] = {};
volatile unsigned char currentPosition[NUMBER_OF_FLOPPIES] = {};
volatile unsigned char currentPeriod[NUMBER_OF_FLOPPIES] = {};
volatile unsigned char tick[NUMBER_OF_FLOPPIES] = {};
volatile unsigned long noteChangeCounter = 0;

bool autoPlayingNote = 0;

const unsigned char AUTO_PLAY_FLOPPY = 0;
const unsigned char FIRST_MANUAL_PLAY_FLOPPY = 0;
unsigned char currentFloppy = FIRST_MANUAL_PLAY_FLOPPY;

byte incomingByte = 0;
byte specialByte = 0;
unsigned int incomingInt = 0;
unsigned char nextPeriod = 0;
unsigned char initialFloppy = 0;
bool availableFloppyFound = false;

char debug_string_buffer[20];
// sprintf + serial of 20 bytes takes ~200us
// sprintf + serial of 10 bytes takes ~144us
// sprintf + serial of  5 bytes takes ~108us
#define debug(formato, valor) \
  sprintf(debug_string_buffer, formato, valor); \
  Serial.print(debug_string_buffer)

void setup() {
  
  //randomSeed(analogRead(0)); // init random seed
  
  pinMode(PIN_LED, OUTPUT);
  for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
    pinMode(PIN_CONTROL_STEP + f * 2, OUTPUT);
    pinMode(PIN_CONTROL_DIRECTION + f * 2, OUTPUT);
  }
  
  //delay(1000);
  digitalWrite(PIN_LED, HIGH);
  
  // reset position: set direction to backwards until the 0 position 
  // is reached
  for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
    digitalWrite(PIN_CONTROL_DIRECTION + f * 2, DIRECTION_BACKWARDS);
    for (int i = 0; i < MAX_POSITION; i++) {
      digitalWrite(PIN_CONTROL_STEP + f * 2, HIGH);
      delay(2);
      digitalWrite(PIN_CONTROL_STEP + f * 2, LOW);
      delay(2);
    }
  }
  // set direction to forwards until the max position, and a bit beyond
  for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
    digitalWrite(PIN_CONTROL_DIRECTION + f * 2, DIRECTION_FORWARDS);
    for (int i = 0; i < MAX_POSITION + 2; i++) {
     digitalWrite(PIN_CONTROL_STEP + f * 2, HIGH);
     delay(2);
     digitalWrite(PIN_CONTROL_STEP + f * 2, LOW);
     delay(2);
    }
  }
  digitalWrite(PIN_LED, LOW);

  // array inits
  for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
    playCurrentNote[f] = 0;
    currentDirection[f] = 0;
    currentStep[f] = 0;
    currentPosition[f] = 0;
    currentPeriod[f] = 0;
    tick[f] = 0;
  }
  
  //delay(2000);
  
  Timer1.initialize(CYCLE_PERIOD);
  Timer1.attachInterrupt(noteSetter);
  Timer1.start();

  if (USE_SERIAL) {
    Serial.begin(SERIAL_BPS);
    Serial.setTimeout(10); // parseInt uses this timeout
  }
}

void noteSetter() {
  for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
    if (playCurrentNote[f]) {
      // move motor every n ticks
      tick[f]++;
      if (tick[f] == currentPeriod[f]) {
        digitalWrite(PIN_CONTROL_STEP + f * 2, currentStep[f]);
        digitalWrite(PIN_CONTROL_DIRECTION + f * 2, currentDirection[f]);
        currentStep[f] = !currentStep[f];
        currentPosition[f]++;
        tick[f] = 0;
      }
      // toggle position
      if (currentPosition[f] == maxMotorMovement) {
        currentDirection[f] = !currentDirection[f];
        currentPosition[f] = 0;
      }
    }
  }
  if (autoPlay) {
    noteChangeCounter++;
  }
}

void loop() {
  
  if (USE_MIDI) {
    midiEventPacket_t rx;
    do {
      rx = MidiUSB.read();
      if (rx.header != 0) {
        digitalWrite(PIN_LED, abs(digitalRead(PIN_LED) - 1));
        unsigned char message = (rx.byte1 >> 4) & MASK_MESSAGE_CHANNEL;
        unsigned char channel = (rx.byte1 & MASK_MESSAGE_CHANNEL) + 1;
        unsigned char note = rx.byte2;
        unsigned char velocity = rx.byte3;
        Serial.print("ch");
        Serial.print(channel);
        Serial.print(", n ");
        Serial.print(note);
        Serial.print(", v");
        Serial.print(velocity);
        Serial.print("\n");
        
        if (channel == 1 || channel == 2 || channel == 3 || channel == 4) {
          if (message == MESSAGE_NOTE_ON || message == MESSAGE_NOTE_OFF) {
            //~ Serial.print(channel);
            //~ Serial.print(": ");
            //~ Serial.print(rx.byte2);
            //~ Serial.print("@");
            //~ Serial.print(rx.byte3);
            //~ Serial.print("\n");
          }
          switch (note) {
            case pitchB5:  nextPeriod = B_4;   break;
            case pitchB5b: nextPeriod = Bb_4;  break;
            case pitchA5:  nextPeriod = A_4;   break;
            case pitchA5b: nextPeriod = Ab_4;  break;
            case pitchG5:  nextPeriod = G_4;   break;
            case pitchG5b: nextPeriod = Gb_4;  break;
            case pitchF5:  nextPeriod = F_4;   break;
            case pitchE5:  nextPeriod = E_4;   break;
            case pitchE5b: nextPeriod = Eb_4;  break;
            case pitchD5:  nextPeriod = D_4;   break;
            case pitchD5b: nextPeriod = Db_4;  break;
            case pitchC5:  nextPeriod = C_4;   break; // C4
            
            case pitchB4:  nextPeriod = B_3;   break;
            case pitchB4b: nextPeriod = Bb_3;  break;
            case pitchA4:  nextPeriod = A_3;   break;
            case pitchA4b: nextPeriod = Ab_3;  break;
            case pitchG4:  nextPeriod = G_3;   break;
            case pitchG4b: nextPeriod = Gb_3;  break;
            case pitchF4:  nextPeriod = F_3;   break;
            case pitchE4:  nextPeriod = E_3;   break;
            case pitchE4b: nextPeriod = Eb_3;  break;
            case pitchD4:  nextPeriod = D_3;   break;
            case pitchD4b: nextPeriod = Db_3;  break;
            case pitchC4:  nextPeriod = C_3;   break; // C4
            
            case pitchB3:  nextPeriod = B_2;   break;
            case pitchB3b: nextPeriod = Bb_2;  break;
            case pitchA3:  nextPeriod = A_2;   break;
            case pitchA3b: nextPeriod = Ab_2;  break;
            case pitchG3:  nextPeriod = G_2;   break;
            case pitchG3b: nextPeriod = Gb_2;  break;
            case pitchF3:  nextPeriod = F_2;   break;
            case pitchE3:  nextPeriod = E_2;   break;
            case pitchE3b: nextPeriod = Eb_2;  break;
            case pitchD3:  nextPeriod = D_2;   break;
            case pitchD3b: nextPeriod = Db_2;  break;
            case pitchC3:  nextPeriod = C_2;   break; // C3
            
            case pitchB2:  nextPeriod = B_2;   break;
            case pitchB2b: nextPeriod = Bb_2;  break;
            case pitchA2:  nextPeriod = A_2;   break;
            case pitchA2b: nextPeriod = Ab_2;  break;
            case pitchG2:  nextPeriod = G_2;   break;
            case pitchG2b: nextPeriod = Gb_2;  break;
            case pitchF2:  nextPeriod = F_2;   break;
            case pitchE2:  nextPeriod = E_2;   break;
            case pitchE2b: nextPeriod = Eb_2;  break;
            case pitchD2:  nextPeriod = D_2;   break;
            case pitchD2b: nextPeriod = Db_2;  break;
            case pitchC2:  nextPeriod = C_2;   break; // C3
          }
          if (message == MESSAGE_NOTE_ON) {
            playCurrentNote[channel - 1] = true;
            currentPeriod[channel - 1] = nextPeriod;
            tick[channel - 1] = 0;
            //~ printCurrentNote(channel - 1);
          }
          if (message == MESSAGE_NOTE_OFF) {
            playCurrentNote[channel - 1] = false;
          }
          //~ nextPeriod = REST;
            
        }
        // send back the received MIDI command
        //MidiUSB.sendMIDI(rx);
        //MidiUSB.flush();
      }
    } while (rx.header != 0);
    delay(5);
  }

  // every noteChangeThreshold ms, change note
  if (autoPlay) {
    if (!autoPlayingNote && noteChangeCounter >= noteChangeThreshold) {
      digitalWrite(PIN_LED, HIGH);
      
      autoPlayingNote = true;
      playCurrentNote[AUTO_PLAY_FLOPPY] = true;

      currentPeriod[AUTO_PLAY_FLOPPY] = NOTE_LIST[noteListIndex];
      tick[AUTO_PLAY_FLOPPY] = 0;
      
      noteListIndex++;
      if (NOTE_LIST[noteListIndex - 1] == END) {
        noteListIndex = 0;
        currentPeriod[AUTO_PLAY_FLOPPY] = NOTE_LIST[noteListIndex];
        noteListIndex++;
      }
      
      printCurrentNote(AUTO_PLAY_FLOPPY);
    }
  
    // if a note is playing, every noteChangeThreshold ms, silence note
    if (autoPlayingNote && noteChangeCounter >= noteChangeThreshold * 2) {
      digitalWrite(PIN_LED, LOW);
      
      autoPlayingNote = false;
      playCurrentNote[AUTO_PLAY_FLOPPY] = false;
      currentPeriod[AUTO_PLAY_FLOPPY] = 0;
      tick[AUTO_PLAY_FLOPPY] = 0;
      
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
            case 'u': nextPeriod = B_4;   break;
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
            case '\'': nextPeriod = Eb_4;  break;
            case '.': nextPeriod = D_4;   break;
            case 'l': nextPeriod = Db_4;  break;
            case ',': nextPeriod = C_4;   break; // C4
            case 'm': nextPeriod = B_3;   break;
            case 'j': nextPeriod = Bb_3;  break;
            case 'n': nextPeriod = A_3;   break;
            case 'h': nextPeriod = Ab_3;  break;
            case 'b': nextPeriod = G_3;   break;
            case 'g': nextPeriod = Gb_3;  break;
            case 'v': nextPeriod = F_3;   break;
            case 'c': nextPeriod = E_3;   break;
            case 'd': nextPeriod = Eb_3;  break;
            case 'x': nextPeriod = D_3;   break;
            case 's': nextPeriod = Db_3;  break;
            case 'z': nextPeriod = C_3;   break; // C3
            
            //default: playCurrentNote[MANUAL_PLAY_FLOPPY] = false; break;
          }
          if (incomingByte == '>' && nextPeriod != REST) {
            // find first available floppy
            initialFloppy = currentFloppy;
            availableFloppyFound = false;
            
            while (true) {
              currentFloppy++;
              if (currentFloppy == NUMBER_OF_FLOPPIES) currentFloppy = FIRST_MANUAL_PLAY_FLOPPY;
              
              if (playCurrentNote[currentFloppy] == false) {
                availableFloppyFound = true;
                break;
              }
              
              // after cycling through all floppies, exit
              if (initialFloppy == currentFloppy) break;
            }
            if (availableFloppyFound) {
              // after floppy is found, play note in that floppy
              playCurrentNote[currentFloppy] = true;
              currentPeriod[currentFloppy] = nextPeriod;
              tick[currentFloppy] = 0;
              printCurrentNote(currentFloppy);
              digitalWrite(PIN_LED, HIGH);
            }
          }
          if (incomingByte == '<') {
            for (unsigned char f = FIRST_MANUAL_PLAY_FLOPPY; f < NUMBER_OF_FLOPPIES; f++) {
              if (nextPeriod == currentPeriod[f] && playCurrentNote[f]) {
                playCurrentNote[f] = false;
                digitalWrite(PIN_LED, LOW);
                break;
              }
            }
          }
          nextPeriod = REST;
          break;
        case '!':
          specialByte = Serial.read();
          switch (specialByte) {
            case 'b': // move motor backwards 20 steps
              for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
                digitalWrite(PIN_CONTROL_DIRECTION + f * 2, DIRECTION_BACKWARDS);
                for (int i = 0; i < 20; i++) {
                  digitalWrite(PIN_CONTROL_STEP + f * 2, HIGH);
                  delay(2);
                  digitalWrite(PIN_CONTROL_STEP + f * 2, LOW);
                  delay(2);
                }
              }
              Serial.println("move motor backwards");
              break;
            case 'f': // move motor forwards
              for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
                digitalWrite(PIN_CONTROL_DIRECTION + f * 2, DIRECTION_FORWARDS);
                for (int i = 0; i < 20; i++) {
                  digitalWrite(PIN_CONTROL_STEP + f * 2, HIGH);
                  delay(2);
                  digitalWrite(PIN_CONTROL_STEP + f * 2, LOW);
                  delay(2);
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
                for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
                  currentPosition[f] = 0;
                }
              }
              break;
            case 'p': // play / pause
              if (autoPlay) {
                Serial.println("pause");
                autoPlay = false;
                playCurrentNote[AUTO_PLAY_FLOPPY] = false;
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
              playCurrentNote[AUTO_PLAY_FLOPPY] = false;
              noteListIndex = 0;
              break;
            case 'r': // reset notes
              Serial.println("reset");
              for (unsigned char f = 0; f < NUMBER_OF_FLOPPIES; f++) {
                playCurrentNote[f] = false;
              }
              digitalWrite(PIN_LED, LOW);
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

void printCurrentNote(unsigned char floppy) {
  if (USE_SERIAL) {
    switch (currentPeriod[floppy]) {
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
      
      // special case
      case REST: Serial.println("REST");break;
    }    
  }
}
