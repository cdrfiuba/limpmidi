const bool USE_SERIAL = true;
const long int SERIAL_BPS = 115200;

const int PIN_LED = 13;
const int PIN_CONTROL_STEP = 10; // rojo
const int PIN_CONTROL_DIRECTION = 9; // azul
const int MAX_POSITION = 2; // fd has 80 tracks

const unsigned int PERIOD_START = 1; // must be larger than 0
const unsigned int PERIOD_END = 1000;

// 440Hz
int A  = 42; // 1
int Ab = 00; // 2
int G  = 00; // 3
int Gb = 00; // 4
int F  = 00; // 5
int E  = 57; // 6
int Eb = 00; // 7
int D  = 00; // 8
int Db = 00; // 9
int C  = 00; // 10
int B  = 00; // 11
int A  = 84; // 12
// 220Hz

//const PERIOD_LIST[] = 

const int CYCLE_PERIOD = 22; // microseconds
const unsigned long NOTE_CHANGE_THRESHOLD_MS = 200; // ms
const unsigned long NOTE_CHANGE_THRESHOLD = NOTE_CHANGE_THRESHOLD_MS * 1000 / CYCLE_PERIOD; //300ms


const int DIRECTION_BACKWARDS = HIGH;
const int DIRECTION_FORWARDS = LOW;

unsigned long interruptCounter = 0;
unsigned long currentPosition = 0;
unsigned long tick = 0;
unsigned long noteChangeCounter = 0;
unsigned int currentPeriod = 0;

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

  delay(1000);
  
  currentPeriod = PERIOD_START; // possible start note
  
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
  /*noteChangeCounter++;
  if (noteChangeCounter == NOTE_CHANGE_THRESHOLD) {
    digitalWrite(PIN_LED, (digitalRead(PIN_LED) == LOW) ? HIGH : LOW);
    noteChangeCounter = 0;
    //currentPeriod = currentPeriod + 1;
    //if (currentPeriod > PERIOD_END) { // possible end note
    //  currentPeriod = PERIOD_START;
    //}
    currentPeriod = random(PERIOD_START, PERIOD_END);
    tick = 0;
    if (USE_SERIAL) {
      debug("%i ", currentPeriod);
      Serial.print("\n");
    }
  }*/

  if (USE_SERIAL) {
    if (Serial.available() > 0) {
      //incomingByte = Serial.read();
      //if (incomingByte == 'n') {
        incomingInt = Serial.parseInt();
        if (incomingInt >= PERIOD_START && incomingInt <= PERIOD_END) {
          Serial.println(incomingInt);
          currentPeriod = incomingInt;
          tick = 0;
        }
      //}
    }
  }
  
  //delay(10);
  delayMicroseconds(CYCLE_PERIOD);
}
