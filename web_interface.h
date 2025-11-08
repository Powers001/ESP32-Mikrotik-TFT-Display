#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

const char* html_page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Mikrotik Display Config</title>
<style>
:root {
  --primary: #667eea;
  --primary-dark: #5568d3;
  --secondary: #764ba2;
  --success: #4caf50;
  --warning: #ff9800;
  --danger: #f44336;
  --dark: #1a1a1a;
  --gray-100: #f8f9fa;
  --gray-200: #e9ecef;
  --gray-300: #dee2e6;
  --gray-700: #495057;
  --gray-900: #212529;
  --card-bg: rgba(255, 255, 255, 0.95);
  --text-primary: #212529;
  --text-secondary: #6c757d;
  --input-bg: white;
  --input-text: #212529;
  --shadow: 0 10px 40px rgba(0,0,0,0.1);
  --shadow-lg: 0 20px 60px rgba(0,0,0,0.3);
  --border-radius: 12px;
  --transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

[data-theme="dark"] {
  --card-bg: rgba(30, 30, 30, 0.95);
  --text-primary: #f8f9fa;
  --text-secondary: #adb5bd;
  --gray-100: #343a40;
  --gray-200: #495057;
  --gray-300: #6c757d;
  --input-bg: #2d3238;
  --input-text: #f8f9fa;
  --shadow: 0 10px 40px rgba(0,0,0,0.5);
}

* { 
  margin: 0; 
  padding: 0; 
  box-sizing: border-box; 
}

body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
  background: linear-gradient(135deg, var(--primary) 0%, var(--secondary) 100%);
  min-height: 100vh;
  padding: 20px;
  line-height: 1.6;
  color: var(--text-primary);
  transition: var(--transition);
}

.container {
  max-width: 900px;
  margin: 0 auto;
}

.header {
  text-align: center;
  margin-bottom: 30px;
  animation: fadeInDown 0.6s ease-out;
}

.header h1 {
  color: white;
  font-size: clamp(24px, 5vw, 36px);
  font-weight: 700;
  margin-bottom: 8px;
  text-shadow: 0 2px 10px rgba(0,0,0,0.2);
}

.header .subtitle {
  color: rgba(255,255,255,0.9);
  font-size: clamp(14px, 3vw, 16px);
}

.theme-toggle {
  position: fixed;
  top: 20px;
  right: 20px;
  width: 50px;
  height: 50px;
  border-radius: 50%;
  background: var(--card-bg);
  border: none;
  cursor: pointer;
  box-shadow: var(--shadow);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 24px;
  transition: var(--transition);
  z-index: 1000;
}

.theme-toggle:hover {
  transform: scale(1.1) rotate(15deg);
}

.card {
  background: var(--card-bg);
  backdrop-filter: blur(10px);
  border-radius: var(--border-radius);
  padding: 30px;
  box-shadow: var(--shadow-lg);
  margin-bottom: 20px;
  animation: fadeInUp 0.6s ease-out;
  animation-fill-mode: both;
}

.card:nth-child(2) { animation-delay: 0.1s; }
.card:nth-child(3) { animation-delay: 0.2s; }
.card:nth-child(4) { animation-delay: 0.3s; }

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
  gap: 15px;
  margin-bottom: 20px;
}

.stat-card {
  background: linear-gradient(135deg, var(--primary) 0%, var(--secondary) 100%);
  color: white;
  padding: 20px;
  border-radius: var(--border-radius);
  text-align: center;
  transition: var(--transition);
  cursor: pointer;
}

.stat-card:hover {
  transform: translateY(-5px);
  box-shadow: 0 15px 35px rgba(102,126,234,0.4);
}

.stat-value {
  font-size: clamp(18px, 3.5vw, 24px);
  font-weight: 700;
  margin-bottom: 5px;
  display: block;
}

.stat-label {
  font-size: 11px;
  opacity: 0.9;
  text-transform: uppercase;
  letter-spacing: 1px;
}

.info-badge {
  display: inline-block;
  background: var(--gray-200);
  color: var(--text-primary);
  padding: 8px 16px;
  border-radius: 20px;
  font-size: 13px;
  font-weight: 600;
  margin: 5px;
}

.section-title {
  color: var(--text-primary);
  font-size: 20px;
  font-weight: 600;
  margin: 30px 0 20px 0;
  padding-bottom: 10px;
  border-bottom: 2px solid var(--gray-300);
  display: flex;
  align-items: center;
  gap: 10px;
}

