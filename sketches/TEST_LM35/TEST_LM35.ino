const int PIN_TEMP = A1;

float readVolt() {
  long sum = 0;
  for (int i = 0; i < 10; i++) sum += analogRead(PIN_TEMP);
  return (sum / 10.0) * 5.0 / 1023.0;
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  float v = readVolt();
  float lm35 = v * 100.0;             // LM35: 10 mV/°C
  float tmp36 = (v - 0.5) * 100.0;    // TMP36
  // NTC 10k: 5V → R10k → A1 → NTC → GND
  float adc = v / 5.0 * 1023.0;
  float rNtc = 10000.0 * adc / (1023.0 - adc);
  float ntc = 1.0 / (1.0 / 298.15 + log(rNtc / 10000.0) / 3950.0) - 273.15;
  Serial.print("V=");     Serial.print(v, 3);
  Serial.print(" LM35="); Serial.print(lm35, 1);
  Serial.print(" TMP36=");Serial.print(tmp36, 1);
  Serial.print(" NTC=");  Serial.println(ntc, 1);
  delay(500);
}
