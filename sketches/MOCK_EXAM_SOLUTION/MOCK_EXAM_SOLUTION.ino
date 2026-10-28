/*
 * Physical Computing — MQTT exam sketch
 * Board: Arduino UNO R4 WiFi   |   Serial Monitor: 115200
 * Libraries: WiFiS3 (มากับ board), ArduinoMqttClient
 *
 * Tasks:
 *  [1] PUB 68070006/temp : อุณหภูมิ
 *  [2] PUB 68070006/light : Potentiometer
 *  [3] PUB 68070006/food : ถ้า ระยะ Ultrasonic (cm) > 20 → "off" ไม่งั้น "on"
 *  [4] PUB 68070006/door : ถ้า ระยะ Ultrasonic (cm) >= 20 และกดปุ่ม → "On" ไม่งั้น "Off"
 *  [5] SUB 68070006/venus : 36-50:R, 26-35:B, 10-25:G
 *  [6] SUB 68070006/prob_stat : ตัวเลข → 7-Segment (0-9)
 *
 * Wiring:
 *  Temperature sensor: TEMP->A1 | NTC: 5V -> R10000 -> PIN -> NTC -> GND
 *  Potentiometer: POT->A0 | ขาข้าง->5V/GND
 *  Ultrasonic HC-SR04: TRIG->D12, ECHO->D13 | VCC->5V GND->GND
 *  Push button: BTN->A2 | ขาทแยงอีกข้าง->GND (INPUT_PULLUP, กด=LOW)
 *  RGB LED: R->D9, G->D10, B->D11 | ขายาว->5V (common anode) + R220 ทุกสี
 *  7-Segment: a->D2, b->D3, c->D4, d->D5, e->D6, f->D7, g->D8 | common cathode, COM->GND, R220-1k ทุก segment
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
const int PIN_R = 9, PIN_G = 10, PIN_B = 11;
const bool RGB_COMMON_ANODE = true;   // true: LOW = ติด
const int PIN_BTN = A2;   // INPUT_PULLUP: กด = LOW
const int PIN_POT = A0;
const int PIN_TRIG = 12, PIN_ECHO = 13;
const int PIN_TEMP = A1;   // NTC
const int SEG_PINS[7] = {2, 3, 4, 5, 6, 7, 8};   // a b c d e f g
const bool SEG_COMMON_ANODE = false;

WiFiClient net;
MqttClient mqtt(net);

// ================== STATE ==================
float g_dist = 999;          // cm (999 = ไม่มี echo)
float g_temp = NAN, g_hum = NAN;
String lastVal[6];
unsigned long lastPub[6];

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

bool buttonPressed() { return digitalRead(PIN_BTN) == LOW; }

int potValue() { return analogRead(PIN_POT); }   // 0..1023

void readTemp() {
  long sum = 0;
  for (int i = 0; i < 10; i++) sum += analogRead(PIN_TEMP);
  float v = (sum / 10.0) * 5.0 / 1023.0;
  // Thermistor NTC (R25 = 10000, B = 3950), R คงที่ = 10000
  // ต่อแบบ: 5V -> R -> PIN -> NTC -> GND
  float adc = v / 5.0 * 1023.0;
  if (adc < 1) adc = 1;
  if (adc > 1022) adc = 1022;
  float r = 10000.0 * adc / (1023.0 - adc);   // ความต้านทาน NTC
  g_temp = 1.0 / (1.0 / 298.15 + log(r / 10000.0) / 3950.0) - 273.15;
  // ร้อนขึ้นแต่ค่าลด = ต่อกลับด้าน -> สลับ ntcTop / สลับสูตร r ข้างบน
}

bool rgbOn[3] = {false, false, false};
void rgbSet(int ch, bool on) {
  const int pins[3] = {PIN_R, PIN_G, PIN_B};
  rgbOn[ch] = on;
  digitalWrite(pins[ch], (on ^ RGB_COMMON_ANODE) ? HIGH : LOW);
}
void rgbColor(bool r, bool g, bool b) { rgbSet(0, r); rgbSet(1, g); rgbSet(2, b); }

// bit0 = a ... bit6 = g
const byte SEG_DIGITS[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
int segValue = -1;   // -1 = ดับ

void segWrite(const int pins[7], byte pat) {
  for (int i = 0; i < 7; i++) {
    bool on = pat & (1 << i);
    digitalWrite(pins[i], (on ^ SEG_COMMON_ANODE) ? HIGH : LOW);
  }
}

void segShowNumber(int n) {   // 0..9, อื่นๆ = ดับ
  segValue = (n >= 0 && n <= 9) ? n : -1;
  segWrite(SEG_PINS, segValue >= 0 ? SEG_DIGITS[segValue] : 0);
}

// ================== SENSORS ==================
void sense() {
  g_dist = readDistanceCM();
  static unsigned long tTemp = 0;
  if (tTemp == 0 || millis() - tTemp >= 500) {   // analog
    tTemp = millis();
    readTemp();
  }
}

// ================== PUBLISH ==================
void doPublishes() {
  String v;
  // [1] PUB 68070006/temp : อุณหภูมิ
  v = isnan(g_temp) ? String("") : String(g_temp, 1);
  pubIf(0, "temp", v, false);
  // [2] PUB 68070006/light : Potentiometer
  v = String(potValue());
  pubIf(1, "light", v, false);
  // [3] PUB 68070006/food : ถ้า ระยะ Ultrasonic (cm) > 20 → "off" ไม่งั้น "on"
  v = (g_dist > 20) ? "off" : "on";
  pubIf(2, "food", v, true);
  // [4] PUB 68070006/door : ถ้า ระยะ Ultrasonic (cm) >= 20 และกดปุ่ม → "On" ไม่งั้น "Off"
  v = ((g_dist >= 20) && buttonPressed()) ? "On" : "Off";
  pubIf(3, "door", v, true);
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
  Serial.print(" btn="); Serial.print(buttonPressed() ? "กด" : "ไม่กด");
  Serial.print(" pot="); Serial.print(potValue());
  Serial.print(" temp="); Serial.print(g_temp); Serial.print("C");
  Serial.print(" adc="); Serial.print(analogRead(PIN_TEMP));
  Serial.println();
  Serial.print("  PUB    ");
  Serial.print(" [1]temp="); Serial.print(lastVal[0]);
  Serial.print(" [2]light="); Serial.print(lastVal[1]);
  Serial.print(" [3]food="); Serial.print(lastVal[2]);
  Serial.print(" [4]door="); Serial.print(lastVal[3]);
  Serial.println();
  Serial.print("  SUB    ");
  Serial.print(" [5]venus="); Serial.print(lastVal[4]);
  Serial.print(" [6]prob_stat="); Serial.print(lastVal[5]);
  Serial.println();
  Serial.print("  OUT    ");
  Serial.print(" RGB="); Serial.print(rgbOn[0] ? "R" : "-");
  Serial.print(""); Serial.print(rgbOn[1] ? "G" : "-");
  Serial.print(""); Serial.print(rgbOn[2] ? "B" : "-");
  Serial.print(" 7seg="); Serial.print(segValue);
  Serial.println();
}

void printBanner() {
  Serial.println();
  Serial.println("===== Phycom exam sketch =====");
  Serial.print("SID = "); Serial.print(SID); Serial.print("   broker = "); Serial.print(broker); Serial.print(":"); Serial.println(port);
  Serial.println("[1] PUB 68070006/temp : อุณหภูมิ");
  Serial.println("[2] PUB 68070006/light : Potentiometer");
  Serial.println("[3] PUB 68070006/food : ถ้า ระยะ Ultrasonic (cm) > 20 → \"off\" ไม่งั้น \"on\"");
  Serial.println("[4] PUB 68070006/door : ถ้า ระยะ Ultrasonic (cm) >= 20 และกดปุ่ม → \"On\" ไม่งั้น \"Off\"");
  Serial.println("[5] SUB 68070006/venus : 36-50:R, 26-35:B, 10-25:G");
  Serial.println("[6] SUB 68070006/prob_stat : ตัวเลข → 7-Segment (0-9)");
  Serial.println("WIRE Temperature sensor: TEMP->A1");
  Serial.println("WIRE Potentiometer: POT->A0");
  Serial.println("WIRE Ultrasonic HC-SR04: TRIG->D12, ECHO->D13");
  Serial.println("WIRE Push button: BTN->A2");
  Serial.println("WIRE RGB LED: R->D9, G->D10, B->D11");
  Serial.println("WIRE 7-Segment: a->D2, b->D3, c->D4, d->D5, e->D6, f->D7, g->D8");
  Serial.println("==============================");
}

// ================== SUBSCRIBE ==================
void onMqttMessage(int messageSize) {
  String topic = mqtt.messageTopic();
  String msg = "";
  while (mqtt.available()) msg += (char)mqtt.read();
  msg.trim();
  Serial.print("SUB "); Serial.print(topic); Serial.print(" = "); Serial.println(msg);

  // [5] SUB 68070006/venus : 36-50:R, 26-35:B, 10-25:G
  if (topic == SID + "/venus") {
    lastVal[4] = msg;
    int v = (int)round(msg.toFloat());
    if (v >= 36 && v <= 50) rgbColor(1, 0, 0);   // แดง
    else if (v >= 26 && v <= 35) rgbColor(0, 0, 1);   // น้ำเงิน/ฟ้า
    else if (v >= 10 && v <= 25) rgbColor(0, 1, 0);   // เขียว
    else rgbColor(0, 0, 0);   // นอกช่วง = ดับ
  }

  // [6] SUB 68070006/prob_stat : ตัวเลข → 7-Segment (0-9)
  if (topic == SID + "/prob_stat") {
    lastVal[5] = msg;
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
  mqtt.subscribe(SID + "/venus");
  mqtt.subscribe(SID + "/prob_stat");
}

// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(PIN_R, OUTPUT); pinMode(PIN_G, OUTPUT); pinMode(PIN_B, OUTPUT);
  rgbColor(0, 0, 0);
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  for (int i = 0; i < 7; i++) pinMode(SEG_PINS[i], OUTPUT);
  segShowNumber(-1);
  printBanner();
  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.poll();                       // ต้องเรียกบ่อยๆ ถึงจะรับ subscribe ได้

  static unsigned long tSense = 0;
  if (millis() - tSense >= SENSE_INTERVAL) {
    tSense = millis();
    sense();
    doPublishes();
  }
  debugPrint();
}
