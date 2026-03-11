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
                    <button id="btn-calibrate" onclick="calibrateIMU()" class="btn-primary" style="width: 100%; background: transparent; border: 1px solid var(--accent-color); color: var(--accent-color);">
                        CALIBRATE IMU (ZERO YAW)
                    </button>
                    <p style="font-size: 0.8em; color: var(--text-muted); text-align: center; margin-top: 8px;">Keep the rover completely still before calibrating.</p>
                </div>
            </div>

            <div id="config" class="tab-content">
                <div class="dashboard-grid">
                    <div class="card">
                        <h3>System Settings</h3>
                        <div class="setting-item" style="margin-bottom: 15px;">
                            <label for="theme-toggle">Dark Theme</label>
                            <label class="switch">
                                <input type="checkbox" id="theme-toggle" onchange="toggleTheme()">
                                <span class="slider round"></span>
                            </label>
                        </div>
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
    /* Light Theme Palette */
    --base-bg: #f6f6f4;
    --card-bg: rgba(255, 255, 255, 0.65);
    --card-border: rgba(32, 34, 28, 0.15);
    
    --text-main: #20221c;
    --text-muted: #555555;
    
    --accent-color: #ce3b9b;
    --accent-hover: #b03284;
    --accent-text: #ffffff;
    --accent-bg-glow: rgba(206, 59, 155, 0.15);
    
    --watermelon: #e42548;
    --sapphire: #4e74bc;
    --iron-grey: #b0b0b0;
    
    --switch-bg: rgba(32, 34, 28, 0.2);
    --switch-thumb: #ffffff;
    
    --header-border: rgba(32, 34, 28, 0.15);
    --list-border: rgba(32, 34, 28, 0.1);
    --input-bg: rgba(255, 255, 255, 0.8);
    --input-border: rgba(32, 34, 28, 0.3);
}

[data-theme="dark"] {
    /* Dark Theme Palette */
    --base-bg: #161712;
    --card-bg: rgba(42, 44, 36, 0.4);
    --card-border: rgba(235, 251, 55, 0.15);
    
    --text-main: #fbf5f3;
    --text-muted: #a0a0a0;
    
    --accent-color: #ebfb37;
    --accent-hover: #c9d82d;
    --accent-text: #161712;
    --accent-bg-glow: rgba(235, 251, 55, 0.15);
    
    --watermelon: #e42548;
    --sapphire: #4e74bc;
    --iron-grey: #464646;
    
    --switch-bg: rgba(0,0,0,0.6);
    --switch-thumb: #a0a0a0;
    
    --header-border: rgba(235, 251, 55, 0.15);
    --list-border: rgba(251, 245, 243, 0.05);
    --input-bg: rgba(0,0,0,0.5);
    --input-border: var(--iron-grey);
}

* { box-sizing: border-box; }

body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background-color: var(--base-bg);
    color: var(--text-main);
    margin: 0;
    padding: 0;
    -webkit-font-smoothing: antialiased;
    transition: background-color 0.4s ease, color 0.4s ease;
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
    border-bottom: 1px solid var(--header-border);
    padding-bottom: 16px;
    transition: border-color 0.4s ease;
}

h1 { 
    margin: 0; 
    color: var(--text-main); 
    font-weight: 700;
    letter-spacing: -0.5px;
    text-transform: uppercase;
    transition: color 0.4s ease;
}
h1 span { color: var(--accent-color); transition: color 0.4s ease; }

.connection-status {
    padding: 6px 14px;
    background-color: var(--accent-bg-glow);
    color: var(--accent-color);
    border: 1px solid var(--accent-color);
    border-radius: 20px;
    font-size: 0.8rem;
    font-weight: 600;
    letter-spacing: 0.5px;
    text-transform: uppercase;
    transition: all 0.4s ease;
}

.tabs {
    display: flex;
    background: var(--card-bg);
    border: 1px solid var(--card-border);
    border-radius: 12px;
    overflow: hidden;
    margin-bottom: 24px;
    backdrop-filter: blur(16px);
    -webkit-backdrop-filter: blur(16px);
    transition: all 0.4s ease;
    box-shadow: 0 4px 24px rgba(0,0,0,0.04);
}

.tab-btn {
    flex: 1;
    padding: 16px;
    border: none;
    background: transparent;
    cursor: pointer;
    font-weight: 600;
    color: var(--text-muted);
    font-size: 0.9rem;
    letter-spacing: 0.5px;
    transition: all 0.3s ease;
}

.tab-btn:hover { color: var(--text-main); background: rgba(128,128,128,0.1); }

.tab-btn.active {
    color: var(--accent-text);
    background-color: var(--accent-color);
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
    backdrop-filter: blur(16px);
    -webkit-backdrop-filter: blur(16px);
    transition: all 0.4s ease;
    box-shadow: 0 8px 32px rgba(0,0,0,0.04);
}

.card:hover { 
    border-color: var(--accent-color);
    box-shadow: 0 8px 32px var(--accent-bg-glow);
}

