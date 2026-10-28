// ต่อ a..g ไปที่ D2..D8 (ไม่แน่ใจลำดับก็ได้) แล้วดู Serial ว่าขาไหนจุด segment ไหน
const int SEG_PINS[7] = {2, 3, 4, 5, 6, 7, 8};   // a b c d e f g
const bool COMMON_ANODE = false;
const byte SEG_DIGITS[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

void segWrite(byte pat) {
  for (int i = 0; i < 7; i++) {
    bool on = pat & (1 << i);
    digitalWrite(SEG_PINS[i], (on ^ COMMON_ANODE) ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 7; i++) pinMode(SEG_PINS[i], OUTPUT);
  // ทีละขา
  for (int i = 0; i < 7; i++) {
    segWrite(1 << i);
    Serial.print("ขา D");
    Serial.print(SEG_PINS[i]);
    Serial.print(" ควรเป็น segment ");
    Serial.println((char)('a' + i));
    delay(1500);
  }
}

void loop() {
  for (int n = 0; n < 10; n++) {
    segWrite(SEG_DIGITS[n]);
    delay(700);
  }
}
