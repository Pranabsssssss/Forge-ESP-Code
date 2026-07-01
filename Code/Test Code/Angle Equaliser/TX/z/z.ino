#define POT1 A0
#define POT2 A1
#define POT3 A2
#define POT4 A3

int readADC(byte pin) {
  analogRead(pin);          // Dummy read
  delayMicroseconds(20);

  long sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += analogRead(pin);
    delayMicroseconds(10);
  }

  return sum / 8;
}

void setup() {
  Serial.begin(115200);

  pinMode(POT1, INPUT);
  pinMode(POT2, INPUT);
  pinMode(POT3, INPUT);
  pinMode(POT4, INPUT);

  Serial.println("Arduino Mega ADC Test");
}

void loop() {
  int pot1 = readADC(POT1);
  int pot2 = readADC(POT2);
  int pot3 = readADC(POT3);
  int pot4 = readADC(POT4);

  Serial.print("P1: ");
  Serial.print(pot1);
  Serial.print(" | P2: ");
  Serial.print(pot2);
  Serial.print(" | P3: ");
  Serial.print(pot3);
  Serial.print(" | P4: ");
  Serial.println(pot4);

  delay(50);
}