.card h3 { 
    margin-top: 0; 
    color: var(--text-muted); 
    font-size: 0.85rem; 
    text-transform: uppercase; 
    letter-spacing: 1px;
    border-bottom: 1px solid var(--header-border); 
    padding-bottom: 12px; 
    margin-bottom: 16px;
    transition: all 0.4s ease;
}

ul { list-style: none; padding: 0; margin: 0; }
li { padding: 10px 0; border-bottom: 1px solid var(--list-border); display: flex; justify-content: space-between; align-items: center; transition: border-color 0.4s ease; }
li:last-child { border-bottom: none; }
li span.label { font-weight: 500; color: var(--text-muted); font-size: 0.9rem; transition: color 0.4s ease; }
li span.value { font-weight: 600; color: var(--text-main); font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; transition: color 0.4s ease; }

.channel-group { margin-bottom: 18px; }
.channel-group label { display: block; font-weight: 500; margin-bottom: 8px; font-size: 0.85rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.5px; transition: color 0.4s ease;}

.channel-row { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
.progress-bar { 
    background: var(--input-bg); 
    height: 8px; 
    border-radius: 4px; 
    overflow: hidden; 
    flex-grow: 1;
    border: 1px solid var(--iron-grey);
    transition: all 0.4s ease;
}
.progress-bar .fill { 
    background: var(--accent-color); 
    height: 100%; 
    width: 50%; 
    transition: width 0.1s linear, background-color 0.4s ease, box-shadow 0.4s ease; 
    box-shadow: 0 0 10px var(--accent-bg-glow);
}
.channel-group span.val { font-family: monospace; font-weight: 600; font-size: 0.95em; color: var(--text-main); min-width: 45px; text-align: right; transition: color 0.4s ease;}

/* Switch Toggle */
.switch { position: relative; display: inline-block; width: 46px; height: 24px; flex-shrink: 0; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--switch-bg); border: 1px solid var(--iron-grey); transition: .4s; }
.slider:before { position: absolute; content: ""; height: 16px; width: 16px; left: 3px; bottom: 3px; background-color: var(--switch-thumb); transition: .4s; }
input:checked + .slider { background-color: var(--accent-bg-glow); border-color: var(--accent-color); }
input:focus + .slider { box-shadow: 0 0 1px var(--accent-color); }
input:checked + .slider:before { transform: translateX(22px); background-color: var(--accent-color); box-shadow: 0 0 8px var(--accent-color); }
.slider.round { border-radius: 34px; }
.slider.round:before { border-radius: 50%; }

.setting-item { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; }

/* Custom Inputs & Buttons */
.custom-input {
    background: var(--input-bg);
    border: 1px solid var(--input-border);
    color: var(--text-main);
    padding: 8px 12px;
    border-radius: 6px;
    font-family: inherit;
    outline: none;
    transition: all 0.3s ease;
}
.custom-input:focus { border-color: var(--accent-color); box-shadow: 0 0 0 2px var(--accent-bg-glow); }

.btn-primary {
    background: var(--accent-color);
    color: var(--accent-text);
    border: none;
    padding: 12px;
    border-radius: 6px;
    font-weight: 700;
    letter-spacing: 1px;
    cursor: pointer;
    transition: all 0.3s ease;
    text-transform: uppercase;
    box-shadow: 0 4px 12px var(--accent-bg-glow);
}
.btn-primary:hover { opacity: 0.9; transform: translateY(-2px); box-shadow: 0 6px 16px var(--accent-bg-glow); }
.btn-primary:active { transform: translateY(1px); box-shadow: 0 2px 8px var(--accent-bg-glow); }

/* Armed Status Styles */
#armed-card { transition: all 0.4s ease; border-left: 4px solid transparent; margin-bottom: 24px; }
.armed-true { 
    background-color: rgba(228, 37, 72, 0.1) !important; 
    border-color: var(--watermelon) !important;
    border-left-width: 6px !important;
}
.armed-false { 
    background-color: var(--accent-bg-glow) !important; 
    border-color: var(--card-border) !important; 
    border-left-color: var(--accent-color) !important;
}
#armed-label { font-size: 1.1em; font-weight: 700; letter-spacing: 1px; color: var(--text-main); transition: color 0.4s ease; }

/* Sensor rows */
.sensor-row { display: flex; align-items: center; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid var(--list-border); gap: 12px; transition: border-color 0.4s ease;}
.sensor-row:last-child { border-bottom: none; }
.sensor-row .label { font-weight: 500; color: var(--text-muted); font-size: 0.9rem; flex-grow: 1; text-align: left; transition: color 0.4s ease;}
.sensor-row .sensor-data { display: flex; align-items: baseline; justify-content: flex-end; min-width: 80px; }
.sensor-row .value { font-family: ui-monospace, monospace; font-weight: 600; color: var(--text-main); margin-right: 6px; font-size: 1rem; text-align: right; transition: color 0.4s ease;}
.sensor-row .unit { font-size: 0.8em; color: var(--iron-grey); width: 20px; text-align: left; transition: color 0.4s ease;}

