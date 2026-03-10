#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Rover</title>
    <link rel="stylesheet" href="style.css">
    <link rel="icon" href="data:,">
</head>
<body>
    <div class="app-container">
        <header>
            <h1>ESP32 <span>Rover</span></h1>
            <div class="connection-status" id="connectionStatus">Connected</div>
        </header>
        
        <nav class="tabs">
            <button class="tab-btn active" onclick="openTab('home')">HOME</button>
            <button class="tab-btn" onclick="openTab('radio')">RADIO</button>
            <button class="tab-btn" onclick="openTab('sensors')">SENSORS</button>
            <button class="tab-btn" onclick="openTab('config')">CONFIG</button>
        </nav>

        <main>
            <div id="home" class="tab-content active">
                <div class="card" id="armed-card">
                    <div class="setting-item" style="margin-bottom: 0;">
                        <label for="armed-toggle" id="armed-label">SYSTEM STATUS</label>
                        <label class="switch">
                            <input type="checkbox" id="armed-toggle" onchange="toggleArmed()">
                            <span class="slider round"></span>
                        </label>
                    </div>
                </div>

                <div class="dashboard-grid">
                    <div class="card">
                        <h3>System Info</h3>
                        <ul id="sysinfo-list">
                            <li>Loading...</li>
                        </ul>
                    </div>
                    <div class="card">
                        <h3>Network</h3>
                        <ul id="netinfo-list">
                            <li>Loading...</li>
                        </ul>
                    </div>
                </div>
            </div>

            <div id="radio" class="tab-content">
                <div class="dashboard-grid">
                    <div class="card">
                        <h3>Main Channels</h3>
                        <div class="channel-group">
                            <label>Throttle</label>
                            <div class="channel-row">
                                <div class="progress-bar"><div id="ch-throttle" class="fill"></div></div>
                                <span class="val" id="val-throttle">1500</span>
                            </div>
                        </div>
                        <div class="channel-group">
                            <label>Steering</label>
                            <div class="channel-row">
                                <div class="progress-bar"><div id="ch-steering" class="fill"></div></div>
                                <span class="val" id="val-steering">1500</span>
                            </div>
                        </div>
                    </div>
                    <div class="card">
                        <h3>Auxiliary</h3>
                        <div id="aux-channels">Loading...</div>
                    </div>
                </div>
            </div>

            <div id="sensors" class="tab-content">
                <div id="imu-offline" class="card" style="display:none; border-left: 4px solid var(--watermelon);">
                    <p style="margin:0; color:var(--watermelon); font-weight:bold; font-size: 0.9rem; text-transform: uppercase; letter-spacing: 1px;">⚠ IMU offline or data invalid</p>
                </div>
                <div class="dashboard-grid">
                    <div class="card">
                        <h3>Orientation</h3>
                        <div class="sensor-row"><span class="label">Roll</span><div class="sensor-data"><span class="value" id="imu-roll">--</span><span class="unit">°</span></div></div>
                        <div class="sensor-row"><span class="label">Pitch</span><div class="sensor-data"><span class="value" id="imu-pitch">--</span><span class="unit">°</span></div></div>
                        <div class="sensor-row"><span class="label">Yaw</span><div class="sensor-data"><span class="value" id="imu-yaw">--</span><span class="unit">°</span></div></div>
                        <div class="sensor-row" style="margin-top:12px;"><span class="label">Temperature</span><div class="sensor-data"><span class="value" id="imu-temp">--</span><span class="unit">°C</span></div></div>
                    </div>
                    <div class="card">
                        <h3>Accelerometer (g)</h3>
                        <div class="sensor-row"><span class="label">X</span><div class="sensor-data"><span class="value" id="accel-x">--</span><span class="unit"></span></div></div>
                        <div class="sensor-row"><span class="label">Y</span><div class="sensor-data"><span class="value" id="accel-y">--</span><span class="unit"></span></div></div>
                        <div class="sensor-row"><span class="label">Z</span><div class="sensor-data"><span class="value" id="accel-z">--</span><span class="unit"></span></div></div>
                    </div>
                    <div class="card">
                        <h3>Gyroscope (°/s)</h3>
                        <div class="sensor-row"><span class="label">X</span><div class="sensor-data"><span class="value" id="gyro-x">--</span><span class="unit"></span></div></div>
                        <div class="sensor-row"><span class="label">Y</span><div class="sensor-data"><span class="value" id="gyro-y">--</span><span class="unit"></span></div></div>
                        <div class="sensor-row"><span class="label">Z</span><div class="sensor-data"><span class="value" id="gyro-z">--</span><span class="unit"></span></div></div>
                    </div>
                    <div class="card">
                        <h3>Magnetometer (µT)</h3>
                        <div class="sensor-row"><span class="label">X</span><div class="sensor-data"><span class="value" id="mag-x">--</span><span class="unit"></span></div></div>
                        <div class="sensor-row"><span class="label">Y</span><div class="sensor-data"><span class="value" id="mag-y">--</span><span class="unit"></span></div></div>
                        <div class="sensor-row"><span class="label">Z</span><div class="sensor-data"><span class="value" id="mag-z">--</span><span class="unit"></span></div></div>
                    </div>
                </div>
                
                <div style="margin-top: 20px;">
                    <button id="btn-calibrate" onclick="calibrateIMU()" class="btn-primary" style="width: 100%; background: transparent; border: 1px solid var(--neon-chartreuse); color: var(--neon-chartreuse);">
                        CALIBRATE IMU (ZERO YAW)
                    </button>
                    <p style="font-size: 0.8em; color: var(--text-muted); text-align: center; margin-top: 8px;">Keep the rover completely still before calibrating.</p>
                </div>
            </div>

            <div id="config" class="tab-content">
                <div class="dashboard-grid">
                    <div class="card">
                        <h3>System Settings</h3>
                        <div class="setting-item">
                            <label for="debug-toggle">Serial Debug Output</label>
                            <label class="switch">
                                <input type="checkbox" id="debug-toggle" onchange="toggleDebug()">
                                <span class="slider round"></span>
                            </label>
                        </div>
                        <p style="font-size: 0.8em; color: var(--text-muted); margin-top: 4px;">Enables detailed logging to the Serial Monitor via USB.</p>
                    </div>
                    
                    <div class="card">
                        <h3>Wi-Fi Configuration</h3>
                        <div class="setting-item" style="margin-bottom: 15px;">
                            <label>Network Mode</label>
                            <select id="wifi-mode" onchange="toggleWifiFields()" class="custom-input">
                                <option value="0">Access Point (AP)</option>
                                <option value="1">Station (Client)</option>
                            </select>
                        </div>
                        
                        <div id="sta-fields" style="display:none; transition: all 0.3s;">
                            <div class="setting-item" style="flex-direction: column; align-items: flex-start;">
                                <label style="margin-bottom: 5px; font-size: 0.85em; color: var(--text-muted);">Router SSID</label>
                                <input type="text" id="sta-ssid" class="custom-input" placeholder="MyNetworkName" style="width: 100%;">
                            </div>
                            <div class="setting-item" style="flex-direction: column; align-items: flex-start; margin-top: 10px;">
                                <label style="margin-bottom: 5px; font-size: 0.85em; color: var(--text-muted);">Password</label>
                                <input type="password" id="sta-pass" class="custom-input" placeholder="••••••••" style="width: 100%;">
                            </div>
                        </div>
                        
                        <button onclick="saveNetworkSettings()" class="btn-primary" style="margin-top: 20px; width: 100%;">SAVE & REBOOT</button>
                    </div>
                </div>
            </div>
        </main>
    </div>
    <script src="script.js"></script>
