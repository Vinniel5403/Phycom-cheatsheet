const int PIN_BTN = A2;   // ขาทแยงของปุ่ม: ข้างหนึ่ง → A2, อีกข้าง → GND
const int PIN_LED1 = 13;   // LED บนบอร์ด

bool ledOn = false;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED1, OUTPUT);
}

void loop() {
  static bool last = HIGH;
  static unsigned long tChange = 0;
  bool now = digitalRead(PIN_BTN);
  if (now != last && millis() - tChange > 30) {   // debounce 30 ms
    tChange = millis();
    if (now == LOW) {                              // เพิ่งกด
      ledOn = !ledOn;
      digitalWrite(PIN_LED1, ledOn);
      Serial.println(ledOn ? "LED ON" : "LED OFF");
    }
    last = now;
  }
}
