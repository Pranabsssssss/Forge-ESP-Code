#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver();

#define SERVOMIN 125
#define SERVOMID 350
#define SERVOMAX 575

void moveAll(int pos) {
  for (int ch = 0; ch < 6; ch++) {
    pca.setPWM(ch, 0, pos);
  }
}

void setup() {
  Serial.begin(115200);

  pca.begin();
  pca.setPWMFreq(50);

  Serial.println("Commands:");
  Serial.println("min");
  Serial.println("mid");
  Serial.println("max");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "min") {
      moveAll(SERVOMIN);
      Serial.println("Moved to MIN");
    }
    else if (cmd == "mid") {
      moveAll(SERVOMID);
      Serial.println("Moved to MID");
    }
    else if (cmd == "max") {
      moveAll(SERVOMAX);
      Serial.println("Moved to MAX");
    }
  }
}