</body>
</html>
)rawliteral";

const char style_css[] PROGMEM = R"rawliteral(
:root {
    --black: #000100;
    --neon-chartreuse: #ddf102;
    --iron-grey: #464646;
    --watermelon: #e83754;
    --bright-snow: #f8f8f8;
    
    --bg-color: #0d0e0d; /* Ligeiramente mais claro que o preto puro para contraste */
    --card-bg: rgba(70, 70, 70, 0.4);
    --card-border: rgba(70, 70, 70, 0.6);
    --text-main: var(--bright-snow);
    --text-muted: #a0a0a0;
}

* { box-sizing: border-box; }

body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background-color: var(--bg-color);
    color: var(--text-main);
    margin: 0;
    padding: 0;
    -webkit-font-smoothing: antialiased;
}

.app-container {
    max-width: 900px;
    margin: 0 auto;
    padding: 24px;
}

header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 32px;
    border-bottom: 1px solid var(--card-border);
    padding-bottom: 16px;
}

h1 { 
    margin: 0; 
    color: var(--bright-snow); 
    font-weight: 700;
    letter-spacing: -0.5px;
    text-transform: uppercase;
}
h1 span { color: var(--neon-chartreuse); }

.connection-status {
    padding: 6px 14px;
    background-color: rgba(221, 241, 2, 0.15);
    color: var(--neon-chartreuse);
    border: 1px solid var(--neon-chartreuse);
    border-radius: 20px;
    font-size: 0.8rem;
    font-weight: 600;
    letter-spacing: 0.5px;
    text-transform: uppercase;
}

