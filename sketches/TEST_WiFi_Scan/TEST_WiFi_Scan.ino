#include <WiFiS3.h>

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Serial.print("WiFi firmware: ");
  Serial.println(WiFi.firmwareVersion());
  Serial.println("Scanning...");
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    Serial.print(i + 1);
    Serial.print(") ");
    Serial.print(WiFi.SSID(i));
    Serial.print("  RSSI=");
    Serial.println(WiFi.RSSI(i));
  }
  Serial.println("ไม่เห็นชื่อ hotspot = เป็น 5 GHz หรืออยู่ไกล");
}

void loop() {}
