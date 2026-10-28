const int SEG_PINS[7] = {2, 3, 4, 5, 6, 7, 8};   // a..g (ต่อขนานทั้งสองตัว)
const int DIG_PINS[2] = {A3, A4};                // COM หลักซ้าย, หลักขวา
const bool COMMON_ANODE = false;                 // CC: digit เปิด = LOW
const byte SEG_DIGITS[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

int value = 0;

void digitEnable(int i, bool on) {
  digitalWrite(DIG_PINS[i], (on ^ COMMON_ANODE) ? LOW : HIGH);
}

void segWrite(byte pat) {
  for (int i = 0; i < 7; i++) {
    bool on = pat & (1 << i);
    digitalWrite(SEG_PINS[i], (on ^ COMMON_ANODE) ? HIGH : LOW);
  }
}

void segRefresh() {
  static unsigned long t = 0;
  static int cur = 0;
  if (micros() - t < 3000) return;
  t = micros();
  digitEnable(0, false);
  digitEnable(1, false);
  cur ^= 1;
  int d = (cur == 0) ? value / 10 : value % 10;
  segWrite(SEG_DIGITS[d]);
  digitEnable(cur, true);
}

void setup() {
  for (int i = 0; i < 7; i++) pinMode(SEG_PINS[i], OUTPUT);
  for (int i = 0; i < 2; i++) pinMode(DIG_PINS[i], OUTPUT);
}

void loop() {
  segRefresh();                       // เรียกทุกรอบ loop
  static unsigned long t = 0;
  if (millis() - t >= 300) {
    t = millis();
    value = (value + 1) % 100;
  }
}
