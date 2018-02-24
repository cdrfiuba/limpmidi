const bool USE_SERIAL = true;
const long int SERIAL_BPS = 115200;

const int PIN_LED = 13;
const int PIN_CONTROL_STEP = 10; // rojo
const int PIN_CONTROL_DIRECTION = 9; // azul
const int MAX_POSITION = 2; // fd has 80 tracks

const unsigned int PERIOD_START = 1; // must be larger than 0
const unsigned int PERIOD_END = 1000;

const int REST = 0;  // silence
const int B_4  = 38; // 494 Hz
const int Bb_4 = 40; // 466 Hz
const int A_4  = 42; // 440 Hz
const int Ab_4 = 45; // 415 Hz
const int G_4  = 48; // 392 Hz
const int Gb_4 = 51; // 370 Hz
const int F_4  = 54; // 349 Hz
const int E_4  = 57; // 329 Hz
const int Eb_4 = 60; // 311 Hz
const int D_4  = 64; // 294 Hz
const int Db_4 = 68; // 277 Hz
const int C_4  = 72; // 262 Hz
const int B_3  = 76; // 247 Hz
const int Bb_3 = 80; // 233 Hz
const int A_3  = 84; // 220 Hz
const int END = PERIOD_END;  // loop

const bool PLAY_NOTES = true;
const int NOTE_LIST[] = {
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, E_4, REST, D_4, REST,
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, D_4, REST, C_4, REST,
  D_4, REST, E_4, C_4, D_4, F_4, E_4, C_4, D_4, F_4, E_4, C_4, D_4, E_4, G_4, REST,
  E_4, E_4, F_4, G_4, G_4, F_4, E_4, D_4, C_4, C_4, D_4, E_4, D_4, REST, C_4, REST,
  REST, END
};
int noteListIndex = 0;

const int CYCLE_PERIOD = 22; // microseconds
const unsigned long NOTE_CHANGE_THRESHOLD_MS = 100; // ms
const unsigned long NOTE_CHANGE_THRESHOLD = NOTE_CHANGE_THRESHOLD_MS * 1000 / CYCLE_PERIOD; //300ms


const int DIRECTION_BACKWARDS = HIGH;
const int DIRECTION_FORWARDS = LOW;

unsigned long interruptCounter = 0;
unsigned long currentPosition = 0;
unsigned long tick = 0;
unsigned long noteChangeCounter = 0;
unsigned long noteSilenceCounter = 0;
unsigned int currentPeriod = 0;
int nextNote = 0;

byte incomingByte = 0;
unsigned int incomingInt = 0;

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
  
  randomSeed(analogRead(0)); // init random seed
  
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_CONTROL_STEP, OUTPUT);
  pinMode(PIN_CONTROL_DIRECTION, OUTPUT);
  
  delay(1000);
  digitalWrite(PIN_LED, HIGH);
  
  // reset position: set direction to backwards until the 0 position 
  // is reached
  // 5ms * (158 / 2) = 395ms
  digitalWrite(PIN_CONTROL_DIRECTION, DIRECTION_BACKWARDS);
  for (int i = 0; i < MAX_POSITION; i++) {
    digitalWrite(PIN_CONTROL_STEP, LOW);
    digitalWrite(PIN_CONTROL_STEP, HIGH);
    delay(10);
  }
  digitalWrite(PIN_CONTROL_DIRECTION, DIRECTION_FORWARDS);
  for (int i = 0; i < MAX_POSITION / 2; i++) {
    digitalWrite(PIN_CONTROL_STEP, LOW);
    digitalWrite(PIN_CONTROL_STEP, HIGH);
    delay(10);
  }
  digitalWrite(PIN_LED, LOW);

  currentPeriod = 0;
  
  delay(2000);
  
}