.tabs {
    display: flex;
    background: var(--card-bg);
    border: 1px solid var(--card-border);
    border-radius: 12px;
    overflow: hidden;
    margin-bottom: 24px;
    backdrop-filter: blur(10px);
}

.tab-btn {
    flex: 1;
    padding: 16px;
    border: none;
    background: none;
    cursor: pointer;
    font-weight: 600;
    color: var(--text-muted);
    font-size: 0.9rem;
    letter-spacing: 0.5px;
    transition: all 0.2s ease;
}

.tab-btn:hover { color: var(--bright-snow); background: rgba(255,255,255,0.05); }

.tab-btn.active {
    color: var(--black);
    background-color: var(--neon-chartreuse);
}

.tab-content { display: none; animation: fadeIn 0.4s ease; }
.tab-content.active { display: block; }

.dashboard-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 20px;
}

.card {
    background: var(--card-bg);
    border: 1px solid var(--card-border);
    padding: 24px;
    border-radius: 12px;
    backdrop-filter: blur(10px);
    transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.card:hover { border-color: rgba(221, 241, 2, 0.3); }

.card h3 { 
    margin-top: 0; 
    color: var(--text-muted); 
    font-size: 0.85rem; 
    text-transform: uppercase; 
    letter-spacing: 1px;
    border-bottom: 1px solid var(--card-border); 
    padding-bottom: 12px; 
    margin-bottom: 16px;
}

ul { list-style: none; padding: 0; margin: 0; }
li { padding: 10px 0; border-bottom: 1px solid rgba(255,255,255,0.05); display: flex; justify-content: space-between; align-items: center; }
li:last-child { border-bottom: none; }
li span.label { font-weight: 500; color: var(--text-muted); font-size: 0.9rem; }
li span.value { font-weight: 600; color: var(--bright-snow); font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; }

.channel-group { margin-bottom: 18px; }
.channel-group label { display: block; font-weight: 500; margin-bottom: 8px; font-size: 0.85rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.5px;}

.channel-row { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
.progress-bar { 
    background: rgba(0,0,0,0.5); 
    height: 8px; 
    border-radius: 4px; 
    overflow: hidden; 
    flex-grow: 1; /* Preenche o espaço disponível */
    border: 1px solid var(--iron-grey);
}
.progress-bar .fill { 
    background: var(--neon-chartreuse); 
    height: 100%; 
    width: 50%; 
    transition: width 0.1s linear; 
    box-shadow: 0 0 10px rgba(221, 241, 2, 0.5);
}
.channel-group span.val { font-family: monospace; font-weight: 600; font-size: 0.95em; color: var(--bright-snow); min-width: 45px; text-align: right; }

/* Switch Toggle */
.switch { position: relative; display: inline-block; width: 46px; height: 24px; flex-shrink: 0; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: rgba(0,0,0,0.6); border: 1px solid var(--iron-grey); transition: .3s; }
.slider:before { position: absolute; content: ""; height: 16px; width: 16px; left: 3px; bottom: 3px; background-color: var(--text-muted); transition: .3s; }
input:checked + .slider { background-color: rgba(221, 241, 2, 0.2); border-color: var(--neon-chartreuse); }
input:focus + .slider { box-shadow: 0 0 1px var(--neon-chartreuse); }
input:checked + .slider:before { transform: translateX(22px); background-color: var(--neon-chartreuse); box-shadow: 0 0 8px var(--neon-chartreuse); }
.slider.round { border-radius: 34px; }
.slider.round:before { border-radius: 50%; }

.setting-item { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; }

/* Custom Inputs & Buttons */
.custom-input {
    background: rgba(0,0,0,0.5);
    border: 1px solid var(--iron-grey);
    color: var(--bright-snow);
    padding: 8px 12px;
    border-radius: 6px;
    font-family: inherit;
    outline: none;
    transition: border-color 0.2s;
}
.custom-input:focus { border-color: var(--neon-chartreuse); }

.btn-primary {
    background: var(--neon-chartreuse);
    color: var(--black);
    border: none;
    padding: 12px;
    border-radius: 6px;
    font-weight: 700;
    letter-spacing: 1px;
    cursor: pointer;
    transition: all 0.2s;
    text-transform: uppercase;
}
.btn-primary:hover { opacity: 0.9; transform: translateY(-1px); }
.btn-primary:active { transform: translateY(1px); }

/* Armed Status Styles */
#armed-card { transition: all 0.3s ease; border-left: 4px solid transparent; margin-bottom: 24px; }
.armed-true { 
    background-color: rgba(232, 55, 84, 0.1) !important; 
    border-color: var(--watermelon) !important;
    border-left-width: 6px !important;
}
.armed-false { 
    background-color: rgba(221, 241, 2, 0.05) !important; 
    border-color: var(--card-border) !important; 
    border-left-color: var(--neon-chartreuse) !important;
}
#armed-label { font-size: 1.1em; font-weight: 700; letter-spacing: 1px; color: var(--bright-snow); }

/* Sensor rows */
.sensor-row { display: flex; align-items: center; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid rgba(255,255,255,0.05); gap: 12px;}
.sensor-row:last-child { border-bottom: none; }
.sensor-row .label { font-weight: 500; color: var(--text-muted); font-size: 0.9rem; flex-grow: 1; text-align: left; }
.sensor-row .sensor-data { display: flex; align-items: baseline; justify-content: flex-end; min-width: 80px; }
.sensor-row .value { font-family: ui-monospace, monospace; font-weight: 600; color: var(--bright-snow); margin-right: 6px; font-size: 1rem; text-align: right; }
.sensor-row .unit { font-size: 0.8em; color: var(--iron-grey); width: 20px; text-align: left;}

#imu-offline {
    border-color: var(--watermelon);
    background: rgba(232, 55, 84, 0.1);
    padding: 16px;
    margin-bottom: 20px;
}

@keyframes fadeIn { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }

/* Responsivo para Smartphone */
@media (max-width: 600px) {
    .app-container { padding: 12px; }
    header { margin-bottom: 20px; padding-bottom: 12px; }
    h1 { font-size: 1.5rem; }
    .connection-status { padding: 4px 10px; font-size: 0.7rem; }
    
    .tabs { flex-wrap: wrap; border-radius: 8px; margin-bottom: 16px; }
    .tab-btn { padding: 12px 6px; font-size: 0.75rem; flex: 1 1 50%; }
    .tab-btn:nth-child(1), .tab-btn:nth-child(2) { border-bottom: 1px solid var(--card-border); }
    
    .card { padding: 16px; }
    .dashboard-grid { gap: 16px; }
    
    #armed-label { font-size: 0.9rem; }
    
    li span.label, .sensor-row .label { font-size: 0.85rem; }
    li span.value, .sensor-row .value { font-size: 0.95rem; }
}
)rawliteral";

