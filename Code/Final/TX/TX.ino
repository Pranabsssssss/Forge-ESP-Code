#include <WiFi.h>
#include <esp_now.h>

const char* RECEIVER_MAC = "30:76:F5:EA:9F:F0";

#define POT1_PIN 32
#define POT2_PIN 33
#define POT3_PIN 34
#define POT4_PIN 35

#define BUTTON_PIN 25

typedef struct {
  uint8_t servo1;
  uint8_t servo2;
  uint8_t servo3;
  uint8_t servo4;
  uint8_t gripper;
} DataPacket;

DataPacket data;
DataPacket lastSentData;

esp_now_peer_info_t peerInfo;

bool parseMAC(const char* macStr, uint8_t* mac) {
  int values[6];

  if (sscanf(macStr,
             "%x:%x:%x:%x:%x:%x",
             &values[0],
             &values[1],
             &values[2],
             &values[3],
             &values[4],
             &values[5]) != 6) {
    return false;
  }

  for (int i = 0; i < 6; i++) {
    mac[i] = (uint8_t)values[i];
  }

  return true;
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    while (true);
  }

  uint8_t receiverMAC[6];

  if (!parseMAC(RECEIVER_MAC, receiverMAC)) {
    while (true);
  }

  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    while (true);
  }

  memset(&lastSentData, 255, sizeof(lastSentData));
}

void loop() {
  analogRead(POT1_PIN);
  data.servo1 = (analogRead(POT1_PIN) * 180) / 4095;

  analogRead(POT2_PIN);
  data.servo2 = (analogRead(POT2_PIN) * 180) / 4095;

  analogRead(POT3_PIN);
  data.servo3 = (analogRead(POT3_PIN) * 180) / 4095;

  analogRead(POT4_PIN);
  data.servo4 = (analogRead(POT4_PIN) * 180) / 4095;

  data.gripper = digitalRead(BUTTON_PIN) == LOW ? 180 : 0;

  if (
    abs((int)data.servo1 - (int)lastSentData.servo1) >= 1 ||
    abs((int)data.servo2 - (int)lastSentData.servo2) >= 1 ||
    abs((int)data.servo3 - (int)lastSentData.servo3) >= 1 ||
    abs((int)data.servo4 - (int)lastSentData.servo4) >= 1 ||
    data.gripper != lastSentData.gripper
  ) {
    esp_now_send(
      peerInfo.peer_addr,
      (uint8_t*)&data,
      sizeof(data)
    );

    lastSentData = data;
  }
}