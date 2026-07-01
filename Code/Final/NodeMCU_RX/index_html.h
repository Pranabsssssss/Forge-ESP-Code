#pragma once
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RoboArm Control Panel</title>
  <style>
    :root {
      --bg-gradient-start: #0f172a;
      --bg-gradient-end: #1e1b4b;
      --card-bg: rgba(30, 41, 59, 0.7);
      --card-border: rgba(255, 255, 255, 0.08);
      --accent-primary: #6366f1;
      --accent-secondary: #a855f7;
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --success: #10b981;
      --danger: #ef4444;
    }
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      font-family: 'Outfit', sans-serif;
    }
    body {
      background: linear-gradient(135deg, var(--bg-gradient-start), var(--bg-gradient-end));
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    .container {
      width: 100%;
      max-width: 500px;
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 24px;
      padding: 30px;
      box-shadow: 0 20px 40px rgba(0, 0, 0, 0.4);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
    }
    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 25px;
    }
    h1 {
      font-size: 24px;
      font-weight: 700;
      background: linear-gradient(to right, #818cf8, #c084fc);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }
    .status-badge {
      display: flex;
      align-items: center;
      gap: 6px;
      font-size: 12px;
      font-weight: 600;
      padding: 6px 12px;
      border-radius: 12px;
      background: rgba(255, 255, 255, 0.05);
    }
    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background-color: var(--danger);
      box-shadow: 0 0 8px var(--danger);
    }
    .status-dot.connected {
      background-color: var(--success);
      box-shadow: 0 0 8px var(--success);
    }
    .mode-section {
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid rgba(255, 255, 255, 0.05);
      border-radius: 16px;
      padding: 15px 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 25px;
    }
    .mode-label {
      display: flex;
      flex-direction: column;
    }
    .mode-title {
      font-size: 16px;
      font-weight: 600;
    }
    .mode-desc {
      font-size: 12px;
      color: var(--text-muted);
      margin-top: 2px;
    }
    .switch {
      position: relative;
      display: inline-block;
      width: 52px;
      height: 28px;
    }
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider-toggle {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: #334155;
      transition: .3s;
      border-radius: 34px;
    }
    .slider-toggle:before {
      position: absolute;
      content: "";
      height: 20px;
      width: 20px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: .3s;
      border-radius: 50%;
    }
    input:checked + .slider-toggle {
      background-color: var(--accent-primary);
    }
    input:checked + .slider-toggle:before {
      transform: translateX(24px);
    }
    .control-group {
      display: flex;
      flex-direction: column;
      gap: 20px;
    }
    .servo-control {
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid rgba(255, 255, 255, 0.03);
      border-radius: 16px;
      padding: 18px;
      transition: opacity 0.3s ease, transform 0.3s ease;
    }
    .servo-control.disabled {
      opacity: 0.4;
      pointer-events: none;
    }
    .servo-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 12px;
    }
    .servo-name {
      font-size: 14px;
      font-weight: 600;
      color: var(--text-main);
    }
    .servo-value {
      font-size: 14px;
      font-weight: 700;
      color: var(--accent-primary);
      background: rgba(99, 102, 241, 0.1);
      padding: 2px 8px;
      border-radius: 6px;
    }
    .slider-container {
      position: relative;
      display: flex;
      align-items: center;
    }
    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      height: 6px;
      border-radius: 3px;
      background: #334155;
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--text-main);
      cursor: pointer;
      box-shadow: 0 0 10px rgba(99, 102, 241, 0.5);
      border: 2px solid var(--accent-primary);
      transition: transform 0.1s;
    }
    input[type=range]::-webkit-slider-thumb:active {
      transform: scale(1.2);
    }
    input[type=range]::-moz-range-thumb {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--text-main);
      cursor: pointer;
      box-shadow: 0 0 10px rgba(99, 102, 241, 0.5);
      border: 2px solid var(--accent-primary);
      transition: transform 0.1s;
    }
  </style>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700&display=swap" rel="stylesheet">
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>RoboArm Control</h1>
      <div class="status-badge">
        <div id="statusDot" class="status-dot"></div>
        <span id="statusText">Connecting</span>
      </div>
    </div>

    <div class="mode-section">
      <div class="mode-label">
        <span class="mode-title">Web Control Mode</span>
        <span class="mode-desc" id="modeDesc">Follows transmitter via ESP-NOW</span>
      </div>
      <label class="switch">
        <input type="checkbox" id="modeToggle" onchange="toggleMode(this.checked)">
        <span class="slider-toggle"></span>
      </label>
    </div>

    <div class="control-group">
      <!-- Servo 1 -->
      <div class="servo-control disabled" id="control-0">
        <div class="servo-header">
          <span class="servo-name">Servo 1 (Base)</span>
          <span class="servo-value" id="val-0">90°</span>
        </div>
        <div class="slider-container">
          <input type="range" min="0" max="180" value="90" id="slider-0" oninput="sendSliderValue(0, this.value)">
        </div>
      </div>

      <!-- Servo 2 -->
      <div class="servo-control disabled" id="control-1">
        <div class="servo-header">
          <span class="servo-name">Servo 2 (Shoulder)</span>
          <span class="servo-value" id="val-1">90°</span>
        </div>
        <div class="slider-container">
          <input type="range" min="0" max="180" value="90" id="slider-1" oninput="sendSliderValue(1, this.value)">
        </div>
      </div>

      <!-- Servo 3 -->
      <div class="servo-control disabled" id="control-2">
        <div class="servo-header">
          <span class="servo-name">Servo 3 (Elbow)</span>
          <span class="servo-value" id="val-2">90°</span>
        </div>
        <div class="slider-container">
          <input type="range" min="0" max="180" value="90" id="slider-2" oninput="sendSliderValue(2, this.value)">
        </div>
      </div>

      <!-- Servo 4 -->
      <div class="servo-control disabled" id="control-3">
        <div class="servo-header">
          <span class="servo-name">Servo 4 (Wrist)</span>
          <span class="servo-value" id="val-3">90°</span>
        </div>
        <div class="slider-container">
          <input type="range" min="0" max="180" value="90" id="slider-3" oninput="sendSliderValue(3, this.value)">
        </div>
      </div>

      <!-- Servo 5 / Gripper -->
      <div class="servo-control disabled" id="control-4">
        <div class="servo-header">
          <span class="servo-name">Servo 5 (Gripper)</span>
          <span class="servo-value" id="val-4">90°</span>
        </div>
        <div class="slider-container">
          <input type="range" min="0" max="180" value="90" id="slider-4" oninput="sendSliderValue(4, this.value)">
        </div>
      </div>
    </div>
  </div>

  <script>
    let ws;
    let reconnectTimeout;
    let lastSent = Array(5).fill(0);
    const minSendInterval = 20; // 50Hz rate limit
    let sendTimeout = Array(5).fill(null);

    function connect() {
      const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const host = window.location.hostname || '192.168.4.1';
      ws = new WebSocket(`${proto}//${host}:81/`);

      ws.onopen = () => {
        document.getElementById('statusDot').classList.add('connected');
        document.getElementById('statusText').innerText = 'Connected';
        clearTimeout(reconnectTimeout);
      };

      ws.onclose = () => {
        document.getElementById('statusDot').classList.remove('connected');
        document.getElementById('statusText').innerText = 'Disconnected';
        reconnectTimeout = setTimeout(connect, 2000);
      };

      ws.onerror = (err) => {
        console.error('Socket error:', err);
      };

      ws.onmessage = (event) => {
        const msg = event.data;
        if (msg.startsWith('INIT:')) {
          const parts = msg.split(':');
          const mode = parts[1];
          const angles = parts[2].split(',').map(Number);
          
          setWebModeUI(mode === 'web');
          document.getElementById('modeToggle').checked = (mode === 'web');
          
          angles.forEach((angle, idx) => {
            updateSliderUI(idx, angle);
          });
        } else if (msg.startsWith('M:')) {
          const mode = msg.substring(2);
          setWebModeUI(mode === 'web');
          document.getElementById('modeToggle').checked = (mode === 'web');
        } else if (msg.startsWith('ANGLES:')) {
          const angles = msg.substring(7).split(',').map(Number);
          angles.forEach((angle, idx) => {
            updateSliderUI(idx, angle);
          });
        } else {
          const parts = msg.split(':');
          if (parts.length === 2) {
            const ch = parseInt(parts[0]);
            const val = parseInt(parts[1]);
            updateSliderUI(ch, val);
          }
        }
      };
    }

    function updateSliderUI(ch, val) {
      const slider = document.getElementById(`slider-${ch}`);
      const valDisp = document.getElementById(`val-${ch}`);
      if (slider && valDisp) {
        slider.value = val;
        valDisp.innerText = val + '°';
      }
    }

    function setWebModeUI(enabled) {
      const desc = document.getElementById('modeDesc');
      desc.innerText = enabled ? 'Sliders active' : 'Follows transmitter via ESP-NOW';
      
      for (let i = 0; i < 5; i++) {
        const ctrl = document.getElementById(`control-${i}`);
        if (enabled) {
          ctrl.classList.remove('disabled');
        } else {
          ctrl.classList.add('disabled');
        }
      }
    }

    function toggleMode(checked) {
      const mode = checked ? 'web' : 'master';
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(`M:${mode}`);
      }
    }

    function sendSliderValue(ch, val) {
      document.getElementById(`val-${ch}`).innerText = val + '°';
      
      const now = Date.now();
      
      if (sendTimeout[ch]) {
        clearTimeout(sendTimeout[ch]);
        sendTimeout[ch] = null;
      }
      
      if (now - lastSent[ch] >= minSendInterval) {
        doSend(ch, val);
        lastSent[ch] = now;
      } else {
        sendTimeout[ch] = setTimeout(() => {
          doSend(ch, val);
          lastSent[ch] = Date.now();
        }, minSendInterval - (now - lastSent[ch]));
      }
    }

    function doSend(ch, val) {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(`${ch}:${val}`);
      }
    }

    window.onload = connect;
  </script>
</body>
</html>
)rawliteral";
