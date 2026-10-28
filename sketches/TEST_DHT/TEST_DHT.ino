#include <DHT.h>

const int PIN_DHT = A1;
DHT dht(PIN_DHT, DHT11);   // หรือ DHT22

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("อ่าน DHT ไม่ได้ (เช็กชนิด/สาย)");
  } else {
    Serial.print("T=");
    Serial.print(t, 1);
    Serial.print(" C  H=");
    Serial.print(h, 0);
    Serial.println(" %");
  }
  delay(2000);
}
