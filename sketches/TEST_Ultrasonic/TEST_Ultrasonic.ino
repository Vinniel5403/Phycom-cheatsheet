const int PIN_TRIG = 12;
const int PIN_ECHO = 13;

float readDistanceCM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 25000UL);  // timeout ~4 m
  if (us == 0) return 999.0;                            // ไม่มี echo
  return us * 0.0343 / 2.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
}

void loop() {
  float cm = readDistanceCM();
  Serial.print("distance = ");
  Serial.print(cm, 1);
  Serial.println(" cm");
  delay(200);
}
