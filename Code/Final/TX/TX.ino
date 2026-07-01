#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
const char* RECEIVER_MAC = "48:E7:29:6D:71:8F";
#define POT1_MIN_RAW 680
#define POT1_MAX_RAW 3730
#define POT2_MIN_RAW 250
#define POT2_MAX_RAW 3600
#define POT3_MIN_RAW 530
#define POT3_MAX_RAW 3100
#define POT4_MIN_RAW 100
#define POT4_MAX_RAW 2640
#define SERVO1_MIN_ANGLE 0
#define SERVO1_MAX_ANGLE 180
#define SERVO2_MIN_ANGLE 0
#define SERVO2_MAX_ANGLE 180
#define SERVO3_MIN_ANGLE 0
#define SERVO3_MAX_ANGLE 180
#define SERVO4_MIN_ANGLE 0
#define SERVO4_MAX_ANGLE 180
#define POT1_PIN 32
#define POT2_PIN 33
#define POT3_PIN 34
#define POT4_PIN 35
#define BUTTON_PIN 25
#define ESPNOW_CHANNEL 1
typedef struct {
  uint8_t servo1;
  uint8_t servo2;
  uint8_t servo3;
  uint8_t servo4;
  uint8_t gripper;
} ServoPacket;
ServoPacket packet;
uint8_t receiverAddr[6];
esp_now_peer_info_t peerInfo;
void parseMac(const char* mac, uint8_t* out) {
  unsigned int values[6];
  if (sscanf(mac, "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; i++) {
      out[i] = (uint8_t)values[i];
    }
  }
}
uint8_t mapPotToAngle(int raw, int rawMin, int rawMax, int angleMin, int angleMax) {
  raw = constrain(raw, rawMin, rawMax);
  return (uint8_t)map(raw, rawMin, rawMax, angleMin, angleMax);
}
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  analogReadResolution(12);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();
  parseMac(RECEIVER_MAC, receiverAddr);
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverAddr, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&peerInfo);
}
void loop() {
  packet.servo1 = mapPotToAngle(analogRead(POT1_PIN), POT1_MIN_RAW, POT1_MAX_RAW, SERVO1_MIN_ANGLE, SERVO1_MAX_ANGLE);
  packet.servo2 = mapPotToAngle(analogRead(POT2_PIN), POT2_MIN_RAW, POT2_MAX_RAW, SERVO2_MIN_ANGLE, SERVO2_MAX_ANGLE);
  packet.servo3 = mapPotToAngle(analogRead(POT3_PIN), POT3_MIN_RAW, POT3_MAX_RAW, SERVO3_MIN_ANGLE, SERVO3_MAX_ANGLE);
  packet.servo4 = mapPotToAngle(analogRead(POT4_PIN), POT4_MIN_RAW, POT4_MAX_RAW, SERVO4_MIN_ANGLE, SERVO4_MAX_ANGLE);
  packet.gripper = (digitalRead(BUTTON_PIN) == LOW) ? 180 : 0;
  esp_now_send(receiverAddr, (uint8_t*)&packet, sizeof(packet));
  Serial.printf("[TX] Sending -> S1:%d S2:%d S3:%d S4:%d Grip:%d\n", packet.servo1, packet.servo2, packet.servo3, packet.servo4, packet.gripper);
  delay(20);
}