#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define SERVOMIN 102
#define SERVOMAX 512

#define SERVO1_CHANNEL 11
#define SERVO2_CHANNEL 12
#define SERVO3_CHANNEL 13
#define SERVO4_CHANNEL 14
#define SERVO5_CHANNEL 15

#define SERVO1_REVERSE 0
#define SERVO2_REVERSE 0
#define SERVO3_REVERSE 0
#define SERVO4_REVERSE 0
#define SERVO5_REVERSE 0

Adafruit_PWMServoDriver pca9685(0x40);

typedef struct {
  uint8_t servo1;
  uint8_t servo2;
  uint8_t servo3;
  uint8_t servo4;
  uint8_t gripper;
} DataPacket;

DataPacket receivedData;

uint8_t applyReverse(uint8_t angle, bool reverseFlag) {
  return reverseFlag ? (180 - angle) : angle;
}

uint16_t angleToPulse(uint8_t angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(DataPacket)) return;

  memcpy(&receivedData, incomingData, sizeof(receivedData));

  pca9685.setPWM(
    SERVO1_CHANNEL,
    0,
    angleToPulse(
      applyReverse(
        receivedData.servo1,
        SERVO1_REVERSE
      )
    )
  );

  pca9685.setPWM(
    SERVO2_CHANNEL,
    0,
    angleToPulse(
      applyReverse(
        receivedData.servo2,
        SERVO2_REVERSE
      )
    )
  );

  pca9685.setPWM(
    SERVO3_CHANNEL,
    0,
    angleToPulse(
      applyReverse(
        receivedData.servo3,
        SERVO3_REVERSE
      )
    )
  );

  pca9685.setPWM(
    SERVO4_CHANNEL,
    0,
    angleToPulse(
      applyReverse(
        receivedData.servo4,
        SERVO4_REVERSE
      )
    )
  );

  pca9685.setPWM(
    SERVO5_CHANNEL,
    0,
    angleToPulse(
      applyReverse(
        receivedData.gripper,
        SERVO5_REVERSE
      )
    )
  );
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  Wire.begin(SDA_PIN, SCL_PIN);

  pca9685.begin();
  pca9685.setPWMFreq(50);

  if (esp_now_init() != ESP_OK) {
    while (true);
  }

  esp_now_register_recv_cb(onReceive);

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
}