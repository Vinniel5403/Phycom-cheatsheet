const int PIN_R = 9, PIN_G = 10, PIN_B = 11;
const bool COMMON_ANODE = true;   // ถ้าสีที่ขึ้นกลับด้าน ให้เปลี่ยนเป็น false

void rgb(bool r, bool g, bool b) {
  digitalWrite(PIN_R, (r ^ COMMON_ANODE) ? HIGH : LOW);
  digitalWrite(PIN_G, (g ^ COMMON_ANODE) ? HIGH : LOW);
  digitalWrite(PIN_B, (b ^ COMMON_ANODE) ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
}

void loop() {
  Serial.println("RED");    rgb(1, 0, 0); delay(1000);
  Serial.println("GREEN");  rgb(0, 1, 0); delay(1000);
  Serial.println("BLUE");   rgb(0, 0, 1); delay(1000);
  Serial.println("YELLOW"); rgb(1, 1, 0); delay(1000);
  Serial.println("OFF");    rgb(0, 0, 0); delay(1000);
}
