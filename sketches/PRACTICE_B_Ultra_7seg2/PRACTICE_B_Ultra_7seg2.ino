/*
 * Physical Computing — MQTT exam sketch
 * Board: Arduino UNO R4 WiFi   |   Serial Monitor: 115200
 * Libraries: WiFiS3 (มากับ board), ArduinoMqttClient
 *
 * Tasks:
 *  [1] PUB 68070006/distance : ระยะ Ultrasonic (cm)
 *  [2] PUB 68070006/level : ถ้า Potentiometer > 512 → "high" ไม่งั้น "low"
 *  [3] SUB 68070006/number : ตัวเลข → 7-Segment (0-99)
 *
 * Wiring:
 *  Ultrasonic HC-SR04: TRIG->D12, ECHO->D13 | VCC->5V GND->GND
 *  Potentiometer: POT->A0 | ขาข้าง->5V/GND
 *  7-Segment: a->D2, b->D3, c->D4, d->D5, e->D6, f->D7, g->D8, DIG1->A3, DIG2->A4 | common cathode, a-g ต่อขนาน 2 ตัว, COM แต่ละตัว->DIG1/DIG2, R220-1k ทุก segment
 */
#include <WiFiS3.h>
#include <ArduinoMqttClient.h>

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
const int PIN_TRIG = 12, PIN_ECHO = 13;
const int SEG_PINS[7] = {2, 3, 4, 5, 6, 7, 8};   // a b c d e f g
const int DIG_PINS[2] = {A3, A4};   // COM หลักซ้าย (สิบ), หลักขวา (หน่วย)
const bool SEG_COMMON_ANODE = false;

WiFiClient net;
MqttClient mqtt(net);

// ================== STATE ==================
float g_dist = 999;          // cm (999 = ไม่มี echo)
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

float readDistanceCM() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long us = pulseIn(PIN_ECHO, HIGH, 25000UL);   // timeout ~4 m
  if (us == 0) return 999.0;
  return us * 0.0343 / 2.0;
}

int potValue() { return analogRead(PIN_POT); }   // 0..1023

// bit0 = a ... bit6 = g
const byte SEG_DIGITS[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
int segValue = -1;   // -1 = ดับ

void segWrite(const int pins[7], byte pat) {
  for (int i = 0; i < 7; i++) {
    bool on = pat & (1 << i);
    digitalWrite(pins[i], (on ^ SEG_COMMON_ANODE) ? HIGH : LOW);
  }
}

const bool SEG_BLANK_LEADING = true;   // true: 7 แสดง " 7", false: "07"
void segShowNumber(int n) { segValue = (n >= 0 && n <= 99) ? n : -1; }   // 0..99

void digitEnable(int i, bool on) {   // CC: เปิด = LOW, CA: เปิด = HIGH
  digitalWrite(DIG_PINS[i], (on ^ SEG_COMMON_ANODE) ? LOW : HIGH);
}

// เรียกทุกรอบ loop: สลับหลักทุก 3 ms (ห้ามมี delay ยาวใน loop)
void segRefresh() {
  static unsigned long t = 0;
  static int cur = 0;
  if (micros() - t < 3000) return;
  t = micros();
  digitEnable(0, false);
  digitEnable(1, false);
  cur ^= 1;
  byte pat = 0;
  if (segValue >= 0) {
    if (cur == 0) pat = (segValue < 10 && SEG_BLANK_LEADING) ? 0 : SEG_DIGITS[segValue / 10];
    else pat = SEG_DIGITS[segValue % 10];
  }
  segWrite(SEG_PINS, pat);
  digitEnable(cur, true);
}

// ================== SENSORS ==================
void sense() {
  g_dist = readDistanceCM();
}

// ================== PUBLISH ==================
void doPublishes() {
  String v;
  // [1] PUB 68070006/distance : ระยะ Ultrasonic (cm)
  v = String(g_dist, 1);
  pubIf(0, "distance", v, false);
  // [2] PUB 68070006/level : ถ้า Potentiometer > 512 → "high" ไม่งั้น "low"
  v = (potValue() > 512) ? "high" : "low";
  pubIf(1, "level", v, true);
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
  Serial.print(" dist="); Serial.print(g_dist); Serial.print("cm");
  Serial.print(" pot="); Serial.print(potValue());
  Serial.println();
  Serial.print("  PUB    ");
  Serial.print(" [1]distance="); Serial.print(lastVal[0]);
  Serial.print(" [2]level="); Serial.print(lastVal[1]);
  Serial.println();
  Serial.print("  SUB    ");
  Serial.print(" [3]number="); Serial.print(lastVal[2]);
  Serial.println();
  Serial.print("  OUT    ");
  Serial.print(" 7seg="); Serial.print(segValue);
  Serial.println();
}

void printBanner() {
  Serial.println();
  Serial.println("===== Phycom exam sketch =====");
  Serial.print("SID = "); Serial.print(SID); Serial.print("   broker = "); Serial.print(broker); Serial.print(":"); Serial.println(port);
  Serial.println("[1] PUB 68070006/distance : ระยะ Ultrasonic (cm)");
  Serial.println("[2] PUB 68070006/level : ถ้า Potentiometer > 512 → \"high\" ไม่งั้น \"low\"");
  Serial.println("[3] SUB 68070006/number : ตัวเลข → 7-Segment (0-99)");
  Serial.println("WIRE Ultrasonic HC-SR04: TRIG->D12, ECHO->D13");
  Serial.println("WIRE Potentiometer: POT->A0");
  Serial.println("WIRE 7-Segment: a->D2, b->D3, c->D4, d->D5, e->D6, f->D7, g->D8, DIG1->A3, DIG2->A4");
  Serial.println("==============================");
}

// ================== SUBSCRIBE ==================
void onMqttMessage(int messageSize) {
  String topic = mqtt.messageTopic();
  String msg = "";
  while (mqtt.available()) msg += (char)mqtt.read();
  msg.trim();
  Serial.print("SUB "); Serial.print(topic); Serial.print(" = "); Serial.println(msg);

  // [3] SUB 68070006/number : ตัวเลข → 7-Segment (0-99)
  if (topic == SID + "/number") {
    lastVal[2] = msg;
    if (msg.length() > 0 && isDigit(msg.charAt(0))) segShowNumber(msg.toInt());
    else segShowNumber(-1);
  }
}

// ================== CONNECTION ==================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("WiFi: connecting to "); Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(2000);
  }
  while (WiFi.localIP() == IPAddress(0, 0, 0, 0)) delay(200);
  Serial.print("\nWiFi OK, IP = "); Serial.println(WiFi.localIP());
}

void connectMQTT() {
  mqtt.setId("R4_" + SID + "_" + String(micros() & 0xFFFF, HEX));
  while (!mqtt.connect(broker, port)) {
    Serial.print("MQTT failed, error = "); Serial.println(mqtt.connectError());
    delay(2000);
    connectWiFi();
  }
  Serial.println("MQTT connected");
  mqtt.onMessage(onMqttMessage);
  mqtt.subscribe(SID + "/number");
}

// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  for (int i = 0; i < 7; i++) pinMode(SEG_PINS[i], OUTPUT);
  for (int i = 0; i < 2; i++) pinMode(DIG_PINS[i], OUTPUT);
  segShowNumber(-1);
  printBanner();
  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.poll();                       // ต้องเรียกบ่อยๆ ถึงจะรับ subscribe ได้
  segRefresh();

  static unsigned long tSense = 0;
  if (millis() - tSense >= SENSE_INTERVAL) {
    tSense = millis();
    sense();
    doPublishes();
  }
  debugPrint();
}
