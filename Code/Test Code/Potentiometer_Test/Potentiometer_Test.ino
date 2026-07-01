#define POT1_PIN A0  // Matched your swapped pins from mega.ino
#define POT2_PIN A1
#define POT3_PIN A2
#define POT4_PIN A

#define BUTTON_PIN 2

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  Serial.println("Starting Highly Accurate Potentiometer Read Test...");
  Serial.println("-------------------------------------------------");
}

// Function to kill crosstalk and electrical noise
int getAccurateAnalog(uint8_t pin) {
  analogRead(pin);           // Dummy read to let the multiplexer switch
  delayMicroseconds(100);    // Give the internal capacitor time to charge
  
  long sum = 0;
  for (int i = 0; i < 8; i++) { // Take 8 readings
    sum += analogRead(pin);
  }
  return sum / 8;            // Return the average
}

void loop() {
  // Read all 4 potentiometers using the accurate function
  int pot1 = getAccurateAnalog(POT1_PIN);
  int pot2 = getAccurateAnalog(POT2_PIN);
  int pot3 = getAccurateAnalog(POT3_PIN);
  int pot4 = getAccurateAnalog(POT4_PIN);
  
  // Read the gripper button
  int button = digitalRead(BUTTON_PIN);

  // Print values to the Serial Monitor
  Serial.print("Pot 1 (Base): ");
  Serial.print(pot1);
  Serial.print(" | Pot 2 (Shoulder): ");
  Serial.print(pot2);
  Serial.print(" | Pot 3 (Elbow): ");
  Serial.print(pot3);
  Serial.print(" | Pot 4 (Wrist): ");
  Serial.print(pot4);
  Serial.print(" | Gripper Button: ");
  Serial.println(button == LOW ? "PRESSED" : "OPEN");

  // Small delay to make the Serial Monitor easy to read
  delay(100);
}
