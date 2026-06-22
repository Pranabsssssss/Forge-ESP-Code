#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const char* SSID = "RobotArm";
const char* PASSWORD = "12345678";

#define SDA_PIN 21
#define SCL_PIN 22

#define SERVOMIN 102
#define SERVOMAX 512

#define SERVO1_CHANNEL 0
#define SERVO2_CHANNEL 1
#define SERVO3_CHANNEL 2
#define SERVO4_CHANNEL 3
#define SERVO5_CHANNEL 4

#define SERVO1_REVERSE 0
#define SERVO2_REVERSE 0
#define SERVO3_REVERSE 0
#define SERVO4_REVERSE 0
#define SERVO5_REVERSE 0

WebServer server(80);
Adafruit_PWMServoDriver pca9685(0x40);

uint8_t servo1 = 90;
uint8_t servo2 = 90;
uint8_t servo3 = 90;
uint8_t servo4 = 90;
uint8_t servo5 = 90;

uint8_t applyReverse(uint8_t angle, bool reverseFlag) {
  return reverseFlag ? (180 - angle) : angle;
}

uint16_t angleToPulse(uint8_t angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void writeServos() {
  pca9685.setPWM(SERVO1_CHANNEL, 0, angleToPulse(applyReverse(servo1, SERVO1_REVERSE)));
  pca9685.setPWM(SERVO2_CHANNEL, 0, angleToPulse(applyReverse(servo2, SERVO2_REVERSE)));
  pca9685.setPWM(SERVO3_CHANNEL, 0, angleToPulse(applyReverse(servo3, SERVO3_REVERSE)));
  pca9685.setPWM(SERVO4_CHANNEL, 0, angleToPulse(applyReverse(servo4, SERVO4_REVERSE)));
  pca9685.setPWM(SERVO5_CHANNEL, 0, angleToPulse(applyReverse(servo5, SERVO5_REVERSE)));
}

void handleSet() {
  if (server.hasArg("s1")) servo1 = constrain(server.arg("s1").toInt(), 0, 180);
  if (server.hasArg("s2")) servo2 = constrain(server.arg("s2").toInt(), 0, 180);
  if (server.hasArg("s3")) servo3 = constrain(server.arg("s3").toInt(), 0, 180);
  if (server.hasArg("s4")) servo4 = constrain(server.arg("s4").toInt(), 0, 180);
  if (server.hasArg("s5")) servo5 = constrain(server.arg("s5").toInt(), 0, 180);

  writeServos();

  server.send(200, "text/plain", "OK");
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot Arm</title>
<style>
body{
background:#111;
color:white;
font-family:Arial;
max-width:800px;
margin:auto;
padding:20px;
}
.slider{
width:100%;
}
.card{
background:#1b1b1b;
padding:15px;
margin:15px 0;
border-radius:12px;
}
.value{
font-size:20px;
font-weight:bold;
}
</style>
</head>
<body>

<h1>Robot Arm Controller</h1>

<div class="card">
Base <span class="value" id="v1">90</span>°
<input class="slider" type="range" min="0" max="180" value="90" id="s1">
</div>

<div class="card">
Shoulder <span class="value" id="v2">90</span>°
<input class="slider" type="range" min="0" max="180" value="90" id="s2">
</div>

<div class="card">
Elbow <span class="value" id="v3">90</span>°
<input class="slider" type="range" min="0" max="180" value="90" id="s3">
</div>

<div class="card">
Wrist <span class="value" id="v4">90</span>°
<input class="slider" type="range" min="0" max="180" value="90" id="s4">
</div>

<div class="card">
Gripper <span class="value" id="v5">90</span>°
<input class="slider" type="range" min="0" max="180" value="90" id="s5">
</div>

<script>
const s1=document.getElementById('s1');
const s2=document.getElementById('s2');
const s3=document.getElementById('s3');
const s4=document.getElementById('s4');
const s5=document.getElementById('s5');

function update(){
document.getElementById('v1').innerText=s1.value;
document.getElementById('v2').innerText=s2.value;
document.getElementById('v3').innerText=s3.value;
document.getElementById('v4').innerText=s4.value;
document.getElementById('v5').innerText=s5.value;

fetch(`/set?s1=${s1.value}&s2=${s2.value}&s3=${s3.value}&s4=${s4.value}&s5=${s5.value}`);
}

s1.addEventListener('input',update);
s2.addEventListener('input',update);
s3.addEventListener('input',update);
s4.addEventListener('input',update);
s5.addEventListener('input',update);
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  pca9685.begin();
  pca9685.setPWMFreq(50);

  writeServos();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASSWORD);

  MDNS.begin("arm");

  server.on("/", handleRoot);
  server.on("/set", handleSet);

  server.begin();

  Serial.println("WiFi Ready");
  Serial.println("SSID: RobotArm");
  Serial.println("Password: 12345678");
  Serial.println("http://arm.local");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}