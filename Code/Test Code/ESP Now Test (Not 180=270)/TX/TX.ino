#include <esp_now.h>
#include <WiFi.h>

const char* RECEIVER_MAC = "30:76:F5:EA:9F:F0";

#define POT1_MIN_RAW 0
#define POT1_MAX_RAW 4095

#define POT2_MIN_RAW 0
#define POT2_MAX_RAW 4095

#define POT3_MIN_RAW 0
#define POT3_MAX_RAW 4095

#define POT4_MIN_RAW 0
#define POT4_MAX_RAW 4095

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

typedef struct {
  uint8_t servo1;
  uint8_t servo2;
  uint8_t servo3;
  uint8_t servo4;
  uint8_t gripper;
} ServoPacket;

ServoPacket packet;
esp_now_peer_info_t peerInfo;
uint8_t receiverAddr[6];

void parseMac(const char* mac, uint8_t* out) {
  for (int i = 0; i < 6; i++) {
    out[i] = (uint8_t)strtol(mac + i * 3, NULL, 16);
  }
}

uint8_t potToAngle(int raw, int rawMin, int rawMax, int angleMin, int angleMax) {
  raw = constrain(raw, rawMin, rawMax);
  return (uint8_t)map(raw, rawMin, rawMax, angleMin, angleMax);
}

#define EMA_ALPHA 0.08
#define DEADBAND 2

float ema1 = 0, ema2 = 0, ema3 = 0, ema4 = 0;
bool emaInit = false;
uint8_t lastServo1 = 0, lastServo2 = 0, lastServo3 = 0, lastServo4 = 0;

int readFiltered(int pin, float &ema) {
  long sum = 0;
  for (int i = 0; i < 32; i++) {
    sum += analogRead(pin);
  }
  float raw = sum / 32.0;
  ema = EMA_ALPHA * raw + (1.0 - EMA_ALPHA) * ema;
  return (int)(ema + 0.5);
}

uint8_t applyDeadband(uint8_t newVal, uint8_t lastVal) {
  if (abs((int)newVal - (int)lastVal) > DEADBAND) return newVal;
  return lastVal;
}

void onSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SEND:OK" : "SEND:FAIL");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== TX BOOT ===");

  Serial.print("[1] Setting button pin... ");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("DONE");

  Serial.print("[2] WiFi.mode(WIFI_STA)... ");
  WiFi.mode(WIFI_STA);
  Serial.println("DONE");

  Serial.print("[3] WiFi.disconnect()... ");
  WiFi.disconnect();
  Serial.println("DONE");

  Serial.print("[4] TX MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("[5] esp_now_init()... ");
  esp_err_t initResult = esp_now_init();
  Serial.println(initResult == ESP_OK ? "OK" : "FAILED");

  Serial.print("[6] Register send callback... ");
  esp_err_t cbResult = esp_now_register_send_cb(onSent);
  Serial.println(cbResult == ESP_OK ? "OK" : "FAILED");

  Serial.print("[7] Parsing MAC: ");
  Serial.print(RECEIVER_MAC);
  parseMac(RECEIVER_MAC, receiverAddr);
  Serial.printf(" -> %02X:%02X:%02X:%02X:%02X:%02X\n",
    receiverAddr[0], receiverAddr[1], receiverAddr[2],
    receiverAddr[3], receiverAddr[4], receiverAddr[5]);

  Serial.print("[8] Adding peer... ");
  memcpy(peerInfo.peer_addr, receiverAddr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_err_t addResult = esp_now_add_peer(&peerInfo);
  if (addResult == ESP_OK) Serial.println("OK");
  else if (addResult == ESP_ERR_ESPNOW_NOT_INIT) Serial.println("FAILED: ESP-NOW not init");
  else if (addResult == ESP_ERR_ESPNOW_ARG) Serial.println("FAILED: invalid arg");
  else if (addResult == ESP_ERR_ESPNOW_FULL) Serial.println("FAILED: peer list full");
  else if (addResult == ESP_ERR_ESPNOW_EXIST) Serial.println("FAILED: peer exists");
  else Serial.printf("FAILED: error 0x%X\n", addResult);

  Serial.println("=== TX READY ===\n");
}

void loop() {
  if (!emaInit) {
    ema1 = analogRead(POT1_PIN);
    ema2 = analogRead(POT2_PIN);
    ema3 = analogRead(POT3_PIN);
    ema4 = analogRead(POT4_PIN);
    emaInit = true;
  }

  int raw1 = readFiltered(POT1_PIN, ema1);
  int raw2 = readFiltered(POT2_PIN, ema2);
  int raw3 = readFiltered(POT3_PIN, ema3);
  int raw4 = readFiltered(POT4_PIN, ema4);
  int btn = digitalRead(BUTTON_PIN);

  lastServo1 = applyDeadband(potToAngle(raw1, POT1_MIN_RAW, POT1_MAX_RAW, SERVO1_MIN_ANGLE, SERVO1_MAX_ANGLE), lastServo1);
  lastServo2 = applyDeadband(potToAngle(raw2, POT2_MIN_RAW, POT2_MAX_RAW, SERVO2_MIN_ANGLE, SERVO2_MAX_ANGLE), lastServo2);
  lastServo3 = applyDeadband(potToAngle(raw3, POT3_MIN_RAW, POT3_MAX_RAW, SERVO3_MIN_ANGLE, SERVO3_MAX_ANGLE), lastServo3);
  lastServo4 = applyDeadband(potToAngle(raw4, POT4_MIN_RAW, POT4_MAX_RAW, SERVO4_MIN_ANGLE, SERVO4_MAX_ANGLE), lastServo4);
  uint8_t newGripper = (btn == LOW) ? 180 : 0;

  if (lastServo1 == packet.servo1 && lastServo2 == packet.servo2 &&
      lastServo3 == packet.servo3 && lastServo4 == packet.servo4 &&
      newGripper == packet.gripper) {
    return;
  }

  packet.servo1 = lastServo1;
  packet.servo2 = lastServo2;
  packet.servo3 = lastServo3;
  packet.servo4 = lastServo4;
  packet.gripper = newGripper;

  Serial.printf("RAW[%d,%d,%d,%d] BTN:%d -> ANG[%d,%d,%d,%d] G:%d ",
    raw1, raw2, raw3, raw4, btn,
    packet.servo1, packet.servo2, packet.servo3, packet.servo4, packet.gripper);

  esp_err_t sendResult = esp_now_send(receiverAddr, (uint8_t*)&packet, sizeof(packet));
  if (sendResult != ESP_OK) {
    Serial.printf("QUEUE_ERR:0x%X\n", sendResult);
  }
}