const char script_js[] PROGMEM = R"rawliteral(
function openTab(tabName) {
    const contents = document.querySelectorAll('.tab-content');
    contents.forEach(content => content.classList.remove('active'));
    
    const btns = document.querySelectorAll('.tab-btn');
    btns.forEach(btn => btn.classList.remove('active'));
    
    document.getElementById(tabName).classList.add('active');
    event.currentTarget.classList.add('active');
}

function updateSysInfo() {
    fetch('/api/sysinfo')
        .then(response => response.json())
        .then(data => {
            const list = document.getElementById('sysinfo-list');
            list.innerHTML = `
                <li><span class="label">Chip Model</span> <span class="value">${data.chip_model}</span></li>
                <li><span class="label">Revision</span> <span class="value">${data.chip_revision}</span></li>
                <li><span class="label">Free Heap</span> <span class="value">${formatBytes(data.free_heap)}</span></li>
                <li><span class="label">Uptime</span> <span class="value">${formatTime(data.uptime)}</span></li>
            `;
            
            const netList = document.getElementById('netinfo-list');
            netList.innerHTML = `
                <li><span class="label">IP Address</span> <span class="value">${data.ip}</span></li>
                <li><span class="label">WiFi Mode</span> <span class="value">AP</span></li>
                <li><span class="label">SSID</span> <span class="value">${data.ssid}</span></li>
            `;
        })
        .catch(err => console.error('Error fetching sysinfo:', err));
}

