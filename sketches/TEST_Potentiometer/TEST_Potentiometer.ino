const int PIN_POT = A0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  int raw = analogRead(PIN_POT);              // 0..1023
  int pct = map(raw, 0, 1023, 0, 100);
  float volt = raw * 5.0 / 1023.0;
  Serial.print("raw=");
  Serial.print(raw);
  Serial.print("  %=");
  Serial.print(pct);
  Serial.print("  V=");
  Serial.println(volt, 2);
  delay(200);
}
