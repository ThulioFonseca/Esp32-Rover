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
            <button class="tab-btn" onclick="openTab('sensors')">SENSORS</button>
            <button class="tab-btn" onclick="openTab('config')">CONFIG</button>
        </nav>

        <main>
            <div id="home" class="tab-content active">
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

            <div id="sensors" class="tab-content">
                <div class="placeholder-container">
                    <div class="icon">📊</div>
                    <h2>Sensor Data Coming Soon</h2>
                    <p>This module will display real-time sensor telemetry.</p>
                </div>
            </div>

            <div id="config" class="tab-content">
                <div class="placeholder-container">
                    <div class="icon">⚙️</div>
                    <h2>Configuration Coming Soon</h2>
                    <p>System parameters and pinout configuration will be available here.</p>
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

// Update every 2 seconds if on Home tab
setInterval(() => {
    if(document.getElementById('home').classList.contains('active')) {
        updateSysInfo();
    }
}, 2000);

// Initial load
document.addEventListener('DOMContentLoaded', updateSysInfo);
)rawliteral";

#endif