function updateRadio() {
    fetch('/api/channels')
    .then(r => r.json())
    .then(d => {
        updateChannel('throttle', d.throttle);
        updateChannel('steering', d.steering);
        
        const auxContainer = document.getElementById('aux-channels');
        if(auxContainer.innerHTML === 'Loading...' || auxContainer.innerHTML === '') {
            auxContainer.innerHTML = '';
            d.aux.forEach((val, i) => {
                auxContainer.innerHTML += `
                    <div class="channel-group">
                        <label>AUX ${i+1}</label>
                        <div class="channel-row">
                            <div class="progress-bar"><div id="ch-aux${i}" class="fill" style="width: 50%"></div></div>
                            <span class="val" id="val-aux${i}">${val}</span>
                        </div>
                    </div>`;
            });
        }
        
        d.aux.forEach((val, i) => {
            const el = document.getElementById(`ch-aux${i}`);
            if(el) updateChannel(`aux${i}`, val);
        });
    })
    .catch(err => console.error('Error fetching channels:', err));
}

function updateChannel(id, val) {
    let pct = ((val - 1000) / 1000) * 100;
    pct = Math.max(0, Math.min(100, pct));
    const bar = document.getElementById(`ch-${id}`);
    const txt = document.getElementById(`val-${id}`);
    if(bar) bar.style.width = `${pct}%`;
    if(txt) txt.innerText = val;
}

function loadSettings() {
    fetch('/api/settings')
    .then(r => r.json())
    .then(d => {
        // Debug Toggle
        const debugToggle = document.getElementById('debug-toggle');
        if(debugToggle) debugToggle.checked = d.debug;

        // Armed Toggle
        const armedToggle = document.getElementById('armed-toggle');
        if(armedToggle) {
            armedToggle.checked = d.armed;
            updateArmedStyle(d.armed);
        }
        
        // WiFi Settings
        const wifiMode = document.getElementById('wifi-mode');
        const staSsid = document.getElementById('sta-ssid');
        if(wifiMode) {
            wifiMode.value = d.wifi_mode;
            toggleWifiFields();
        }
        if(staSsid && d.sta_ssid) staSsid.value = d.sta_ssid;
    })
    .catch(console.error);
}

function toggleWifiFields() {
    const mode = document.getElementById('wifi-mode').value;
    const fields = document.getElementById('sta-fields');
    if (mode == "1") {
        fields.style.display = "block";
    } else {
        fields.style.display = "none";
    }
}

function saveNetworkSettings() {
    const mode = parseInt(document.getElementById('wifi-mode').value);
    const ssid = document.getElementById('sta-ssid').value;
    const pass = document.getElementById('sta-pass').value;
    
    if (mode === 1 && ssid.trim() === '') {
        alert("Please enter a valid SSID for Station Mode.");
        return;
    }
    
    const payload = { wifi_mode: mode };
    if (mode === 1) {
        payload.sta_ssid = ssid;
        if (pass) payload.sta_pass = pass;
    }
    
    fetch('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(payload)
    })
    .then(r => r.json())
    .then(d => {
        if(d.status === "rebooting") {
            alert("Network settings saved. Rover is rebooting to apply changes. You may need to connect to the new network.");
        }
    })
    .catch(console.error);
}

