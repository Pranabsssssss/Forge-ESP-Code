#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define N_SERVOS 5
#define CHANNEL_OFFSET 0

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
ESP8266WebServer server(80);

const uint16_t servoMin = 150;
const uint16_t servoMax = 600;

int currentAngle[N_SERVOS];

uint16_t angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, servoMin, servoMax);
}

void setServo(int idx, int angle) {
  if (idx < 0 || idx >= N_SERVOS) return;

  pwm.setPWM(idx + CHANNEL_OFFSET, 0, angleToPulse(angle));
  currentAngle[idx] = angle;
}

String pageHTML() {
  String html = "<!doctype html><html><head>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Servo Controller</title>";
  html += "<style>";
  html += "body{font-family:Arial;padding:15px}";
  html += ".servo{margin:15px 0}";
  html += "input[type=range]{width:100%}";
  html += "</style></head><body>";

  html += "<h2>Servo Controller</h2>";

  for (int i = 0; i < N_SERVOS; i++) {
    html += "<div class='servo'>";
    html += "<b>Servo ";
    html += String(i + 1);
    html += "</b><br>";
    html += "<input type='range' min='0' max='180' value='";
    html += String(currentAngle[i]);
    html += "' id='s";
    html += String(i);
    html += "' oninput='updateSlider(this)'><br>";
    html += "<input type='number' min='0' max='180' value='";
    html += String(currentAngle[i]);
    html += "' id='n";
    html += String(i);
    html += "' onchange='updateNumber(this)'>";
    html += "</div>";
  }

  html += R"rawliteral(
<script>
function updateSlider(el){
var i=el.id.substring(1);
document.getElementById('n'+i).value=el.value;
fetch('/set?i='+i+'&angle='+el.value);
}

function updateNumber(el){
var i=el.id.substring(1);
var v=Math.min(180,Math.max(0,el.value));
document.getElementById('s'+i).value=v;
fetch('/set?i='+i+'&angle='+v);
}
</script>
)rawliteral";

  html += "</body></html>";

  return html;
}

void handleRoot() {
  server.send(200, "text/html", pageHTML());
}

void handleSet() {
  if (!server.hasArg("i") || !server.hasArg("angle")) {
    server.send(400, "text/plain", "Missing Parameters");
    return;
  }

  int idx = server.arg("i").toInt();
  int angle = server.arg("angle").toInt();

  setServo(idx, angle);

  server.send(200, "text/plain", "OK");
}

void setup() {

  Serial.begin(115200);

  Wire.begin(D2, D1);      // SDA, SCL

  pwm.begin();
  pwm.setPWMFreq(50);

  for (int i = 0; i < N_SERVOS; i++) {
    currentAngle[i] = 90;
    setServo(i, 90);
  }

  WiFi.softAP("AngleEqualiser");

  Serial.println(WiFi.softAPIP());

  if (MDNS.begin("arm")) {
    Serial.println("mDNS Started");
  }

  server.on("/", handleRoot);
  server.on("/set", handleSet);

  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();
}