.section-title::before {
  content: '';
  width: 4px;
  height: 24px;
  background: linear-gradient(135deg, var(--primary), var(--secondary));
  border-radius: 2px;
}

.form-group {
  margin-bottom: 20px;
}

label {
  display: block;
  color: var(--text-primary);
  font-weight: 600;
  margin-bottom: 8px;
  font-size: 14px;
}

input, select {
  width: 100%;
  padding: 12px 15px;
  border: 2px solid var(--gray-300);
  border-radius: 8px;
  font-size: 14px;
  transition: var(--transition);
  background: var(--input-bg);
  color: var(--input-text);
}

input:focus, select:focus {
  outline: none;
  border-color: var(--primary);
  box-shadow: 0 0 0 3px rgba(102,126,234,0.1);
}

select option {
  background: var(--input-bg);
  color: var(--input-text);
}

.slider-container {
  display: flex;
  align-items: center;
  gap: 15px;
}

.slider-input {
  flex: 1;
}

.slider-value {
  min-width: 60px;
  text-align: center;
  font-weight: 600;
  color: var(--primary);
  font-size: 18px;
  background: var(--gray-100);
  padding: 8px 15px;
  border-radius: 8px;
}

input[type="range"] {
  height: 8px;
  border-radius: 5px;
  background: linear-gradient(to right, 
    var(--gray-300) 0%, 
    var(--primary) 0%, 
    var(--primary) var(--slider-percent, 100%), 
    var(--gray-300) var(--slider-percent, 100%));
  outline: none;
  padding: 0;
  border: none;
}

input[type="range"]::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 24px;
  height: 24px;
  border-radius: 50%;
  background: linear-gradient(135deg, var(--primary), var(--secondary));
  cursor: pointer;
  box-shadow: 0 2px 10px rgba(102,126,234,0.5);
  transition: var(--transition);
}

input[type="range"]::-webkit-slider-thumb:hover {
  transform: scale(1.2);
  box-shadow: 0 4px 20px rgba(102,126,234,0.7);
}

input[type="range"]::-moz-range-thumb {
  width: 24px;
  height: 24px;
  border-radius: 50%;
  background: linear-gradient(135deg, var(--primary), var(--secondary));
  cursor: pointer;
  border: none;
  box-shadow: 0 2px 10px rgba(102,126,234,0.5);
}

.input-group {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 15px;
}

.btn {
  width: 100%;
  padding: 14px;
  background: linear-gradient(135deg, var(--primary) 0%, var(--secondary) 100%);
  color: white;
  border: none;
  border-radius: 8px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: var(--transition);
  margin-top: 10px;
  position: relative;
  overflow: hidden;
}

.btn::before {
  content: '';
  position: absolute;
  top: 50%;
  left: 50%;
  width: 0;
  height: 0;
  border-radius: 50%;
  background: rgba(255,255,255,0.3);
  transform: translate(-50%, -50%);
  transition: width 0.6s, height 0.6s;
}

.btn:hover::before {
  width: 300px;
  height: 300px;
}

.btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 10px 25px rgba(102,126,234,0.3);
}

.btn:active {
  transform: translateY(0);
}

