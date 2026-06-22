#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVO1_REVERSE 0
#define SERVO2_REVERSE 0
#define SERVO3_REVERSE 0
#define SERVO4_REVERSE 0
#define SERVO5_REVERSE 0

#define SERVOMIN 102
#define SERVOMAX 512

#define SERVO_SPEED 30.0       // max degree / sec
#define UPDATE_INTERVAL 20    // ms btween srvo updates (50Hz)

const char* AP_SSID = "RoboArm";
const char* AP_PASS = "12345678";

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);
WebServer server(80);
WebSocketsServer ws(81);

bool webMode = false;
uint8_t webAngles[5] = {90, 90, 90, 90, 90};
const bool REVERSE[5] = {SERVO1_REVERSE, SERVO2_REVERSE, SERVO3_REVERSE, SERVO4_REVERSE, SERVO5_REVERSE};

float currentAngle[5] = {90, 90, 90, 90, 90};
float targetAngle[5] = {90, 90, 90, 90, 90};
uint8_t lastDriven[5] = {90, 90, 90, 90, 90};
unsigned long lastUpdateMs = 0;

typedef struct {
  uint8_t servo1;
  uint8_t servo2;
  uint8_t servo3;
  uint8_t servo4;
  uint8_t gripper;
} ServoPacket;

uint16_t angleToPulse(uint8_t angle) {
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

void driveServo(uint8_t ch, uint8_t angle) {
  if (REVERSE[ch]) angle = 180 - angle;
  pca.setPWM(ch, 0, angleToPulse(angle));
}

void updateServos() {
  unsigned long now = millis();
  if (now - lastUpdateMs < UPDATE_INTERVAL) return;
  float dt = (now - lastUpdateMs) / 1000.0;
  lastUpdateMs = now;
  float maxStep = SERVO_SPEED * dt;

  for (int i = 0; i < 5; i++) {
    float diff = targetAngle[i] - currentAngle[i];
    if (fabs(diff) <= maxStep) {
      currentAngle[i] = targetAngle[i];
    } else {
      currentAngle[i] += (diff > 0 ? maxStep : -maxStep);
    }
    uint8_t intAngle = (uint8_t)(currentAngle[i] + 0.5);
    if (intAngle != lastDriven[i]) {
      lastDriven[i] = intAngle;
      driveServo(i, intAngle);
    }
  }
}

void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (webMode) return;
  if (len != sizeof(ServoPacket)) return;
  ServoPacket* p = (ServoPacket*)data;
  targetAngle[0] = p->servo1;
  targetAngle[1] = p->servo2;
  targetAngle[2] = p->servo3;
  targetAngle[3] = p->servo4;
  targetAngle[4] = p->gripper;
}

void sendStatus(uint8_t num) {
  String json = "{\"mode\":\"" + String(webMode ? "web" : "master") + "\",\"angles\":[";
  for (int i = 0; i < 5; i++) {
    if (i > 0) json += ",";
    json += String(webAngles[i]);
  }
  json += "]}";
  ws.sendTXT(num, json);
}

void broadcastStatus() {
  String json = "{\"mode\":\"" + String(webMode ? "web" : "master") + "\",\"angles\":[";
  for (int i = 0; i < 5; i++) {
    if (i > 0) json += ",";
    json += String(webAngles[i]);
  }
  json += "]}";
  ws.broadcastTXT(json);
}

void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[WS] Client %d connected\n", num);
      sendStatus(num);
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %d disconnected\n", num);
      break;
    case WStype_TEXT: {
      char* msg = (char*)payload;
      if (msg[0] == '?') {
        sendStatus(num);
      } else if (msg[0] == 'M') {
        bool newMode = (msg[2] == 'w');
        if (newMode != webMode) {
          webMode = newMode;
          Serial.printf("[MODE] %s\n", webMode ? "WEB" : "MASTER");
          if (webMode) {
            for (int i = 0; i < 5; i++) targetAngle[i] = webAngles[i];
          }
        }
        broadcastStatus();
      } else {
        int ch = msg[0] - '0';
        int angle = atoi(msg + 2);
        if (webMode && ch >= 0 && ch < 5 && angle >= 0 && angle <= 180) {
          webAngles[ch] = angle;
          targetAngle[ch] = angle;
        }
      }
      break;
    }
    default: break;
  }
}

