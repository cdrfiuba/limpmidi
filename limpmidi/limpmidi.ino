const int PIN_BLUE_LED = 13;
const int PIN_CONTROL_STEP = 10;
const int PIN_CONTROL_DIRECTION = 9;

void setup() {
  pinMode(PIN_BLUE_LED, OUTPUT);
  pinMode(PIN_CONTROL_STEP, OUTPUT);
  pinMode(PIN_CONTROL_DIRECTION, OUTPUT);
}

void loop() {
  for (int i = 0; i <= 158; i++) {
    if (i == 0) {
      digitalWrite(PIN_BLUE_LED, (digitalRead(PIN_BLUE_LED) == LOW) ? HIGH : LOW);
      digitalWrite(PIN_CONTROL_DIRECTION, (digitalRead(PIN_CONTROL_DIRECTION) == LOW) ? HIGH : LOW);
    }
    if (i % 80 == 0) {
      digitalWrite(PIN_CONTROL_DIRECTION, (digitalRead(PIN_CONTROL_DIRECTION) == LOW) ? HIGH : LOW);
    }
    digitalWrite(PIN_CONTROL_STEP, (digitalRead(PIN_CONTROL_STEP) == LOW) ? HIGH : LOW);
    delayMicroseconds(30);
  }
}
