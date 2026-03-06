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
            <h1>ESP32 Rover</h1>
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
                            <div class="progress-bar"><div id="ch-throttle" class="fill"></div></div>
                            <span id="val-throttle">1500</span>
                        </div>
                        <div class="channel-group">
                            <label>Steering</label>
                            <div class="progress-bar"><div id="ch-steering" class="fill"></div></div>
                            <span id="val-steering">1500</span>
                        </div>
                    </div>
                    <div class="card">
                        <h3>Auxiliary</h3>
                        <div id="aux-channels">Loading...</div>
                    </div>
                </div>
            </div>

            <div id="sensors" class="tab-content">
                <div class="placeholder-container">
                    <div class="icon">📊</div>
                    <h2>Sensor Data Coming Soon</h2>
                    <p>This module will display real-time sensor telemetry.</p>
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
                        <p style="font-size: 0.8em; color: #7f8c8d;">Enables detailed logging to the Serial Monitor via USB.</p>
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
    --primary-color: #2c3e50;
    --accent-color: #3498db;
    --bg-color: #f4f7f6;
    --card-bg: #ffffff;
    --text-color: #333;
}

body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background-color: var(--bg-color);
    color: var(--text-color);
    margin: 0;
    padding: 0;
}

.app-container {
    max-width: 800px;
    margin: 0 auto;
    padding: 20px;
}

header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
}

h1 { margin: 0; color: var(--primary-color); }

.connection-status {
    padding: 5px 10px;
    background-color: #2ecc71;
    color: white;
    border-radius: 15px;
    font-size: 0.8rem;
}

.tabs {
    display: flex;
    background: var(--card-bg);
    border-radius: 10px;
    overflow: hidden;
    box-shadow: 0 2px 5px rgba(0,0,0,0.05);
    margin-bottom: 20px;
}

.tab-btn {
    flex: 1;
    padding: 15px;
    border: none;
    background: none;
    cursor: pointer;
    font-weight: bold;
    color: #7f8c8d;
    transition: all 0.3s ease;
}

.tab-btn.active {
    color: var(--accent-color);
    border-bottom: 3px solid var(--accent-color);
    background-color: #ecf0f1;
}

.tab-content { display: none; animation: fadeIn 0.3s; }
.tab-content.active { display: block; }

.dashboard-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 20px;
}

.card {
    background: var(--card-bg);
    padding: 20px;
    border-radius: 10px;
    box-shadow: 0 4px 6px rgba(0,0,0,0.05);
}

.card h3 { margin-top: 0; border-bottom: 1px solid #eee; padding-bottom: 10px; }

ul { list-style: none; padding: 0; }
li { padding: 8px 0; border-bottom: 1px solid #f9f9f9; display: flex; justify-content: space-between; }
li span.label { font-weight: 500; color: #555; }
li span.value { font-weight: bold; color: #2c3e50; font-family: monospace; }

.placeholder-container {
    text-align: center;
    padding: 50px;
    color: #95a5a6;
}
.placeholder-container .icon { font-size: 3rem; margin-bottom: 10px; }

.channel-group { margin-bottom: 15px; }
.channel-group label { display: block; font-weight: bold; margin-bottom: 5px; font-size: 0.9em; color: #555; }
.progress-bar { background: #eee; height: 12px; border-radius: 6px; overflow: hidden; display: inline-block; width: 75%; vertical-align: middle; margin-right: 10px; }
.progress-bar .fill { background: var(--accent-color); height: 100%; width: 50%; transition: width 0.1s ease; }
.channel-group span { display: inline-block; width: 15%; text-align: right; font-family: monospace; font-weight: bold; font-size: 0.9em; }

/* Switch Toggle */
.switch { position: relative; display: inline-block; width: 50px; height: 28px; flex-shrink: 0; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; }
.slider:before { position: absolute; content: ""; height: 20px; width: 20px; left: 4px; bottom: 4px; background-color: white; transition: .4s; }
input:checked + .slider { background-color: var(--accent-color); }
input:focus + .slider { box-shadow: 0 0 1px var(--accent-color); }
input:checked + .slider:before { transform: translateX(22px); }
.slider.round { border-radius: 34px; }
.slider.round:before { border-radius: 50%; }
.setting-item { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }

/* Armed Status Styles */
#armed-card { transition: all 0.3s ease; border-left: 5px solid transparent; margin-bottom: 20px; }
.armed-true { background-color: #ffebee !important; border-left-color: #e53935 !important; }
.armed-false { background-color: #e8f5e9 !important; border-left-color: #43a047 !important; }
#armed-label { font-size: 1.1em; font-weight: 800; letter-spacing: 0.5px; }

@keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
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
                        <div class="progress-bar"><div id="ch-aux${i}" class="fill" style="width: 50%"></div></div>
                        <span id="val-aux${i}">${val}</span>
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
        label.style.color = "#c62828";
    } else {
        card.classList.remove('armed-true');
        card.classList.add('armed-false');
        label.innerText = "SYSTEM DISARMED (SAFE MODE)";
        label.style.color = "#2e7d32";
    }
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

// Update loop
setInterval(() => {
    if(document.getElementById('home').classList.contains('active')) {
        updateSysInfo();
    }
    if(document.getElementById('radio').classList.contains('active')) {
        updateRadio();
    }
}, 500);

// Initial load
document.addEventListener('DOMContentLoaded', () => {
    updateSysInfo();
    loadSettings();
});
)rawliteral";

#endif
