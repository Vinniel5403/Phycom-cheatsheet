#include <LiquidCrystal.h>

//               RS  E  D4 D5 D6 D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);   // RW → GND, V0 → pot/R ลง GND

void setup() {
  lcd.begin(16, 2);
  lcd.print("Hello Phycom!");
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.print("t=");
  lcd.print(millis() / 1000);
  lcd.print("s   ");      // เติมช่องว่างลบตัวเก่า
  delay(200);
}
