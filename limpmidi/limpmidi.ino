const int PIN_LED = 13;
const int PIN_CONTROL_STEP = 10;
const int PIN_CONTROL_DIRECTION = 9;
const int MAX_POSITION = 80;

const int DIRECTION_BACKWARDS = HIGH;
const int DIRECTION_FORWARDS = LOW;

unsigned long interruptCounter = 0;
unsigned long currentPosition = 0;
unsigned long tick = 0;
unsigned long currentPeriod = 0;
unsigned long noteChangeCounter = 0;

void setup() {
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
    digitalWrite(PIN_CONTROL_STEP, HIGH);
    digitalWrite(PIN_CONTROL_STEP, LOW);
    delay(10);
  }
  digitalWrite(PIN_LED, LOW);
  delay(1000);
  
  currentPeriod = 35; // possible start note
  
}

void loop() {
  
  /*interruptCounter++;
  // 40us * 10000 = 400ms
  if (interruptCounter == 10000) {
    digitalWrite(PIN_LED, (digitalRead(PIN_LED) == LOW) ? HIGH : LOW);
    interruptCounter = 0;
  }*/

  // move motor every n ticks
  tick++;
  if (tick == currentPeriod) {
    digitalWrite(PIN_CONTROL_STEP, HIGH);
    digitalWrite(PIN_CONTROL_STEP, LOW);
    currentPosition++;
    tick = 0;
  }
  // toggle position
  if (currentPosition == MAX_POSITION - 1) {
    digitalWrite(PIN_CONTROL_DIRECTION, (digitalRead(PIN_CONTROL_DIRECTION) == LOW) ? HIGH : LOW);
    currentPosition = 0;
  }
  
  noteChangeCounter++;
  if (noteChangeCounter == 30000) {
    digitalWrite(PIN_LED, (digitalRead(PIN_LED) == LOW) ? HIGH : LOW);
    noteChangeCounter = 0;
    currentPeriod++;
    if (currentPeriod == 43) { // possible end note
      currentPeriod = 35;
    }
  }
  
  delayMicroseconds(40);
}
