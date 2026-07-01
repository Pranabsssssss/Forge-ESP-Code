#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <user_interface.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#define SERVO1_REVERSE 1
#define SERVO2_REVERSE 0
#define SERVO3_REVERSE 0
#define SERVO4_REVERSE 0
#define SERVO5_REVERSE 0
#define SERVOMIN 102
#define SERVOMAX 512
#define PCA9685_ADDRESS 0x40
#define ESPNOW_CHANNEL 1
#define SERVO_SPEED 180 
#define UPDATE_INTERVAL 20
const char* AP_SSID = "RoboArm";
const char* AP_PASS = "12345678";
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(PCA9685_ADDRESS);
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
const bool REVERSE[5] = {SERVO1_REVERSE, SERVO2_REVERSE, SERVO3_REVERSE, SERVO4_REVERSE, SERVO5_REVERSE};
typedef struct {
  uint8_t servo1;
  uint8_t servo2;
  uint8_t servo3;
  uint8_t servo4;
  uint8_t gripper;
} ServoPacket;
float targetAngle[5] = {90.0, 90.0, 90.0, 90.0, 90.0};
float currentAngle[5] = {90.0, 90.0, 90.0, 90.0, 90.0};
uint8_t lastDrivenAngle[5] = {255, 255, 255, 255, 255};
bool webMode = false; 
uint32_t lastServoUpdate = 0;
volatile bool espNowUpdatePending = false;
ServoPacket tempPacket;
uint16_t angleToPulse(uint8_t angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}
void driveServo(uint8_t channel, uint8_t angle) {
  if (REVERSE[channel]) {
    angle = 180 - angle;
  }
  pca.setPWM(channel, 0, angleToPulse(angle));
}
void onReceive(uint8_t *mac_addr, uint8_t *data, uint8_t len) {
  if (webMode) return;
  if (len != sizeof(ServoPacket)) return;
  memcpy(&tempPacket, data, sizeof(ServoPacket));
  espNowUpdatePending = true;
}
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %s\n", num, ip.toString().c_str());
      String initMsg = "INIT:" + String(webMode ? "web" : "master") + ":";
      for (int i = 0; i < 5; i++) {
        initMsg += String((int)round(targetAngle[i]));
        if (i < 4) initMsg += ",";
      }
      webSocket.sendTXT(num, initMsg);
      break;
    }
    case WStype_TEXT: {
      String msg = String((char*)payload);
      if (msg.startsWith("M:")) {
        String modeStr = msg.substring(2);
        if (modeStr == "web") {
          webMode = true;
        } else if (modeStr == "master") {
          webMode = false;
        }
        webSocket.broadcastTXT(msg);
        Serial.printf("Mode changed to: %s\n", webMode ? "WEB" : "MASTER");
      } 
      else {
        int colonIdx = msg.indexOf(':');
        if (colonIdx > 0 && webMode) {
          int ch = msg.substring(0, colonIdx).toInt();
          int val = msg.substring(colonIdx + 1).toInt();
          if (ch >= 0 && ch < 5 && val >= 0 && val <= 180) {
            targetAngle[ch] = val;
            Serial.printf("[WEB] Target Updated Ch:%d Val:%d\n", ch, val);
            webSocket.broadcastTXT(msg);
          }
        }
      }
      break;
    }
    default:
      break;
  }
}
#include "index_html.h"
void updateServos() {
  if (targetAngle[4] > 140) targetAngle[4] = 140;
  if (targetAngle[4] < 60) targetAngle[4] = 60;
  uint32_t now = millis();
  if (now - lastServoUpdate < UPDATE_INTERVAL) {
    return;
  }
  float dt = (now - lastServoUpdate) / 1000.0f;
  lastServoUpdate = now;
  if (dt > 0.1f) dt = 0.1f;
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
void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5);
  pca.begin();
  pca.setPWMFreq(50);
  for (int i = 0; i < 5; i++) {
    driveServo(i, 90);
    lastDrivenAngle[i] = 90;
  }
  Serial.println("Servos initialized to 90 degrees at boot.");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS, ESPNOW_CHANNEL);
  wifi_set_channel(ESPNOW_CHANNEL);
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  if (esp_now_init() == 0) {
    Serial.println("ESP-NOW Initialized Successfully");
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(onReceive);
  } else {
    Serial.println("Error initializing ESP-NOW");
  }
  if (MDNS.begin("arm")) {
    Serial.println("mDNS responder started: http://arm.local");
    MDNS.addService("http", "tcp", 80);
  }
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", INDEX_HTML);
  });
  server.begin();
  Serial.println("HTTP Server started on port 80");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket Server started on port 81");
}
uint32_t lastPrintTime = 0;

void loop() {
  server.handleClient();
  webSocket.loop();
  if (espNowUpdatePending) {
    espNowUpdatePending = false;
    targetAngle[0] = tempPacket.servo1;
    targetAngle[1] = tempPacket.servo2;
    targetAngle[2] = tempPacket.servo3;
    targetAngle[3] = tempPacket.servo4;
    targetAngle[4] = tempPacket.gripper;
    Serial.printf("[ESP-NOW] Target Updated: S1:%d S2:%d S3:%d S4:%d Grip:%d\n", (int)targetAngle[0], (int)targetAngle[1], (int)targetAngle[2], (int)targetAngle[3], (int)targetAngle[4]);
    String anglesMsg = "ANGLES:";
    for (int i = 0; i < 5; i++) {
      anglesMsg += String((int)targetAngle[i]);
      if (i < 4) anglesMsg += ",";
    }
    webSocket.broadcastTXT(anglesMsg);
  }
  updateServos();
  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();
    Serial.printf("[STATUS] Current Angles -> S1:%d S2:%d S3:%d S4:%d Grip:%d | Mode: %s\n", (int)currentAngle[0], (int)currentAngle[1], (int)currentAngle[2], (int)currentAngle[3], (int)currentAngle[4], webMode ? "WEB" : "MASTER");
  }
  MDNS.update();
}