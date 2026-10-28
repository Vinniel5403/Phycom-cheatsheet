/*
 * Physical Computing — MQTT exam sketch
 * Board: Arduino UNO R4 WiFi   |   Serial Monitor: 115200
 * Libraries: WiFiS3 (มากับ board), ArduinoMqttClient, Keypad (Mark Stanley), LiquidCrystal (มากับ IDE)
 *
 * Tasks:
 *  [1] PUB 68070006/key : Keypad พิมพ์แล้ว # ส่ง, * ลบ
 *  [2] SUB 68070006/message : ข้อความ → LCD แถว 2
 *  [3] PUB 68070006/light : Potentiometer
 *
 * Wiring:
 *  Keypad 4x4: R1->D9, R2->D8, R3->D7, R4->D6, C1->D5, C2->D4, C3->D3, C4->D2 | สายซ้าย->ขวา = R1..R4,C1..C4
 *  LCD 16x2: RS->D10, E->D11, D4->D12, D5->D13, D6->A1, D7->A2 | VSS,RW,K->GND  VDD,A->5V  V0->pot/R1k->GND
 *  Potentiometer: POT->A0 | ขาข้าง->5V/GND
 */
#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

// ================== CONFIG (แก้ตรงนี้) ==================
char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PASS";
const char broker[] = "phycom.it.kmitl.ac.th";
const int  port = 1883;
const String SID = "68070006";     // รหัสนักศึกษา = prefix ของ topic
const unsigned long PUB_INTERVAL = 1000;   // ms ส่งค่าซ้ำทุกๆ
const unsigned long SENSE_INTERVAL = 100;   // ms อ่าน sensor ทุกๆ
const bool DEBUG = true;                     // พิมพ์สถานะทั้งหมดใน Serial
const unsigned long DEBUG_INTERVAL = 1000;   // ms

// ================== PINS ==================
const int PIN_POT = A0;
char keys[4][4] = {{'1','2','3','A'}, {'4','5','6','B'}, {'7','8','9','C'}, {'*','0','#','D'}};
byte rowPins[4] = {9, 8, 7, 6};   // R1..R4
byte colPins[4] = {5, 4, 3, 2};   // C1..C4
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);
LiquidCrystal lcd(10, 11, 12, 13, A1, A2);   // RS, E, D4, D5, D6, D7

WiFiClient net;
MqttClient mqtt(net);

// ================== STATE ==================
String lastVal[3];
unsigned long lastPub[3];

// ================== HELPERS ==================
void publish(const String& sub, const String& payload) {
  String topic = SID + "/" + sub;
  mqtt.beginMessage(topic);
  mqtt.print(payload);
  mqtt.endMessage();
  Serial.print("PUB "); Serial.print(topic); Serial.print(" = "); Serial.println(payload);
}

// ส่งเมื่อครบเวลา PUB_INTERVAL หรือ (onChange) เมื่อค่าเปลี่ยน
void pubIf(int i, const char* sub, const String& v, bool onChange) {
  if (v.length() == 0) return;
  bool due = (lastPub[i] == 0) || (millis() - lastPub[i] >= PUB_INTERVAL);
  if (due || (onChange && v != lastVal[i])) {
    publish(sub, v);
    lastVal[i] = v;
    lastPub[i] = millis();
  }
}

int potValue() { return analogRead(PIN_POT); }   // 0..1023

String lcdLast[2];
void lcdPrintRow(int row, String s) {   // เขียนทั้งแถว 16 ตัว (เติมช่องว่าง)
  if (s.length() > 16) s = s.substring(0, 16);
  while (s.length() < 16) s += ' ';
  if (s == lcdLast[row]) return;
  lcdLast[row] = s;
  lcd.setCursor(0, row);
  lcd.print(s);
}

String keyBuf = "";
void handleKey(char k) {
  Serial.print("KEY "); Serial.println(k);
  // [1] PUB 68070006/key : Keypad พิมพ์แล้ว # ส่ง, * ลบ
  if (k == '#') {
    publish("key", keyBuf);
    lastVal[0] = keyBuf;
    lcdPrintRow(0, "Sent: " + keyBuf);
    keyBuf = "";
  } else if (k == '*') {
    keyBuf = "";
    lcdPrintRow(0, "");
  } else {
    keyBuf += k;
    lcdPrintRow(0, "> " + keyBuf);
  }
}

// ================== SENSORS ==================
void sense() {
}

