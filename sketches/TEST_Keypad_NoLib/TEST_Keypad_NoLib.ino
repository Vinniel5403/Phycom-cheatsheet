const byte ROW_PINS[4] = {9, 8, 7, 6};
const byte COL_PINS[4] = {5, 4, 3, 2};
const char KEYS[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

char scanKey() {
  for (int c = 0; c < 4; c++) {
    digitalWrite(COL_PINS[c], LOW);
    delayMicroseconds(5);
    for (int r = 0; r < 4; r++) {
      if (digitalRead(ROW_PINS[r]) == LOW) {
        digitalWrite(COL_PINS[c], HIGH);
        return KEYS[r][c];
      }
    }
    digitalWrite(COL_PINS[c], HIGH);
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  for (int r = 0; r < 4; r++) pinMode(ROW_PINS[r], INPUT_PULLUP);
  for (int c = 0; c < 4; c++) {
    pinMode(COL_PINS[c], OUTPUT);
    digitalWrite(COL_PINS[c], HIGH);
  }
}

void loop() {
  static char last = 0;
  static unsigned long t = 0;
  if (millis() - t < 20) return;   // สแกนทุก 20 ms (debounce)
  t = millis();
  char k = scanKey();
  if (k && k != last) {
    Serial.print("key: ");
    Serial.println(k);
  }
  last = k;
}
