#include <WiFiS3.h>
#include <ArduinoMqttClient.h>

char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PASS";
const char broker[] = "phycom.it.kmitl.ac.th";
const int port = 1883;
const String SID = "68070006";

WiFiClient net;
MqttClient mqtt(net);

void onMqttMessage(int size) {
  String topic = mqtt.messageTopic();
  String msg = "";
  while (mqtt.available()) msg += (char)mqtt.read();
  Serial.print("SUB ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(115200);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(2000);
  }
  while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) delay(200);
  Serial.print("\nIP: ");
  Serial.println(WiFi.localIP());

  mqtt.setId("R4_" + SID + "_" + String(micros() & 0xFFFF, HEX));
  while (!mqtt.connect(broker, port)) {
    Serial.print("MQTT error ");
    Serial.println(mqtt.connectError());
    delay(2000);
  }
  Serial.println("MQTT OK");
  mqtt.onMessage(onMqttMessage);
  mqtt.subscribe(SID + "/#");
}

void loop() {
  mqtt.poll();
  static unsigned long t = 0;
  static int count = 0;
  if (millis() - t >= 2000) {
    t = millis();
    mqtt.beginMessage(SID + "/test");
    mqtt.print(count++);
    mqtt.endMessage();
    Serial.println("PUB test");
  }
}