void loop() {
  
  /*interruptCounter++;
  // 40us * 10000 = 400ms
  if (interruptCounter == 10) {
    digitalWrite(PIN_LED, (digitalRead(PIN_LED) == LOW) ? HIGH : LOW);
    interruptCounter = 0;
  }*/

  // move motor every n ticks
  tick++;
  if (tick == currentPeriod) {
    digitalWrite(PIN_CONTROL_STEP, HIGH);
    digitalWrite(PIN_CONTROL_STEP, LOW);
    //digitalWrite(PIN_LED, (digitalRead(PIN_LED) == LOW) ? HIGH : LOW);
    currentPosition++;
    tick = 0;
  }
  // toggle position
  if (currentPosition == MAX_POSITION - 1) {
    digitalWrite(PIN_CONTROL_DIRECTION, (digitalRead(PIN_CONTROL_DIRECTION) == LOW) ? HIGH : LOW);
    currentPosition = 0;
  }
  
  // every 40ms, change note
  if (PLAY_NOTES) {
    noteChangeCounter++;
    if (noteChangeCounter == NOTE_CHANGE_THRESHOLD * 2) {
      digitalWrite(PIN_LED, (digitalRead(PIN_LED) == LOW) ? HIGH : LOW);
      noteChangeCounter = 0;

      currentPeriod = NOTE_LIST[noteListIndex];
      noteListIndex++;
      if (currentPeriod == END) {
        noteListIndex = 0;
        currentPeriod = NOTE_LIST[noteListIndex];
        noteListIndex++;
      }
      
      printCurrentNote();
      
      tick = 0;
      noteSilenceCounter = 0;
      //if (USE_SERIAL) {
      //  debug("%i ", currentPeriod);
      //  Serial.print("\n");
      //}
    }
  }
  
  // if a note is playing, every NOTE_CHANGE_THRESHOLD_MS ms, silence note
  if (currentPeriod > 0) {
    noteSilenceCounter++;
    if (noteSilenceCounter == NOTE_CHANGE_THRESHOLD) {
      noteSilenceCounter = 0;
      currentPeriod = 0;
      tick = 0;
      if (USE_SERIAL) {
        Serial.println("rest");
      }
    }
  }  

  if (USE_SERIAL) {
    if (Serial.available() > 0) {
      //incomingInt = Serial.parseInt();
      //if (incomingInt >= PERIOD_START && incomingInt <= PERIOD_END) {
      //  Serial.println(incomingInt);
      //  currentPeriod = incomingInt;
      //  tick = 0;
      //}
      incomingByte = Serial.read();
      switch (incomingByte) {
        case 'm': currentPeriod = B_4;  if (USE_SERIAL) {Serial.println("B4");}  break;
        case 'j': currentPeriod = Bb_4; if (USE_SERIAL) {Serial.println("Bb4");} break;
        case 'n': currentPeriod = A_4;  if (USE_SERIAL) {Serial.println("A4");}  break;
        case 'h': currentPeriod = Ab_4; if (USE_SERIAL) {Serial.println("Ab4");} break;
        case 'b': currentPeriod = G_4;  if (USE_SERIAL) {Serial.println("G4");}  break;
        case 'g': currentPeriod = Gb_4; if (USE_SERIAL) {Serial.println("Gb4");} break;
        case 'v': currentPeriod = F_4;  if (USE_SERIAL) {Serial.println("F4");}  break;
        
        case 'c': currentPeriod = E_4;  if (USE_SERIAL) {Serial.println("E4");}  break;
        case 'd': currentPeriod = Eb_4; if (USE_SERIAL) {Serial.println("Eb4");} break;
        case 'x': currentPeriod = D_4;  if (USE_SERIAL) {Serial.println("D4");}  break;
        case 's': currentPeriod = Db_4; if (USE_SERIAL) {Serial.println("Db4");} break;
        case 'z': currentPeriod = C_4;  if (USE_SERIAL) {Serial.println("C4");}  break;
        
        case 'p': currentPeriod = E_4;  if (USE_SERIAL) {Serial.println("E4");}  break;
        case '0': currentPeriod = Eb_4; if (USE_SERIAL) {Serial.println("Eb4");} break;
        case 'o': currentPeriod = D_4;  if (USE_SERIAL) {Serial.println("D4");}  break;
        case '9': currentPeriod = Db_4; if (USE_SERIAL) {Serial.println("Db4");} break;
        case 'i': currentPeriod = C_4;  if (USE_SERIAL) {Serial.println("C4");}  break;
        
        case 'u': currentPeriod = B_3;  if (USE_SERIAL) {Serial.println("B3");}  break;
        case '7': currentPeriod = Bb_3; if (USE_SERIAL) {Serial.println("Bb3");} break;
        case 'y': currentPeriod = A_3;  if (USE_SERIAL) {Serial.println("A3");}  break;
      }
      tick = 0;
      noteSilenceCounter = 0;
    }
  }
  
  //delay(10);
  delayMicroseconds(CYCLE_PERIOD);
}

void printCurrentNote() {
  if (USE_SERIAL) {
    switch (currentPeriod) {
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
    }
    
  }
}