const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>RoboArm Control</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{
  font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
  background:linear-gradient(135deg,#0a0a1a 0%,#1a1a3e 100%);
  color:#e8e8f0;min-height:100vh;
  display:flex;justify-content:center;padding:20px;
}
.card{
  background:rgba(20,20,40,.95);
  border:1px solid rgba(108,99,255,.15);
  border-radius:24px;padding:32px 24px;
  width:100%;max-width:440px;
  box-shadow:0 20px 60px rgba(0,0,0,.5);
  align-self:flex-start;
}
h1{
  text-align:center;font-size:26px;font-weight:700;margin-bottom:6px;
  background:linear-gradient(135deg,#6c63ff,#e94560);
  -webkit-background-clip:text;-webkit-text-fill-color:transparent;
}
.sub{text-align:center;font-size:13px;color:#666;margin-bottom:24px}
.ms{
  display:flex;align-items:center;justify-content:space-between;
  padding:16px 20px;
  background:rgba(108,99,255,.08);
  border:1px solid rgba(108,99,255,.15);
  border-radius:16px;margin-bottom:24px;
}
.ml{font-size:16px;font-weight:600}
.mb{
  font-size:11px;padding:4px 12px;border-radius:20px;
  font-weight:600;margin-top:4px;display:inline-block;
}
.mb.m{background:rgba(108,99,255,.2);color:#6c63ff}
.mb.w{background:rgba(233,69,96,.2);color:#e94560}
.tg{position:relative;width:56px;height:28px;cursor:pointer;flex-shrink:0}
.tg input{display:none}
.tt{position:absolute;inset:0;background:#2a2a4a;border-radius:28px;transition:.3s}
.tg input:checked+.tt{background:#e94560}
.th{
  position:absolute;top:2px;left:2px;width:24px;height:24px;
  background:#fff;border-radius:50%;transition:.3s;
  box-shadow:0 2px 4px rgba(0,0,0,.3);
}
.tg input:checked~.th{transform:translateX(28px)}
.sg{
  padding:14px 16px;
  background:rgba(255,255,255,.03);
  border:1px solid rgba(255,255,255,.06);
  border-radius:14px;margin-bottom:12px;
  transition:opacity .3s;
}
.sg.off{opacity:.3;pointer-events:none}
.sh{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px}
.sn{font-size:14px;font-weight:600;color:#aaa}
.sv{font-size:18px;font-weight:700;color:#6c63ff;min-width:45px;text-align:right}
input[type=range]{
  -webkit-appearance:none;width:100%;height:8px;
  background:#1a1a3e;border-radius:4px;outline:none;cursor:pointer;
}
input[type=range]::-webkit-slider-thumb{
  -webkit-appearance:none;width:24px;height:24px;
  background:linear-gradient(135deg,#6c63ff,#8b5cf6);
  border-radius:50%;cursor:pointer;
  box-shadow:0 0 10px rgba(108,99,255,.4);
}
.st{text-align:center;font-size:12px;color:#555;margin-top:16px}
.dot{
  display:inline-block;width:8px;height:8px;
  border-radius:50%;margin-right:6px;animation:p 2s infinite;
}
.dg{background:#4ade80}.dr{background:#e94560}.dy{background:#facc15}
@keyframes p{0%,100%{opacity:1}50%{opacity:.4}}
</style>
</head>
<body>
<div class="card">
<h1>🤖 RoboArm</h1>
<div class="sub">ESP-NOW Robotic Arm Controller</div>
<div class="ms">
<div>
<div class="ml" id="mt">Master Mode</div>
<span class="mb m" id="badge">ESP-NOW</span>
</div>
<label class="tg">
<input type="checkbox" id="tog" onchange="tm(this)">
<div class="tt"></div><div class="th"></div>
</label>
</div>
<div id="sl"></div>
<div class="st">
<span class="dot dy" id="sd"></span>
<span id="stxt">Connecting...</span>
</div>
</div>
<script>
const N=['Servo 1','Servo 2','Servo 3','Servo 4','Gripper'];
let mode='master',sock;
const sl=document.getElementById('sl');
N.forEach((n,i)=>{
  sl.innerHTML+=
    '<div class="sg off" id="sg'+i+'">'+
    '<div class="sh"><span class="sn">'+n+'</span>'+
    '<span class="sv" id="v'+i+'">90\u00B0</span></div>'+
    '<input type="range" min="0" max="180" value="90" id="r'+i+'" oninput="ss('+i+',this.value)">'+
    '</div>';
});
function connect(){
  sock=new WebSocket('ws://'+location.hostname+':81/');
  sock.onopen=()=>{
    document.getElementById('sd').className='dot dg';
    document.getElementById('stxt').textContent='Connected';
    sock.send('?');
  };
  sock.onmessage=(e)=>{
    let d=JSON.parse(e.data);
    mode=d.mode;
    if(d.angles)d.angles.forEach((a,i)=>{
      document.getElementById('r'+i).value=a;
      document.getElementById('v'+i).textContent=a+'\u00B0';
    });
    ui();
  };
  sock.onclose=()=>{
    document.getElementById('sd').className='dot dy';
    document.getElementById('stxt').textContent='Reconnecting...';
    setTimeout(connect,1000);
  };
}
connect();
function tm(el){sock.send('M:'+(el.checked?'web':'master'));}
function ss(ch,a){
  if(mode!='web')return;
  document.getElementById('v'+ch).textContent=a+'\u00B0';
  sock.send(ch+':'+a);
}
function ui(){
  document.getElementById('tog').checked=mode=='web';
  document.getElementById('mt').textContent=mode=='web'?'Web Mode':'Master Mode';
  const b=document.getElementById('badge');
  b.textContent=mode=='web'?'SLIDERS':'ESP-NOW';
  b.className='mb '+(mode=='web'?'w':'m');
  document.getElementById('sd').className='dot '+(mode=='web'?'dr':'dg');
  document.getElementById('stxt').textContent=mode=='web'?'Web Mode Active':'Master Mode Active';
  N.forEach((_,i)=>{
    const g=document.getElementById('sg'+i);
    if(mode=='web')g.classList.remove('off');else g.classList.add('off');
  });
}
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== RX BOOT ===");

  Serial.print("[1] WiFi.mode(WIFI_AP_STA)... ");
  WiFi.mode(WIFI_AP_STA);
  Serial.println("DONE");

  Serial.print("[2] Starting AP... ");
  WiFi.softAP(AP_SSID, AP_PASS, 1);
  Serial.print("DONE, IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("[3] RX STA MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("[4] mDNS (arm.local)... ");
  Serial.println(MDNS.begin("arm") ? "OK" : "FAILED");

  Serial.print("[5] Wire.begin(21, 22)... ");
  Wire.begin(21, 22);
  Serial.println("DONE");

  Serial.print("[6] I2C scan 0x40... ");
  Wire.beginTransmission(0x40);
  Serial.println(Wire.endTransmission() == 0 ? "FOUND" : "NOT FOUND");

  Serial.print("[7] pca.begin()... ");
  pca.begin();
  Serial.println("DONE");

  Serial.print("[8] pca.setPWMFreq(50)... ");
  pca.setPWMFreq(50);
  Serial.println("DONE");

  Serial.print("[9] All servos to 90... ");
  for (int i = 0; i < 5; i++) {
    pca.setPWM(i, 0, map(90, 0, 180, SERVOMIN, SERVOMAX));
  }
  Serial.println("DONE");

  Serial.print("[10] esp_now_init()... ");
  Serial.println(esp_now_init() == ESP_OK ? "OK" : "FAILED");

  Serial.print("[11] Register recv callback... ");
  Serial.println(esp_now_register_recv_cb(onReceive) == ESP_OK ? "OK" : "FAILED");

  server.on("/", handleRoot);
  server.begin();
  Serial.println("[12] HTTP server started (port 80)");

  ws.begin();
  ws.onEvent(wsEvent);
  Serial.println("[13] WebSocket server started (port 81)");

  lastUpdateMs = millis();

  Serial.println("\n=== RX READY ===");
  Serial.printf("Speed: %.1f deg/sec\n", SERVO_SPEED);
  Serial.println("Mode: MASTER (default)");
  Serial.printf("WiFi SSID: %s  Pass: %s\n", AP_SSID, AP_PASS);
  Serial.println("Web: http://arm.local\n");
}

void loop() {
  server.handleClient();
  ws.loop();
  updateServos();
}