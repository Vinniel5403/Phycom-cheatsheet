#include <Wire.h>

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Wire.begin();
  Serial.println("Scanning I2C (SDA=A4, SCL=A5)...");
  int found = 0;
  for (byte a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print("found 0x");
      Serial.println(a, HEX);
      found++;
    }
  }
  if (!found) Serial.println("ไม่เจอ: เช็ก SDA/SCL/VCC");
}

void loop() {}