#imu-offline {
    border-color: var(--watermelon);
    background: rgba(228, 37, 72, 0.1);
    padding: 16px;
    margin-bottom: 20px;
    transition: all 0.4s ease;
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
function updateSysInfo() {
    fetch('/api/sysinfo')
        .then(response => response.json())
        .then(data => {
            const list = document.getElementById('sysinfo-list');
            list.innerHTML = `
                <li><span class="label">Chip Model</span> <span class="value">${data.chip_model} (Rev ${data.chip_revision})</span></li>
                <li><span class="label">CPU Freq</span> <span class="value">${data.cpu_freq} MHz</span></li>
                <li><span class="label">RAM (Total)</span> <span class="value">${formatBytes(data.heap_total)}</span></li>
                <li><span class="label">Flash (Total)</span> <span class="value">${formatBytes(data.flash_size)}</span></li>
                <li><span class="label">Sketch Size</span> <span class="value">${formatBytes(data.sketch_size)}</span></li>
                <li><span class="label">Uptime</span> <span class="value">${formatTime(data.uptime)}</span></li>
            `;
            
            const netList = document.getElementById('netinfo-list');
            let netHTML = `
                <li><span class="label">Mode</span> <span class="value">${data.mode}</span></li>
                <li><span class="label">SSID</span> <span class="value">${data.ssid}</span></li>
                <li><span class="label">IP Address</span> <span class="value">${data.ip}</span></li>
                <li><span class="label">MAC Address</span> <span class="value">${data.mac}</span></li>
                <li><span class="label">Channel</span> <span class="value">${data.channel}</span></li>
            `;
            
            if (data.mode === "Station") {
                netHTML += `<li><span class="label">Gateway</span> <span class="value">${data.gateway}</span></li>`;
            } else {
                netHTML += `<li><span class="label">Clients</span> <span class="value">${data.clients}</span></li>`;
            }
            
            netList.innerHTML = netHTML;
        })
        .catch(err => console.error('Error fetching sysinfo:', err));
}

function updateRadio() {
    fetch('/api/channels')
    .then(r => {
        if (!r.ok) throw new Error("Network response was not ok");
        return r.json();
    })
    .then(d => {
        // Se a resposta vier sem os atributos principais, aborta o update
        if (d.throttle === undefined || d.steering === undefined || !d.aux) {
            return;
        }
        
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
        // Theme Toggle
        const themeToggle = document.getElementById('theme-toggle');
        if(themeToggle) {
            themeToggle.checked = d.dark_theme;
            if(d.dark_theme) {
                document.body.setAttribute('data-theme', 'dark');
            } else {
                document.body.removeAttribute('data-theme');
            }
        }

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

function toggleTheme() {
    const enabled = document.getElementById('theme-toggle').checked;
    if(enabled) {
        document.body.setAttribute('data-theme', 'dark');
    } else {
        document.body.removeAttribute('data-theme');
    }
    fetch('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({dark_theme: enabled})
    }).catch(console.error);
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
                btn.style.color = "var(--accent-color)";
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

let sensorFailCount = 0;

function updateSensors() {
    fetch('/api/sensors')
    .then(r => {
        if (!r.ok) throw new Error("Network response was not ok");
        return r.json();
    })
    .then(d => {
        const offline = document.getElementById('imu-offline');
        if (!d.valid) {
            sensorFailCount++;
            if (sensorFailCount > 3 && offline) {
                offline.style.display = 'block';
            }
            return;
        }
        
        sensorFailCount = 0;
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
    .catch(err => {
        console.error('Error fetching sensors:', err);
        sensorFailCount++;
        const offline = document.getElementById('imu-offline');
        if (sensorFailCount > 3 && offline) {
            offline.style.display = 'block';
        }
    });
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

// Variação de tempo dependendo da aba para reduzir a carga de polling no ESP32
let pollingInterval;

function startPolling() {
    if(pollingInterval) clearInterval(pollingInterval);
    
    let rate = 1000; // Padrão: 1 segundo
    
    // Se estiver no rádio ou sensores, pode precisar de atualização mais rápida
    if(document.getElementById('radio').classList.contains('active') || 
       document.getElementById('sensors').classList.contains('active')) {
        rate = 500;
    }
    
    pollingInterval = setInterval(() => {
        if(document.getElementById('home').classList.contains('active')) {
            updateSysInfo();
        }
        if(document.getElementById('radio').classList.contains('active')) {
            updateRadio();
        }
        if(document.getElementById('sensors').classList.contains('active')) {
            updateSensors();
        }
    }, rate);
}

function openTab(tabName) {
    const contents = document.querySelectorAll('.tab-content');
    contents.forEach(content => content.classList.remove('active'));
    
    const btns = document.querySelectorAll('.tab-btn');
    btns.forEach(btn => btn.classList.remove('active'));
    
    document.getElementById(tabName).classList.add('active');
    event.currentTarget.classList.add('active');
    
    startPolling();
}

// Initial load
document.addEventListener('DOMContentLoaded', () => {
    updateSysInfo();
    loadSettings();
    startPolling();
});
)rawliteral";

#endif
