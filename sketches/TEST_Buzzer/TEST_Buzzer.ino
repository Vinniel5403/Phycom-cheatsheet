const int PIN_BUZ = 1;

void setup() {
  pinMode(PIN_BUZ, OUTPUT);
}

void loop() {
  tone(PIN_BUZ, 1000, 200);   // passive buzzer: ความถี่ 1 kHz 200 ms
  delay(500);
  tone(PIN_BUZ, 1500, 200);
  delay(1500);
  // active buzzer: digitalWrite(PIN_BUZ, HIGH); delay(200); digitalWrite(PIN_BUZ, LOW);
}
