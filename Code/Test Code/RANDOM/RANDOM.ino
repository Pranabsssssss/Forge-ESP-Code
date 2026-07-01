// ESP32 Potentiometer + Button Test

const int potPins[] = {32, 33, 34, 35};
const int buttonPin = 25;

void setup() {
  Serial.begin(115200);

  // 12-bit ADC (0-4095)
  analogReadResolution(8.5);

  // Use internal pull-up for the button
  pinMode(buttonPin, INPUT_PULLUP);

  Serial.println("ESP32 Potentiometer + Button Test");
}

void loop() {
  Serial.print("P32: ");
  Serial.print(analogRead(potPins[0]));

  Serial.print(" | P33: ");
  Serial.print(analogRead(potPins[1]));

  Serial.print(" | P34: ");
  Serial.print(analogRead(potPins[2]));

  Serial.print(" | P35: ");
  Serial.print(analogRead(potPins[3]));

  Serial.print(" | Button: ");

  if (digitalRead(buttonPin) == LOW) {
    Serial.println("PRESSED");
  } else {
    Serial.println("RELEASED");
  }

  delay(100);
}