function toggleDebug() {
    const enabled = document.getElementById('debug-toggle').checked;
    fetch('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({debug: enabled})
    }).catch(console.error);
}

function calibrateIMU() {
    const btn = document.getElementById('btn-calibrate');
    const originalText = btn.innerText;
    
    btn.innerText = "CALIBRATING...";
    btn.disabled = true;
    btn.style.opacity = "0.5";
    
    fetch('/api/calibrate-imu', { method: 'POST' })
    .then(r => r.json())
    .then(d => {
        // A calibração assíncrona leva ~2.5s no backend
        setTimeout(() => {
            btn.innerText = "CALIBRATION COMPLETE";
            btn.style.color = "var(--black)";
            btn.style.background = "var(--neon-chartreuse)";
            
            setTimeout(() => {
                btn.innerText = originalText;
                btn.disabled = false;
                btn.style.opacity = "1";
                btn.style.color = "var(--neon-chartreuse)";
                btn.style.background = "transparent";
            }, 3000);
        }, 2500);
    })
    .catch(err => {
        console.error(err);
        btn.innerText = "ERROR";
        setTimeout(() => {
            btn.innerText = originalText;
            btn.disabled = false;
            btn.style.opacity = "1";
        }, 2000);
    });
}

function toggleArmed() {
    const enabled = document.getElementById('armed-toggle').checked;
    updateArmedStyle(enabled);
    fetch('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({armed: enabled})
    }).catch(console.error);
}

function updateArmedStyle(isArmed) {
    const card = document.getElementById('armed-card');
    const label = document.getElementById('armed-label');
    
    if (isArmed) {
        card.classList.remove('armed-false');
        card.classList.add('armed-true');
        label.innerText = "SYSTEM ARMED (MOTORS ACTIVE)";
        label.style.color = "var(--watermelon)";
    } else {
        card.classList.remove('armed-true');
        card.classList.add('armed-false');
        label.innerText = "SYSTEM DISARMED (SAFE MODE)";
        label.style.color = "var(--text-muted)";
    }
}

function updateSensors() {
    fetch('/api/sensors')
    .then(r => r.json())
    .then(d => {
        const offline = document.getElementById('imu-offline');
        if (!d.valid) {
            if (offline) offline.style.display = 'block';
            return;
        }
        if (offline) offline.style.display = 'none';

        const fmt = v => (v >= 0 ? '+' : '') + v.toFixed(3);

        document.getElementById('imu-roll').innerText  = fmt(d.angles.roll);
        document.getElementById('imu-pitch').innerText = fmt(d.angles.pitch);
        document.getElementById('imu-yaw').innerText   = fmt(d.angles.yaw);
        document.getElementById('imu-temp').innerText  = d.temperature.toFixed(1);

        document.getElementById('accel-x').innerText = fmt(d.accel.x);
        document.getElementById('accel-y').innerText = fmt(d.accel.y);
        document.getElementById('accel-z').innerText = fmt(d.accel.z);

        document.getElementById('gyro-x').innerText = fmt(d.gyro.x);
        document.getElementById('gyro-y').innerText = fmt(d.gyro.y);
        document.getElementById('gyro-z').innerText = fmt(d.gyro.z);

        document.getElementById('mag-x').innerText = fmt(d.mag.x);
        document.getElementById('mag-y').innerText = fmt(d.mag.y);
        document.getElementById('mag-z').innerText = fmt(d.mag.z);
    })
    .catch(err => console.error('Error fetching sensors:', err));
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatTime(ms) {
    const seconds = Math.floor(ms / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    return `${hours}h ${minutes % 60}m ${seconds % 60}s`;
}

// Update loop — executa apenas para a aba visível, evitando requisições desnecessárias.
setInterval(() => {
    if(document.getElementById('home').classList.contains('active')) {
        updateSysInfo();
    }
    if(document.getElementById('radio').classList.contains('active')) {
        updateRadio();
    }
    if(document.getElementById('sensors').classList.contains('active')) {
        updateSensors();
    }
}, 500);

// Initial load
document.addEventListener('DOMContentLoaded', () => {
    updateSysInfo();
    loadSettings();
});
)rawliteral";

#endif
