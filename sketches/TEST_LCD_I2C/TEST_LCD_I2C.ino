#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);   // ไม่ขึ้นลอง 0x3F

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.print("Hello I2C LCD");
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.print(millis() / 1000);
  lcd.print("   ");
  delay(200);
}