// ================== PUBLISH ==================
void doPublishes() {
  String v;
  // [3] PUB 68070006/light : Potentiometer
  v = String(potValue());
  pubIf(2, "light", v, false);
}

// ================== DEBUG LOG ==================
// พิมพ์สถานะทุกอย่างทุก DEBUG_INTERVAL ms (ปิด: DEBUG = false)
void debugPrint() {
  static unsigned long t = 0;
  if (!DEBUG || millis() - t < DEBUG_INTERVAL) return;
  t = millis();
  Serial.print("------ DBG t="); Serial.print(millis() / 1000.0, 1); Serial.print("s  WiFi=");
  if (WiFi.status() == WL_CONNECTED) { Serial.print("OK "); Serial.print(WiFi.RSSI()); Serial.print("dBm"); } else Serial.print("DOWN");
  Serial.print("  MQTT="); Serial.println(mqtt.connected() ? "OK" : "DOWN");
  Serial.print("  SENSOR ");
  Serial.print(" pot="); Serial.print(potValue());
  Serial.print(" keyBuf='"); Serial.print(keyBuf); Serial.print("'");
  Serial.println();
  Serial.print("  PUB    ");
  Serial.print(" [1]key="); Serial.print(lastVal[0]);
  Serial.print(" [3]light="); Serial.print(lastVal[2]);
  Serial.println();
  Serial.print("  SUB    ");
  Serial.print(" [2]message="); Serial.print(lastVal[1]);
  Serial.println();
  Serial.print("  OUT    ");
  Serial.print(" LCD1='"); Serial.print(lcdLast[0]); Serial.print("'");
  Serial.print(" LCD2='"); Serial.print(lcdLast[1]); Serial.print("'");
  Serial.println();
}

void printBanner() {
  Serial.println();
  Serial.println("===== Phycom exam sketch =====");
  Serial.print("SID = "); Serial.print(SID); Serial.print("   broker = "); Serial.print(broker); Serial.print(":"); Serial.println(port);
  Serial.println("[1] PUB 68070006/key : Keypad พิมพ์แล้ว # ส่ง, * ลบ");
  Serial.println("[2] SUB 68070006/message : ข้อความ → LCD แถว 2");
  Serial.println("[3] PUB 68070006/light : Potentiometer");
  Serial.println("WIRE Keypad 4x4: R1->D9, R2->D8, R3->D7, R4->D6, C1->D5, C2->D4, C3->D3, C4->D2");
  Serial.println("WIRE LCD 16x2: RS->D10, E->D11, D4->D12, D5->D13, D6->A1, D7->A2");
  Serial.println("WIRE Potentiometer: POT->A0");
  Serial.println("==============================");
}

// ================== SUBSCRIBE ==================
void onMqttMessage(int messageSize) {
  String topic = mqtt.messageTopic();
  String msg = "";
  while (mqtt.available()) msg += (char)mqtt.read();
  msg.trim();
  Serial.print("SUB "); Serial.print(topic); Serial.print(" = "); Serial.println(msg);

  // [2] SUB 68070006/message : ข้อความ → LCD แถว 2
  if (topic == SID + "/message") {
    lastVal[1] = msg;
    lcdPrintRow(1, msg);
  }
}

// ================== CONNECTION ==================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("WiFi: connecting to "); Serial.println(ssid);
  lcdPrintRow(0, "WiFi...");
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(2000);
  }
  while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) delay(200);
  Serial.print("\nWiFi OK, IP = "); Serial.println(WiFi.localIP());
}

void connectMQTT() {
  lcdPrintRow(0, "MQTT...");
  mqtt.setId("R4_" + SID + "_" + String(micros() & 0xFFFF, HEX));
  while (!mqtt.connect(broker, port)) {
    Serial.print("MQTT failed, error = "); Serial.println(mqtt.connectError());
    delay(2000);
    connectWiFi();
  }
  Serial.println("MQTT connected");
  mqtt.onMessage(onMqttMessage);
  mqtt.subscribe(SID + "/message");
  lcdPrintRow(0, "Ready"); lcdPrintRow(1, "");
}

// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);
  delay(1000);
  lcd.begin(16, 2);
  printBanner();
  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.poll();                       // ต้องเรียกบ่อยๆ ถึงจะรับ subscribe ได้
  char k = keypad.getKey();
  if (k) handleKey(k);

  static unsigned long tSense = 0;
  if (millis() - tSense >= SENSE_INTERVAL) {
    tSense = millis();
    sense();
    doPublishes();
  }
  debugPrint();
}
