#include <Keypad.h>

const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};   // R1..R4
byte colPins[COLS] = {5, 4, 3, 2};   // C1..C4
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String buf = "";

void setup() {
  Serial.begin(115200);
}

void loop() {
  char k = keypad.getKey();
  if (!k) return;
  Serial.print("key: ");
  Serial.println(k);
  if (k == '#') {
    Serial.print("ENTER -> ");
    Serial.println(buf);
    buf = "";
  } else if (k == '*') {
    buf = "";
  } else {
    buf += k;
  }
}