.btn-scan {
  background: linear-gradient(135deg, #00bcd4, #009688);
  margin-bottom: 10px;
}

.alert {
  background: rgba(76,175,80,0.1);
  border-left: 4px solid var(--success);
  padding: 15px;
  border-radius: 8px;
  margin-top: 20px;
  display: none;
  animation: slideIn 0.3s ease-out;
}

.alert.show { 
  display: block; 
}

.alert.error {
  background: rgba(244,67,54,0.1);
  border-left-color: var(--danger);
}

.alert.warning {
  background: rgba(255,152,0,0.1);
  border-left-color: var(--warning);
}

.help-text {
  font-size: 12px;
  color: var(--text-secondary);
  margin-top: 5px;
}

.ota-section {
  margin-top: 30px;
  padding-top: 30px;
  border-top: 2px dashed var(--gray-300);
}

.ota-link {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 12px 24px;
  background: linear-gradient(135deg, var(--warning), #ff6f00);
  color: white;
  text-decoration: none;
  border-radius: 8px;
  font-weight: 600;
  transition: var(--transition);
  box-shadow: 0 4px 15px rgba(255,152,0,0.3);
}

.ota-link:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(255,152,0,0.4);
}

.loading {
  display: inline-block;
  width: 16px;
  height: 16px;
  border: 3px solid rgba(255,255,255,.3);
  border-radius: 50%;
  border-top-color: white;
  animation: spin 1s ease-in-out infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

@keyframes fadeInDown {
  from {
    opacity: 0;
    transform: translateY(-20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@keyframes fadeInUp {
  from {
    opacity: 0;
    transform: translateY(20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@keyframes slideIn {
  from {
    opacity: 0;
    transform: translateX(-20px);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

@media (max-width: 768px) {
  body {
    padding: 10px;
  }
  
  .card {
    padding: 20px;
  }
  
  .stats-grid {
    grid-template-columns: repeat(2, 1fr);
  }
  
  .theme-toggle {
    width: 45px;
    height: 45px;
    top: 15px;
    right: 15px;
  }
  
  .input-group {
    grid-template-columns: 1fr;
  }
}
</style>
</head>
<body>
<button class="theme-toggle" id="themeToggle" aria-label="Toggle theme">🌙</button>

<div class="container">
  <div class="header">
    <h1>🌐 Mikrotik Display</h1>
    <div class="subtitle">Network Monitoring Dashboard</div>
  </div>

  <div class="card">
    <div id="deviceInfo" style="text-align: center; margin-bottom: 20px;">
      <span class="info-badge" id="ipAddress">IP: Loading...</span>
    </div>

    <div class="stats-grid" id="stats">
      <div class="stat-card">
        <span class="stat-value" id="cpu">--</span>
        <span class="stat-label">CPU Load</span>
      </div>
      <div class="stat-card">
        <span class="stat-value" id="ram">--</span>
        <span class="stat-label">RAM Used</span>
      </div>
      <div class="stat-card">
        <span class="stat-value" id="rx">--</span>
        <span class="stat-label">RX Speed</span>
      </div>
      <div class="stat-card">
        <span class="stat-value" id="tx">--</span>
        <span class="stat-label">TX Speed</span>
      </div>
    </div>

    <div class="section-title">Display Settings</div>
    
    <div class="form-group">
      <label>Backlight Brightness</label>
      <div class="slider-container">
        <input type="range" class="slider-input" id="backlightSlider" min="0" max="100" value="100">
        <div class="slider-value"><span id="backlightValue">100</span>%</div>
      </div>
      <div class="help-text">Adjust screen brightness (0-100%)</div>
    </div>
  </div>

  <div class="card">
    <div class="section-title">WiFi Settings</div>
    <form id="wifiForm">
      <button type="button" class="btn btn-scan" id="scanBtn">
        📡 Scan WiFi Networks
      </button>
      
      <div class="form-group">
        <label>WiFi Network</label>
        <select name="ssid" id="ssid" required>
          <option value="">Click 'Scan WiFi Networks' first</option>
        </select>
      </div>
      
      <div class="form-group">
        <label>WiFi Password</label>
        <input type="password" name="password" id="password" placeholder="Enter WiFi password" required>
      </div>

      <button type="submit" class="btn">💾 Save WiFi & Restart</button>
      <div class="alert" id="wifiAlert"></div>
    </form>
  </div>

  <div class="card">
    <div class="section-title">Router Settings</div>
    <form id="routerForm">
      <div class="form-group">
        <label>Router Address</label>
        <input type="text" name="router_addr" id="router_addr" placeholder="http://192.168.1.1" required>
        <div class="help-text">Full URL including http://</div>
      </div>
      
      <div class="form-group">
        <label>Router API Account Username</label>
        <input type="text" name="router_user" id="router_user" placeholder="APIUser" required>
      </div>
      
      <div class="form-group">
        <label>Router API Account Password</label>
        <input type="password" name="router_pass" id="router_pass" placeholder="Router API password" required>
      </div>
      
      <div class="form-group">
        <label>Interface to Monitor</label>
        <select name="interface_id" id="interface_id" required>
          <option value="">Loading interfaces...</option>
        </select>
        <div class="help-text">Select the network interface to monitor</div>
      </div>

      <button type="submit" class="btn">💾 Save Router Settings & Restart</button>
      <div class="alert" id="routerAlert"></div>
    </form>
  </div>

  <div class="card">
    <div class="section-title">Graph Settings</div>
    <form id="graphForm">
      <div class="form-group">
        <label>Graph Speed Range</label>
        <div class="input-group">
          <div>
            <label style="font-size: 12px; font-weight: normal;">Minimum (Mbps)</label>
            <input type="number" name="min_mbps" id="min_mbps" value="0" min="0" max="10000" required>
          </div>
          <div>
            <label style="font-size: 12px; font-weight: normal;">Maximum (Mbps)</label>
            <input type="number" name="max_mbps" id="max_mbps" value="480" min="1" max="10000" required>
          </div>
        </div>
        <div class="help-text">Set the Y-axis range for the traffic graph</div>
      </div>

      <button type="submit" class="btn">💾 Save Graph Settings & Restart</button>
      <div class="alert" id="graphAlert"></div>
    </form>

    <div class="ota-section">
      <div class="section-title">Firmware Update</div>
      <a href="/update" class="ota-link" target="_blank">
        <span>🔄</span>
        <span>Open OTA Update Portal</span>
      </a>
      <div class="help-text" style="margin-top: 10px;">Upload new firmware wirelessly without USB cable</div>
    </div>
  </div>
</div>

<script>
const themeToggle = document.getElementById('themeToggle');
const html = document.documentElement;
const backlightSlider = document.getElementById('backlightSlider');
const backlightValue = document.getElementById('backlightValue');
const scanBtn = document.getElementById('scanBtn');
const ssidSelect = document.getElementById('ssid');

let currentTheme = localStorage.getItem('theme') || 'light';
html.setAttribute('data-theme', currentTheme);
themeToggle.textContent = currentTheme === 'dark' ? '☀️' : '🌙';

themeToggle.addEventListener('click', () => {
  currentTheme = currentTheme === 'light' ? 'dark' : 'light';
  html.setAttribute('data-theme', currentTheme);
  localStorage.setItem('theme', currentTheme);
  themeToggle.textContent = currentTheme === 'dark' ? '☀️' : '🌙';
  
  fetch('/api/theme', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({theme: currentTheme})
  }).catch(e => console.error('Theme save failed:', e));
});

function updateSliderBackground(slider, value) {
  const percent = value || slider.value;
  slider.style.setProperty('--slider-percent', percent + '%');
}

backlightSlider.addEventListener('input', function() {
  backlightValue.textContent = this.value;
  updateSliderBackground(this);
  
  fetch('/api/backlight', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({brightness: parseInt(this.value)})
  }).catch(e => console.error('Backlight update failed:', e));
});

updateSliderBackground(backlightSlider);

scanBtn.addEventListener('click', function() {
  const originalText = this.innerHTML;
  this.innerHTML = '<span class="loading"></span> Scanning...';
  this.disabled = true;
  
  fetch('/api/scan')
    .then(r => r.json())
    .then(data => {
      ssidSelect.innerHTML = '<option value="">Select a network</option>';
      
      if (data.networks && data.networks.length > 0) {
        data.networks.forEach(net => {
          const option = document.createElement('option');
          option.value = net.ssid;
          option.innerHTML = `${net.ssid} (${net.strength} ${net.rssi}dBm)`;
          ssidSelect.appendChild(option);
        });
      } else {
        ssidSelect.innerHTML = '<option value="">No networks found</option>';
      }
      
      this.innerHTML = originalText;
      this.disabled = false;
    })
    .catch(e => {
      console.error('WiFi scan failed:', e);
      this.innerHTML = originalText;
      this.disabled = false;
      alert('WiFi scan failed. Please try again.');
    });
});

function loadInterfaces(selectedInterface) {
  fetch('/api/interfaces')
    .then(r => r.json())
    .then(data => {
      const ifaceSelect = document.getElementById('interface_id');
      ifaceSelect.innerHTML = '<option value="">Select interface</option>';
      
      if (data.interfaces && data.interfaces.length > 0) {
        data.interfaces.forEach(iface => {
          const option = document.createElement('option');
          option.value = iface.id;
          option.textContent = `${iface.name} (ID: ${iface.id})`;
          if (selectedInterface && iface.id == selectedInterface) {
            option.selected = true;
          }
          ifaceSelect.appendChild(option);
        });
      }
    })
    .catch(e => console.error('Interface load failed:', e));
}

function updateStats() {
  fetch('/api/stats')
    .then(r => r.json())
    .then(data => {
      document.getElementById('cpu').textContent = data.cpu + '%';
      document.getElementById('ram').textContent = data.ram;
      document.getElementById('rx').textContent = data.rx + ' Mbps';
      document.getElementById('tx').textContent = data.tx + ' Mbps';
      
      if (data.ip) {
        document.getElementById('ipAddress').textContent = 'IP: ' + data.ip;
      }
    })
    .catch(e => {
      console.error('Stats fetch failed:', e);
      ['cpu', 'ram', 'rx', 'tx'].forEach(id => {
        document.getElementById(id).textContent = 'N/A';
      });
    });
}

document.getElementById('wifiForm').addEventListener('submit', function(e) {
  e.preventDefault();
  const formData = new FormData(e.target);
  const data = Object.fromEntries(formData);
  
  const alertBox = document.getElementById('wifiAlert');
  alertBox.textContent = 'Saving WiFi configuration...';
  alertBox.className = 'alert show';
  
  fetch('/save-wifi', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(data)
  })
  .then(r => {
    if (!r.ok) throw new Error('Save failed');
    return r.json();
  })
  .then(data => {
    alertBox.textContent = '✓ WiFi configuration saved! Device restarting...';
    alertBox.className = 'alert show';
  })
  .catch(e => {
    alertBox.textContent = '✗ Error: ' + e.message;
    alertBox.className = 'alert show error';
  });
});

document.getElementById('routerForm').addEventListener('submit', function(e) {
  e.preventDefault();
  const formData = new FormData(e.target);
  const data = Object.fromEntries(formData);
  
  const alertBox = document.getElementById('routerAlert');
  alertBox.textContent = 'Saving router configuration...';
  alertBox.className = 'alert show';
  
  fetch('/save-router', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(data)
  })
  .then(r => {
    if (!r.ok) throw new Error('Save failed');
    return r.json();
  })
  .then(data => {
    alertBox.textContent = '✓ Router configuration saved! Device restarting...';
    alertBox.className = 'alert show';
  })
  .catch(e => {
    alertBox.textContent = '✗ Error: ' + e.message;
    alertBox.className = 'alert show error';
  });
});

document.getElementById('graphForm').addEventListener('submit', function(e) {
  e.preventDefault();
  const formData = new FormData(e.target);
  const data = Object.fromEntries(formData);
  
  const alertBox = document.getElementById('graphAlert');
  alertBox.textContent = 'Saving graph settings...';
  alertBox.className = 'alert show';
  
  fetch('/save-graph', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(data)
  })
  .then(r => {
    if (!r.ok) throw new Error('Save failed');
    return r.json();
  })
  .then(data => {
    alertBox.textContent = '✓ Graph settings saved! Device restarting...';
    alertBox.className = 'alert show';
  })
  .catch(e => {
    alertBox.textContent = '✗ Error: ' + e.message;
    alertBox.className = 'alert show error';
  });
});

fetch('/api/config')
  .then(r => r.json())
  .then(data => {
    if (data.ssid) {
      const option = document.createElement('option');
      option.value = data.ssid;
      option.textContent = data.ssid + ' (current)';
      option.selected = true;
      ssidSelect.appendChild(option);
    }
    if (data.router_addr) document.getElementById('router_addr').value = data.router_addr;
    if (data.router_user) document.getElementById('router_user').value = data.router_user;
    loadInterfaces(data.interface_id);
    if (data.max_mbps !== undefined) document.getElementById('max_mbps').value = data.max_mbps;
    if (data.min_mbps !== undefined) document.getElementById('min_mbps').value = data.min_mbps;
    if (data.backlight !== undefined) {
      backlightSlider.value = data.backlight;
      backlightValue.textContent = data.backlight;
      updateSliderBackground(backlightSlider);
    }
    if (data.theme) {
      currentTheme = data.theme;
      html.setAttribute('data-theme', currentTheme);
      localStorage.setItem('theme', currentTheme);
      themeToggle.textContent = currentTheme === 'dark' ? '☀️' : '🌙';
    }
    if (data.ip) {
      document.getElementById('ipAddress').textContent = 'IP: ' + data.ip;
    }
  })
  .catch(e => console.error('Config load failed:', e));

updateStats();
setInterval(updateStats, 5000);
</script>
</body>
</html>
)rawliteral";

#endif // WEB_INTERFACE_H