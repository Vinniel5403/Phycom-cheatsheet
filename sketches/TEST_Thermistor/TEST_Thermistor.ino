const int PIN_TEMP = A1;
const float R_FIXED = 10000.0;   // R ที่ต่ออนุกรม
const float R_NOM = 10000.0;     // NTC ที่ 25 °C
const float BETA = 3950.0;       // ไม่ตรงลอง 3435

float toC(float r) {
  return 1.0 / (1.0 / 298.15 + log(r / R_NOM) / BETA) - 273.15;
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  long sum = 0;
  for (int i = 0; i < 10; i++) sum += analogRead(PIN_TEMP);
  float adc = sum / 10.0;
  if (adc < 1) adc = 1;
  if (adc > 1022) adc = 1022;
  // แบบ A: 5V -> R -> A1 -> NTC -> GND
  float rA = R_FIXED * adc / (1023.0 - adc);
  // แบบ B: 5V -> NTC -> A1 -> R -> GND
  float rB = R_FIXED * (1023.0 - adc) / adc;
  Serial.print("adc=");  Serial.print(adc, 0);
  Serial.print("  A: "); Serial.print(toC(rA), 1);
  Serial.print(" C   B: "); Serial.print(toC(rB), 1);
  Serial.println(" C   <- จับ NTC แล้วแบบไหนค่าขึ้น ~25-35 = แบบนั้น");
  delay(500);
}
