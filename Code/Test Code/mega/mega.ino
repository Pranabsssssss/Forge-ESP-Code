#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// --- Configuration ---
// Potentiometer Analog Pins (Arduino Mega)
#define POT1_PIN A0
#define POT2_PIN A1
#define POT3_PIN A2
#define POT4_PIN A3

// Button Pin for Gripper
#define BUTTON_PIN 2

// Potentiometer Calibration Ranges (Standard Arduino ADC is 10-bit: 0-1023)
#define POT1_MIN_RAW 0
#define POT1_MAX_RAW 1023
#define POT2_MIN_RAW 0
#define POT2_MAX_RAW 900
#define POT3_MIN_RAW 400   
#define POT3_MAX_RAW 1023
#define POT4_MIN_RAW 31
#define POT4_MAX_RAW 

// Servo Inversion settings
#define SERVO1_REVERSE 1
#define SERVO2_REVERSE 0
#define SERVO3_REVERSE 0
#define SERVO4_REVERSE 0
#define SERVO5_REVERSE 0

// PCA9685 Config
#define PCA9685_ADDRESS 0x40
#define SERVOMIN 102 // ~0 degrees pulse length
#define SERVOMAX 512 // ~180 degrees pulse length

// Motion & Smoothing Parameters
#define SERVO_SPEED 1000.0     // Maximum servo speed in degrees per second
#define UPDATE_INTERVAL 1      // 1000Hz update rate (1 millisecond)
#define ADC_DEADZONE 8         // Increased deadzone: RAW must change by > 8 (~1.5 deg) to register
#define FILTER_ALPHA 0.4       // Balance between speed and smoothing
#define ANGLE_DEADZONE 3       // Angle must change by > 1 degree to trigger movement

// --- Global Variables ---
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(PCA9685_ADDRESS);

const bool REVERSE[5] = {SERVO1_REVERSE, SERVO2_REVERSE, SERVO3_REVERSE, SERVO4_REVERSE, SERVO5_REVERSE};
const uint8_t PINS[4] = {POT1_PIN, POT2_PIN, POT3_PIN, POT4_PIN};
const int MIN_RAW[4] = {POT1_MIN_RAW, POT2_MIN_RAW, POT3_MIN_RAW, POT4_MIN_RAW};
const int MAX_RAW[4] = {POT1_MAX_RAW, POT2_MAX_RAW, POT3_MAX_RAW, POT4_MAX_RAW};

float targetAngle[5] = {90.0, 90.0, 90.0, 90.0, 90.0};
float currentAngle[5] = {90.0, 90.0, 90.0, 90.0, 90.0};
uint8_t lastDrivenAngle[5] = {255, 255, 255, 255, 255};

float filteredRaw[4] = {512.0, 512.0, 512.0, 512.0};
int lastStableRaw[4] = {512, 512, 512, 512};

uint32_t lastServoUpdate = 0;

// --- Functions ---

uint16_t angleToPulse(uint8_t angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void driveServo(uint8_t channel, uint8_t angle) {
  if (REVERSE[channel]) {
    angle = 180 - angle;
  }
  pca.setPWM(channel, 0, angleToPulse(angle));
}

// Reads ADC with Crosstalk fix
int readAdcWithCrosstalkFix(uint8_t pin) {
  analogRead(pin);           // Dummy read to switch multiplexer
  return analogRead(pin);    // Actual read
}

void readInputs() {
  for (int i = 0; i < 4; i++) {
    // Override: Lock Servo 3 (index 2) to 90 degrees and ignore POT3
    if (i == 2) {
      targetAngle[i] = 90;
      continue;
    }

    int raw = readAdcWithCrosstalkFix(PINS[i]);
    
    // Sudden False Value Spike Filter: 
    // Ignore absurdly massive jumps between consecutive 1ms reads unless they persist.
    if (abs(raw - filteredRaw[i]) > 300) {
      // It's likely a spike. Let's smooth it extremely heavily for this frame.
      filteredRaw[i] = (0.01 * raw) + (0.99 * filteredRaw[i]);
    } else {
      // Standard Exponential Moving Average Filter
      filteredRaw[i] = (FILTER_ALPHA * raw) + ((1.0 - FILTER_ALPHA) * filteredRaw[i]);
    }

    // Deadzone application to prevent tiny vibrations
    if (abs((int)filteredRaw[i] - lastStableRaw[i]) >= ADC_DEADZONE) {
      lastStableRaw[i] = (int)filteredRaw[i];
    }
    
    // Map stable RAW to 0-180 target angle
    int mappedAngle = map(lastStableRaw[i], MIN_RAW[i], MAX_RAW[i], 0, 180);
    mappedAngle = constrain(mappedAngle, 0, 180);

    // Angle Hysteresis: only update if the angle changes significantly
    if (abs(mappedAngle - (int)targetAngle[i]) > ANGLE_DEADZONE) {
      targetAngle[i] = mappedAngle;
    }
  }

  // Gripper logic (Button)
  // Limited to 140 and 60 instead of 180 and 0 to prevent the servo from 
  // hitting physical limits, stalling, and overheating/shutting down.
  targetAngle[4] = (digitalRead(BUTTON_PIN) == LOW) ? 140 : 60;
}

void updateServos() {
  uint32_t now = millis();
  if (now - lastServoUpdate < UPDATE_INTERVAL) {
    return;
  }
  
  float dt = (now - lastServoUpdate) / 1000.0f;
  lastServoUpdate = now;

  // Cap dt to prevent massive jumps if loop execution is blocked temporarily
  if (dt > 0.05f) dt = 0.05f;

  for (int i = 0; i < 5; i++) {
    float diff = targetAngle[i] - currentAngle[i];
    if (abs(diff) > 0.01f) {
      float step = SERVO_SPEED * dt;
      if (abs(diff) <= step) {
        currentAngle[i] = targetAngle[i];
      } else {
        if (diff > 0) {
          currentAngle[i] += step;
        } else {
          currentAngle[i] -= step;
        }
      }
      
      uint8_t angleToDrive = (uint8_t)round(currentAngle[i]);
      if (angleToDrive != lastDrivenAngle[i]) {
        driveServo(i, angleToDrive);
        lastDrivenAngle[i] = angleToDrive;
      }
    }
  }
}

// --- Setup & Loop ---

void setup() {
  Serial.begin(115200);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin();
  pca.begin();
  pca.setPWMFreq(50); // Analog servos run at ~50 Hz

  // Initialize all servos to 90 degrees at boot
  for (int i = 0; i < 5; i++) {
    driveServo(i, 90);
    lastDrivenAngle[i] = 90;
  }
  Serial.println("Mega 2560 Direct Control Initialized. Servos at 90 deg.");
}

uint32_t lastPrintTime = 0;

void loop() {
  readInputs();
  updateServos();

  // Print values every 100 milliseconds (non-blocking)
  if (millis() - lastPrintTime >= 100) {
    lastPrintTime = millis();
    Serial.print("POT1 (Base): "); Serial.print(lastStableRaw[0]);
    Serial.print(" | POT2 (Shoulder): "); Serial.print(lastStableRaw[1]);
    Serial.print(" | POT3 (Elbow-DISABLED): "); Serial.print(lastStableRaw[2]);
    Serial.print(" | POT4 (Wrist): "); Serial.print(lastStableRaw[3]);
    Serial.print(" | Switch: "); Serial.println(digitalRead(BUTTON_PIN) == LOW ? "PRESSED" : "OPEN");
  }
}
