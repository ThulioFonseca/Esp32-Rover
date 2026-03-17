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
            <button class="tab-btn" onclick="openTab('hud-tab')">HUD</button>
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
                <div class="dashboard-grid" style="grid-template-columns: 1fr;">
                    <div class="card">
                        <h3>Receiver Channels</h3>
                        <div id="all-channels">Loading...</div>
                    </div>
                </div>
            </div>

            <div id="sensors" class="tab-content">
                <div id="imu-offline" class="card" style="display:none; border-left: 4px solid var(--watermelon); margin-bottom: 20px;">
                    <p style="margin:0; color:var(--watermelon); font-weight:bold; font-size: 0.9rem; text-transform: uppercase; letter-spacing: 1px;">⚠ IMU offline or data invalid</p>
                </div>
                
                <!-- GPS Card span 2 columns if space allows -->
                <div class="dashboard-grid" style="margin-bottom: 20px;">
                    <div class="card" style="grid-column: 1 / -1;">
                        <div style="display:flex; justify-content: space-between; align-items: center; border-bottom: 1px solid var(--header-border); padding-bottom: 12px; margin-bottom: 16px;">
                            <h3 style="border: none; padding: 0; margin: 0;">GPS Location</h3>
                            <span id="gps-status" style="font-size: 0.75rem; font-weight: bold; padding: 4px 8px; border-radius: 4px; background: rgba(228, 37, 72, 0.1); color: var(--watermelon); text-transform: uppercase;">NO FIX</span>
                        </div>
                        <div class="dashboard-grid">
                            <div>
                                <div class="sensor-row"><span class="label">Latitude</span><div class="sensor-data"><span class="value" id="gps-lat">--</span><span class="unit">°</span></div></div>
                                <div class="sensor-row"><span class="label">Longitude</span><div class="sensor-data"><span class="value" id="gps-lng">--</span><span class="unit">°</span></div></div>
                                <div class="sensor-row"><span class="label">Altitude</span><div class="sensor-data"><span class="value" id="gps-alt">--</span><span class="unit">m</span></div></div>
                                <div class="sensor-row"><span class="label">Satellites</span><div class="sensor-data"><span class="value" id="gps-sats">0</span><span class="unit"></span></div></div>
                            </div>
                            <div>
                                <div class="sensor-row"><span class="label">Speed</span><div class="sensor-data"><span class="value" id="gps-speed">--</span><span class="unit">km/h</span></div></div>
                                <div class="sensor-row"><span class="label">Course</span><div class="sensor-data"><span class="value" id="gps-course">--</span><span class="unit">°</span></div></div>
                                <div class="sensor-row"><span class="label">HDOP</span><div class="sensor-data"><span class="value" id="gps-hdop">--</span><span class="unit"></span></div></div>
                                <div class="sensor-row"><span class="label">Time (UTC-3)</span><div class="sensor-data"><span class="value" style="font-size:0.8rem" id="gps-time">--</span><span class="unit"></span></div></div>
                            </div>
                        </div>
                        <div style="border-top: 1px solid var(--header-border); margin-top: 16px; padding-top: 16px;">
                            <h3 style="border: none; padding: 0; margin-bottom: 12px; font-size: 0.8rem;">Compass (External Mag)</h3>
                            <div class="dashboard-grid">
                                <div>
                                    <div class="sensor-row"><span class="label">Heading</span><div class="sensor-data"><span class="value" id="comp-heading">--</span><span class="unit">°</span></div></div>
                                    <div class="sensor-row"><span class="label">Mag X</span><div class="sensor-data"><span class="value" id="comp-x">--</span><span class="unit"></span></div></div>
                                </div>
                                <div>
                                    <div class="sensor-row"><span class="label">Mag Y</span><div class="sensor-data"><span class="value" id="comp-y">--</span><span class="unit"></span></div></div>
                                    <div class="sensor-row"><span class="label">Mag Z</span><div class="sensor-data"><span class="value" id="comp-z">--</span><span class="unit"></span></div></div>
                                </div>
                            </div>
                        </div>
                    </div>
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
                </div>
                
                <div style="margin-top: 20px;">
                    <button id="btn-calibrate" onclick="calibrateIMU()" class="btn-primary" style="width: 100%; background: transparent; border: 1px solid var(--accent-color); color: var(--accent-color);">
                        CALIBRATE IMU (ZERO YAW)
                    </button>
                    <p style="font-size: 0.8em; color: var(--text-muted); text-align: center; margin-top: 8px;">Keep the rover completely still before calibrating.</p>
                </div>
            </div>

            <div id="hud-tab" class="tab-content">
                <div class="hud-viewport">
                    <div class="lens-fx"></div>
                    <div class="screen-fx"></div>

                    <div id="boot-screen">
                        <div class="boot-line" id="b1">> POWER ON... OK</div>
                        <div class="boot-line" id="b2">> MOUNTING FILESYSTEM... OK</div>
                        <div class="boot-line" id="b3">> INIT IMU MPU6050... CALIBRATING... DONE</div>
                        <div class="boot-line" id="b4">> SECURING GNSS LINK... TRACKING... OK</div>
                        <div class="boot-line" id="b5" style="color: #fff">> ARMING UGV ESP-TANK...</div>
                    </div>

                    <svg class="optics-bg" viewBox="0 0 1920 1080" preserveAspectRatio="xMidYMid slice">
                        <circle cx="960" cy="540" r="400" stroke="var(--primary)" stroke-width="1" fill="none" stroke-dasharray="2 10" style="transition: stroke 0.2s;" />
                        <circle cx="960" cy="540" r="600" stroke="var(--primary)" stroke-width="0.5" fill="none" style="transition: stroke 0.2s;" />
                        <line x1="0" y1="540" x2="1920" y2="540" stroke="var(--primary)" stroke-width="0.5" stroke-dasharray="5 5" style="transition: stroke 0.2s;" />
                        <line x1="960" y1="0" x2="960" y2="1080" stroke="var(--primary)" stroke-width="0.5" stroke-dasharray="5 5" style="transition: stroke 0.2s;" />
                    </svg>

                    <div class="hud-container" id="main-hud">
                        <div class="center-hud">
                            <svg class="svg-center" viewBox="0 0 600 600">
                                <defs>
                                    <linearGradient id="compass-fade-gradient" x1="0%" y1="0%" x2="100%" y2="0%">
                                        <stop offset="0%" stop-color="black" /><stop offset="10%" stop-color="black" />
                                        <stop offset="30%" stop-color="white" /><stop offset="70%" stop-color="white" />
                                        <stop offset="90%" stop-color="black" /><stop offset="100%" stop-color="black" />
                                    </linearGradient>
                                    <mask id="compass-fade" maskUnits="objectBoundingBox"><rect x="-150" y="-50" width="300" height="100" fill="url(#compass-fade-gradient)" /></mask>

                                    <linearGradient id="window-fade-gradient" x1="0" y1="0" x2="0" y2="600" gradientUnits="userSpaceOnUse">
                                        <stop offset="0%" stop-color="black" /><stop offset="22%" stop-color="black" />   
                                        <stop offset="35%" stop-color="white" /><stop offset="70%" stop-color="white" />   
                                        <stop offset="85%" stop-color="black" /><stop offset="100%" stop-color="black" />  
                                    </linearGradient>
                                    <mask id="window-fade" maskUnits="userSpaceOnUse" x="0" y="0" width="600" height="600"><rect x="0" y="0" width="600" height="600" fill="url(#window-fade-gradient)" /></mask>
                                </defs>

                                <text x="300" y="210" id="critical-warn-msg">CRITICAL TILT ANGLE</text>

                                <g class="compass-group" mask="url(#compass-fade)">
                                    <path d="M -150 0 L 150 0" class="stroke-main" />
                                    <polygon points="0,0 -8,-10 8,-10" fill="var(--danger)" />
                                    <g id="compass-tape"></g>
                                </g>

                                <g transform="translate(300, 100)">
                                    <text x="-25" y="0" class="center-text-readout" font-size="13" id="ret-pitch" text-anchor="end">P: 0°</text>
                                    <circle cx="0" cy="-4" r="1.5" fill="var(--primary-dim)" style="transition: fill 0.2s;" />
                                    <text x="25" y="0" class="center-text-readout" font-size="13" id="ret-roll" text-anchor="start">R: 0°</text>
                                </g>

                                <path d="M 150 400 A 141.42 141.42 0 0 1 150 200" class="stroke-dim" stroke-width="6" />
                                <path d="M 150 400 A 141.42 141.42 0 0 1 150 200" class="stroke-main" stroke-width="6" id="arc-speed" stroke-dasharray="0 450" />
                                <text x="90" y="296" class="center-text-readout" id="center-spd" text-anchor="end">0.0</text>
                                <text x="90" y="312" class="center-text-label" text-anchor="end">KM/H</text>

                                <path d="M 450 400 A 141.42 141.42 0 0 0 450 200" class="stroke-dim" stroke-width="6" />
                                <path d="M 450 400 A 141.42 141.42 0 0 0 450 200" stroke="var(--warning)" fill="none" stroke-width="6" id="arc-batt" stroke-dasharray="222 450" />
                                <text x="510" y="296" class="center-text-readout" style="fill: var(--warning)" id="center-bat" text-anchor="start">PWR</text>
                                <text x="510" y="312" class="center-text-label" text-anchor="start">SYSTEM</text>

                                <g transform="translate(300, 300)">
                                    <circle cx="0" cy="0" r="3" fill="var(--danger)" />
                                    <path d="M -20 0 L -8 0 M 8 0 L 20 0 M 0 -20 L 0 -8" class="stroke-main" stroke-width="2" />
                                    <path d="M -50 0 L -30 0 L -30 10 M 50 0 L 30 0 L 30 10" class="stroke-dim" />
                                </g>

                                <g mask="url(#window-fade)">
                                    <g id="horizon-group" transform="translate(300, 300)">
                                        <g id="pitch-ladder"></g>
                                    </g>
                                </g>
                            </svg>
                        </div>

                        <div class="side-panel panel-top-left">
                            <div class="panel-header">SYS-OPS</div>
                            <div class="data-row"><span class="data-label">UNIT</span><span class="data-value data-value-small">UGV ESP-TANK</span></div>
                            <div class="data-row"><span class="data-label">MODE</span><span class="data-value data-value-small" style="color: var(--warning);">MANUAL FPV</span></div>
                            <div class="data-row" style="margin-top: 4px;"><span class="data-label">STATE</span><span class="status-badge" id="hud-sys-state">DISARMED</span></div>
                            <div class="data-row" style="margin-top: 8px;"><span class="data-label">UPTIME</span><span class="data-value data-value-small" id="hud-sys-uptime" style="color: var(--primary-dim);">00:00:00</span></div>
                        </div>

                        <div class="side-panel panel-top-right">
                            <div class="panel-header">SAT-NAV</div>
                            <div class="data-row"><span class="data-label">LAT</span><span class="data-value data-value-small" id="hud-gps-lat">--</span></div>
                            <div class="data-row"><span class="data-label">LNG</span><span class="data-value data-value-small" id="hud-gps-lng">--</span></div>
                            <div class="data-row"><span class="data-label">ALT</span><span class="data-value data-value-small" id="hud-gps-alt">--</span></div>
                            <div class="accel-row">
                                <div class="accel-item"><span class="data-label">SATS</span><span class="data-value" id="hud-gps-sats" style="color: var(--primary);">0</span></div>
                                <div class="accel-item"><span class="data-label">HDOP</span><span class="data-value" id="hud-gps-hdop">--</span></div>
                                <div class="accel-item"><span class="data-label">CRS</span><span class="data-value" id="hud-gps-crs">--</span></div>
                                <div class="accel-item"><span class="data-label">SPD</span><span class="data-value" id="hud-gps-spd">--</span></div>
                            </div>
                            <div style="text-align: right; color: var(--primary-dim); font-size: 0.55rem; font-weight: bold; transition: color 0.2s;">TIME: <span id="hud-sys-time-local">--</span></div>
                        </div>

                        <div class="side-panel panel-left">
                            <div class="panel-header">DRIVE (TRACKS)</div>
                            <div class="motor-grid" style="grid-template-columns: 1fr;">
                                <div><div class="data-row"><span class="data-label">LEFT</span><span class="data-value" id="hud-mot-l">0%</span></div><div class="bar-bg"><div class="bar-fill" id="hud-bar-mot-l"></div></div></div>
                                <div><div class="data-row"><span class="data-label">RIGHT</span><span class="data-value" id="hud-mot-r">0%</span></div><div class="bar-bg"><div class="bar-fill" id="hud-bar-mot-r"></div></div></div>
                            </div>
                            <div class="push-bottom">
                                <div class="data-row"><span class="data-label">TEMP</span><span class="data-value" id="hud-imu-temp">--°C</span></div>
                                <div class="data-row"><span class="data-label">LINK</span><span class="data-value" id="hud-sys-link" style="color: var(--primary-dim)">OK</span></div>
                            </div>
                        </div>

                        <div class="side-panel panel-right">
                            <div class="panel-header">IMU SENSE</div>
                            <div class="accel-row">
                                <div class="accel-item"><span class="data-label">AX</span><span class="data-value" id="hud-acc-x">0.0</span></div>
                                <div class="accel-item"><span class="data-label">AY</span><span class="data-value" id="hud-acc-y">0.0</span></div>
                                <div class="accel-item"><span class="data-label">AZ</span><span class="data-value" id="hud-acc-z">1.0</span></div>
                            </div>
                            <div style="margin-top: 4px;">
                                <div class="data-row"><span class="data-label">SLOPE</span><span class="data-value" id="hud-slope-val">0°</span></div>
                                <div class="bar-bg"><div class="bar-fill" id="hud-bar-slope" style="width: 0%;"></div></div>
                            </div>
                            <div class="push-bottom">
                                <div class="data-row"><span class="data-label">STATUS</span><span class="data-value" id="hud-imu-stat" style="color: var(--primary-dim)">NOMINAL</span></div>
                            </div>
                        </div>
                    </div>
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
                    
                    <!-- System Logs Console -->
                    <div class="card" style="grid-column: 1 / -1;">
                        <div style="display:flex; justify-content: space-between; align-items: center; border-bottom: 1px solid var(--header-border); padding-bottom: 12px; margin-bottom: 16px;">
                            <h3 style="border: none; padding: 0; margin: 0;">System Logs</h3>
                            <div class="setting-item" style="margin:0; min-width:180px;">
                                <label for="auto-refresh-logs" style="font-size: 0.8rem; margin-right: 10px;">Auto Refresh Logs</label>
                                <label class="switch">
                                    <input type="checkbox" id="auto-refresh-logs" checked>
                                    <span class="slider round"></span>
                                </label>
                            </div>
                        </div>
                        <div id="log-console" 
                             style="width: 100%; height: 300px; background: #1e1e1e; color: #d4d4d4; font-family: 'Consolas', 'Monaco', 'Courier New', monospace; font-size: 0.85rem; border-radius: 6px; padding: 12px; border: 1px solid var(--header-border); overflow-y: scroll; line-height: 1.5; white-space: pre-wrap; display: flex; flex-direction: column;"></div>
                        <button onclick="clearSystemLogs()" class="btn-primary" style="margin-top: 10px; width: 100%; background: transparent; border: 1px solid var(--accent-color); color: var(--accent-color);">CLEAR CONSOLE</button>
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
.armed-true .switch input:checked + .slider {
    background-color: rgba(228, 37, 72, 0.2);
    border-color: var(--watermelon);
}
.armed-true .switch input:checked + .slider:before {
    background-color: var(--watermelon);
    box-shadow: 0 0 8px var(--watermelon);
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

/* HUD Tab Specific Styles */
#hud-tab {
    padding: 0;
    margin: 0;
    background-color: #050807;
    position: relative;
    overflow: hidden;
    height: 85vh;
    min-height: 600px;
    border-radius: 12px;
    box-shadow: inset 0 0 100px rgba(0,0,0,0.8);
}

.hud-viewport {
    --primary: var(--accent-color);
    --primary-dim: rgba(var(--accent-rgb), 0.25); /* We'll need to define this or use a workaround */
    --danger: var(--watermelon);
    --warning: #ffb300;
    --text-main: var(--text-main);
    --text-dim: rgba(224, 248, 245, 0.65);
    --font-mono: 'Share Tech Mono', 'Courier New', monospace;
    --glow: 0 0 4px var(--primary);
    --fpv-aberration: 0.6px 0px 0px rgba(255,0,0,0.3), -0.6px 0px 0px rgba(0,255,255,0.3), 0 0 2px rgba(0,0,0,0.8);

    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    overflow: hidden;
    color: var(--text-main);
    font-family: var(--font-mono);
}

/* Fallback for rgb if not defined */
[data-theme="dark"] .hud-viewport {
    --primary-dim: rgba(235, 251, 55, 0.25);
}
:root:not([data-theme="dark"]) .hud-viewport {
    --primary-dim: rgba(206, 59, 155, 0.25);
}

.hud-viewport.critical-mode {
    --primary: var(--watermelon);
    --primary-dim: rgba(228, 37, 72, 0.3);
    --glow: 0 0 15px var(--watermelon);
}

.lens-fx {
    position: absolute; inset: 0; pointer-events: none; z-index: 998;
    box-shadow: inset 0 0 150px rgba(0,0,0,1);
    background: radial-gradient(circle at center, transparent 60%, rgba(0,0,0,0.4) 100%);
}
.screen-fx { 
    position: absolute; inset: 0; pointer-events: none; z-index: 999; 
    background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.15) 50%); background-size: 100% 4px; 
}
.optics-bg { position: absolute; inset: 0; width: 100%; height: 100%; pointer-events: none; z-index: 1; opacity: 0.12; transition: opacity 1s; }

#boot-screen {
    position: absolute; inset: 0; z-index: 9999; background: #050807;
    display: flex; flex-direction: column; justify-content: flex-end; padding: 40px;
    font-size: 18px; color: var(--primary); text-shadow: var(--glow);
    transition: opacity 0.3s ease-out;
}
.boot-line { margin: 5px 0; opacity: 0; }

.hud-container { 
    position: absolute; inset: 0; width: 100%; height: 100%; z-index: 10; 
    opacity: 0; transform: scale(0.95); transition: opacity 0.5s, transform 0.5s cubic-bezier(0.1, 1, 0.3, 1);
}
.hud-container.active { opacity: 1; transform: scale(1); }

.center-hud { position: absolute; inset: 0; display: flex; justify-content: center; align-items: center; pointer-events: none; z-index: 5; }
    .svg-center { 
        width: clamp(300px, 85vmin, 850px); 
        height: clamp(300px, 85vmin, 850px); 
        aspect-ratio: 1;
        overflow: visible; 
        filter: drop-shadow(var(--glow)); 
        transition: filter 0.2s;
    }

.stroke-main { stroke: var(--primary); fill: none; stroke-width: 1.5; transition: stroke 0.2s; }
.stroke-dim { stroke: var(--primary-dim); fill: none; stroke-width: 1; transition: stroke 0.2s; }
.stroke-danger { stroke: var(--danger); fill: none; stroke-width: 2; }

.center-text-readout { font-family: var(--font-mono); font-size: 14px; fill: var(--primary); font-weight: bold; text-shadow: 0 0 5px rgba(0,0,0,0.8); transition: fill 0.2s; }
.center-text-label { font-family: var(--font-mono); font-size: 8px; fill: var(--text-dim); }
.compass-group { transform: translate(300px, 80px); }

#critical-warn-msg {
    font-size: 20px; letter-spacing: 4px; fill: var(--danger); text-anchor: middle;
    opacity: 0; transition: opacity 0.2s;
}
.critical-mode #critical-warn-msg {
    animation: blink-fast 0.3s infinite alternate;
}
@keyframes blink-fast { 0% { opacity: 1; } 100% { opacity: 0.2; } }

.side-panel {
    position: absolute; width: clamp(130px, 15vw, 190px); 
    opacity: 0.85; display: flex; flex-direction: column; gap: clamp(4px, 0.6vw, 6px); z-index: 20; 
    text-shadow: var(--fpv-aberration);
}
.panel-left, .panel-right { bottom: clamp(10px, 4vh, 30px); }
.panel-left { left: clamp(15px, 3vw, 40px); }
.panel-right { right: clamp(15px, 3vw, 40px); }
.panel-top-left { top: clamp(15px, 3vh, 30px); left: clamp(15px, 3vw, 40px); }
.panel-top-right { top: clamp(15px, 3vh, 30px); right: clamp(15px, 3vw, 40px); }

.panel-header { font-size: clamp(0.55rem, 0.65vw, 0.65rem); letter-spacing: 3px; color: var(--primary); border-bottom: 1px solid var(--primary-dim); padding-bottom: 2px; margin-bottom: 2px; text-transform: uppercase; transition: color 0.2s, border-color 0.2s; }
.data-row { display: flex; justify-content: space-between; align-items: flex-end; line-height: 1; margin-bottom: 2px; }
.data-label { color: var(--text-dim); font-size: clamp(0.5rem, 0.6vw, 0.65rem); }
.data-value { font-weight: bold; color: var(--text-main); font-size: clamp(0.7rem, 0.85vw, 0.85rem); transition: color 0.2s; }
.data-value-small { font-weight: normal; font-size: clamp(0.65rem, 0.7vw, 0.75rem); color: var(--text-main); }

.bar-bg { width: 100%; height: 2px; background: rgba(255,255,255,0.1); margin-top: 2px; }
.bar-fill { height: 100%; background: var(--primary); box-shadow: var(--glow); transition: width 0.1s linear, background 0.2s; }

.motor-grid { display: grid; gap: 4px; }
.accel-row { display: flex; justify-content: space-between; padding: 2px 0; border-top: 1px solid rgba(255,255,255,0.1); border-bottom: 1px solid rgba(255,255,255,0.1); margin-top: 4px; margin-bottom: 4px; }
.accel-item { display: flex; flex-direction: column; align-items: center; }
.accel-item .data-label { font-size: 0.45rem; margin-bottom: 2px; color: var(--primary-dim); transition: color 0.2s; }
.accel-item .data-value { font-size: 0.65rem; text-shadow: var(--fpv-aberration); }

.status-badge {
    display: inline-flex; align-items: center; line-height: 1;
    color: var(--danger); border: 1px solid var(--danger); background: rgba(0,0,0,0.5);
    padding: 2px 5px; font-size: clamp(0.55rem, 0.65vw, 0.7rem); font-weight: bold; text-shadow: none; animation: pulse-danger 1.5s infinite;
}
@keyframes pulse-danger { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }
.push-bottom { margin-top: auto; }

/* Responsivo para Tablet (intermediate screens) */
@media (max-width: 960px) and (min-height: 600px) {
    .hud-viewport {
        display: flex;
        flex-direction: column;
        justify-content: space-between;
        align-items: center;
    }
    
    .svg-center {
        width: clamp(280px, 50vh, 550px) !important;
        height: clamp(280px, 50vh, 550px) !important;
    }
    
    .side-panel {
        width: clamp(110px, 12vw, 150px);
        gap: 3px;
    }
    
    .panel-header {
        font-size: 0.54rem;
        margin-bottom: 1px;
    }
    
    .data-row {
        margin-bottom: 1px;
    }
    
    .data-label {
        font-size: 0.48rem;
    }
    
    .data-value {
        font-size: 0.62rem;
    }
}

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

    /* HUD Responsiveness - Mobile specific */
    #hud-tab { 
        min-height: 550px; 
        height: 70vh; 
        margin-left: -12px; 
        margin-right: -12px; 
        border-radius: 0;
    }
    
    .hud-viewport {
        display: flex;
        flex-direction: column;
        justify-content: space-between;
        align-items: center;
        padding: 8px 0;
    }
    
    .center-hud {
        position: relative;
        width: 100%;
        height: 100%;
        flex: 1;
        display: flex;
        justify-content: center;
        align-items: center;
    }
    
    .side-panel { 
        width: clamp(95px, 40vw, 155px); 
        gap: 3px;
        height: auto;
        max-height: none;
    }
    
    .panel-header { 
        font-size: 0.52rem; 
        letter-spacing: 1.5px; 
        margin-bottom: 1px; 
    }
    
    .data-label { 
        font-size: 0.48rem; 
    }
    
    .data-value { 
        font-size: 0.6rem; 
    }
    
    .data-value-small { 
        font-size: 0.55rem; 
    }
    
    .data-row {
        margin-bottom: 1px;
        line-height: 1.1;
    }
    
    /* Layout positioning for mobile: layers architecture */
    .panel-top-left { 
        top: 10px; 
        left: 10px; 
        position: absolute;
    }
    
    .panel-top-right { 
        top: 10px; 
        right: 10px; 
        position: absolute;
    }
    
    .panel-left { 
        bottom: 10px; 
        left: 10px; 
        position: absolute;
    }
    
    .panel-right { 
        bottom: 10px; 
        right: 10px; 
        position: absolute;
    }
    
    .svg-center { 
        width: clamp(280px, 78vmin, 500px) !important;
        height: clamp(280px, 78vmin, 500px) !important;
        max-width: none;
        max-height: none;
    }
    
    #critical-warn-msg { 
        font-size: 18px; 
    }
    
    .accel-row {
        padding: 1px 0;
        margin-top: 2px;
        margin-bottom: 2px;
    }
    
    .accel-item .data-label {
        font-size: 0.4rem;
        margin-bottom: 1px;
    }
    
    .accel-item .data-value {
        font-size: 0.58rem;
    }
    
    .bar-bg {
        height: 1px;
        margin-top: 1px;
    }
    
    .push-bottom {
        margin-top: 3px;
    }
}

/* Extra small devices / very narrow viewports */
@media (max-height: 550px) {
    #hud-tab {
        min-height: 500px;
        height: 90vh;
    }
    
    .svg-center {
        width: clamp(200px, 50vmin, 350px) !important;
        height: clamp(200px, 50vmin, 350px) !important;
    }
    
    .side-panel {
        width: clamp(80px, 35vw, 130px);
        gap: 2px;
    }
    
    .panel-header {
        font-size: 0.5rem;
        margin-bottom: 1px;
    }
    
    .data-label {
        font-size: 0.45rem;
    }
    
    .data-value {
        font-size: 0.55rem;
    }
    
    .data-row {
        margin-bottom: 0.5px;
    }
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
        if (!d.raw_channels || !Array.isArray(d.raw_channels)) {
            // Backward compatibility if backend isn't updated yet
            if (d.throttle !== undefined && d.steering !== undefined && d.aux) {
                // Usually CH1 is Steering, CH2 ?, CH3 Throttle in generic cars. We will just use standard order 1 to 10.
                // It's better to fetch raw channels from backend. 
                // As the backend will be modified to send `raw_channels`, we expect it here.
            }
            return;
        }

        const container = document.getElementById('all-channels');
        if(container.innerHTML === 'Loading...' || container.innerHTML === '') {
            container.innerHTML = '';
            // Divide channels in 2 columns
            container.innerHTML = '<div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px;">' +
                d.raw_channels.map((val, i) => `
                    <div class="channel-group" style="margin-bottom: 0;">
                        <label>CH ${i+1}</label>
                        <div class="channel-row">
                            <div class="progress-bar"><div id="ch-${i}" class="fill" style="width: 50%"></div></div>
                            <span class="val" id="val-${i}">${val}</span>
                        </div>
                    </div>
                `).join('') + '</div>';
        }
        
        d.raw_channels.forEach((val, i) => {
            updateChannel(`${i}`, val);
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

function updateLogs() {
    const logConsole = document.getElementById('log-console');
    if (!logConsole || !document.getElementById('config').classList.contains('active')) return;

    const autoRefresh = document.getElementById('auto-refresh-logs');
    if (autoRefresh && !autoRefresh.checked) return;

    fetch('/api/logs')
        .then(r => r.text())
        .then(text => {
            if (text.trim() === '') return;
            
            // Processa as linhas para adicionar cores
            const lines = text.trim().split('\n');
            let html = '';
            
            lines.forEach(line => {
                let color = '#d4d4d4'; // Default (White/Grey)
                let weight = 'normal';
                
                if (line.includes('[ERROR]')) {
                    color = '#f44336'; // Red
                    weight = 'bold';
                } else if (line.includes('[WARN]')) {
                    color = '#ffc107'; // Yellow
                } else if (line.includes('[INFO]')) {
                    color = '#ffffff'; // Pure White
                } else if (line.includes('[DEBUG]')) {
                    color = '#2196f3'; // Blue
                }
                
                html += `<div style="color: ${color}; font-weight: ${weight}; margin-bottom: 2px; border-bottom: 1px solid rgba(255,255,255,0.03);">${line}</div>`;
            });
            
            if (logConsole.innerHTML !== html) {
                logConsole.innerHTML = html;
                logConsole.scrollTop = logConsole.scrollHeight;
            }
        })
        .catch(console.error);
}

function clearSystemLogs() {
    fetch('/api/clear-logs', { method: 'POST' })
        .then(() => {
            document.getElementById('log-console').innerHTML = '<div style="color: #6a9955">Console limpo.</div>';
        })
        .catch(console.error);
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
        } else {
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
        }

        // Compass Data
        if (d.compass) {
            if (d.compass.valid) {
                document.getElementById('comp-heading').innerText = d.compass.heading.toFixed(1);
                document.getElementById('comp-x').innerText = d.compass.x.toFixed(2);
                document.getElementById('comp-y').innerText = d.compass.y.toFixed(2);
                document.getElementById('comp-z').innerText = d.compass.z.toFixed(2);
            } else {
                document.getElementById('comp-heading').innerText = "--";
                document.getElementById('comp-x').innerText = "--";
                document.getElementById('comp-y').innerText = "--";
                document.getElementById('comp-z').innerText = "--";
            }
        }

        // GPS Data Processing
        if (d.gps) {
            const gpsStatus = document.getElementById('gps-status');
            if (d.gps.valid) {
                gpsStatus.innerText = "3D FIX";
                gpsStatus.style.background = "var(--accent-bg-glow)";
                gpsStatus.style.color = "var(--accent-color)";
                
                document.getElementById('gps-lat').innerText = d.gps.lat.toFixed(6);
                document.getElementById('gps-lng').innerText = d.gps.lng.toFixed(6);
                document.getElementById('gps-alt').innerText = d.gps.alt.toFixed(1);
                document.getElementById('gps-sats').innerText = d.gps.satellites;
                document.getElementById('gps-speed').innerText = d.gps.speed.toFixed(1);
                document.getElementById('gps-course').innerText = d.gps.course.toFixed(1);
                document.getElementById('gps-hdop').innerText = d.gps.hdop.toFixed(2);
                
                // Format the ISO time slightly for better reading (split date and time)
                if(d.gps.time) {
                    const timeStr = d.gps.time.split('T');
                    if(timeStr.length === 2) {
                        const justTime = timeStr[1].split('-')[0]; // Remove timezone part for display if needed
                        document.getElementById('gps-time').innerText = justTime;
                    } else {
                        document.getElementById('gps-time').innerText = d.gps.time;
                    }
                }
            } else {
                gpsStatus.innerText = "NO FIX";
                gpsStatus.style.background = "rgba(228, 37, 72, 0.1)";
                gpsStatus.style.color = "var(--watermelon)";
                
                document.getElementById('gps-lat').innerText = "--";
                document.getElementById('gps-lng').innerText = "--";
                document.getElementById('gps-alt').innerText = "--";
                document.getElementById('gps-speed').innerText = "--";
                document.getElementById('gps-course').innerText = "--";
                document.getElementById('gps-hdop').innerText = "--";
                document.getElementById('gps-time').innerText = "--";
                
                // We might still get satellite count even without fix
                if (d.gps.satellites !== undefined) {
                    document.getElementById('gps-sats').innerText = d.gps.satellites;
                }
            }
        }
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
        if(document.getElementById('hud-tab').classList.contains('active')) {
            fetch('/api/sensors')
            .then(r => r.json())
            .then(d => updateHUD(d))
            .catch(console.error);
        }
        // Logs são atualizados mais rápido e independentemente se na aba config
    }, rate);

    // Loop de logs roda independente pra tempo real, a 500ms
    setInterval(() => {
        if(document.getElementById('config').classList.contains('active')) {
            updateLogs();
        }
    }, 500);
}

function openTab(tabName) {
    const contents = document.querySelectorAll('.tab-content');
    contents.forEach(content => content.classList.remove('active'));
    
    const btns = document.querySelectorAll('.tab-btn');
    btns.forEach(btn => btn.classList.remove('active'));
    
    document.getElementById(tabName).classList.add('active');
    event.currentTarget.classList.add('active');

    if (tabName === 'hud-tab') {
        initHUD();
    }
    
    startPolling();
}

// --- HUD Logic ---
let hudBootTime = 0;
let hudInitialized = false;

function initHUD() {
    if (hudInitialized) return;
    
    // Construct Pitch Ladder
    const pitchGroup = document.getElementById('pitch-ladder');
    if (pitchGroup) {
        pitchGroup.innerHTML = '';
        for (let i = -60; i <= 60; i += 10) {
            const y = i * 3;
            const w = (i === 0) ? 120 : ((i % 20 === 0) ? 70 : 40);
            const g = document.createElementNS("http://www.w3.org/2000/svg", "g");
            if (i === 0) {
                g.innerHTML = `<path d="M-${w},0 L-30,0 M30,0 L${w},0" class="stroke-main" stroke-width="2" />`;
            } else if (i > 0) {
                g.innerHTML = `
                    <path d="M-${w},-${y} L-20,-${y} M20,-${y} L${w},-${y}" class="stroke-main" />
                    <text x="-${w + 15}" y="-${y - 4}" class="center-text-readout" font-size="10" text-anchor="end">${i}</text>
                    <text x="${w + 15}" y="-${y - 4}" class="center-text-readout" font-size="10" text-anchor="start">${i}</text>
                `;
            } else {
                const absY = Math.abs(y);
                g.innerHTML = `
                    <path d="M-${w},${absY} L-20,${absY} L-20,${absY + 5} M20,${absY + 5} L20,${absY} L${w},${absY}" class="stroke-main" stroke-dasharray="6" />
                    <text x="-${w + 15}" y="${absY + 4}" class="center-text-readout" font-size="10" text-anchor="end">${Math.abs(i)}</text>
                    <text x="${w + 15}" y="${absY + 4}" class="center-text-readout" font-size="10" text-anchor="start">${Math.abs(i)}</text>
                `;
            }
            pitchGroup.appendChild(g);
        }
    }

    // Construct Compass Tape
    const compassTape = document.getElementById('compass-tape');
    if (compassTape) {
        compassTape.innerHTML = '';
        const headings = ['N', '030', '060', 'E', '120', '150', 'S', '210', '240', 'W', '300', '330'];
        const tapeContent = [...headings, ...headings, ...headings]; 
        tapeContent.forEach((h, index) => {
            const xPos = index * 40; 
            const mark = document.createElementNS("http://www.w3.org/2000/svg", "g");
            mark.innerHTML = `
                <line x1="${xPos}" y1="-5" x2="${xPos}" y2="-15" class="stroke-main" stroke-width="1" />
                <text x="${xPos}" y="-20" class="center-text-readout" font-size="10" text-anchor="middle">${h}</text>
            `;
            compassTape.appendChild(mark);
        });
    }

    // Boot Sequence
    const lines = ['b1', 'b2', 'b3', 'b4', 'b5'];
    let delay = 200;
    lines.forEach((id) => {
        const el = document.getElementById(id);
        if (el) setTimeout(() => { el.style.opacity = 1; }, delay);
        delay += 300;
    });
    setTimeout(() => {
        const boot = document.getElementById('boot-screen');
        const mainHud = document.getElementById('main-hud');
        if (boot) boot.style.opacity = 0;
        if (mainHud) mainHud.classList.add('active');
        setTimeout(() => { if (boot) boot.style.display = 'none'; }, 300);
    }, delay + 400);

    hudBootTime = Date.now();
    hudInitialized = true;
}

function updateHUD(data) {
    if (!hudInitialized) return;

    const pitch = data.angles.pitch;
    const roll = data.angles.roll;
    const yaw = data.angles.yaw;
    const speed = data.gps.valid ? data.gps.speed : 0;
    
    // Tip-Over Alert
    const maxSlope = Math.max(Math.abs(pitch), Math.abs(roll));
    const isCritical = maxSlope > 35;
    const viewport = document.querySelector('.hud-viewport');

    if (isCritical) {
        viewport.classList.add('critical-mode');
        document.getElementById('critical-warn-msg').style.opacity = 1;
        document.getElementById('hud-imu-stat').innerText = 'WARN: TILT';
    } else {
        viewport.classList.remove('critical-mode');
        document.getElementById('critical-warn-msg').style.opacity = 0;
        document.getElementById('hud-imu-stat').innerText = 'NOMINAL';
    }

    // Horizon
    const pitchY = pitch * 3; 
    document.getElementById('horizon-group').setAttribute('transform', `translate(300, 300) rotate(${-roll}) translate(0, ${pitchY})`);
    
    document.getElementById('ret-pitch').textContent = `P: ${(pitch>0?'+':'')}${pitch.toFixed(1)}°`;
    document.getElementById('ret-roll').textContent = `R: ${(roll>0?'+':'')}${roll.toFixed(1)}°`;
    const speedDash = Math.min((speed / 50) * 222, 222); 
    document.getElementById('arc-speed').setAttribute('stroke-dasharray', `${speedDash} 400`);
    document.getElementById('center-spd').innerText = speed.toFixed(1);

    // Compass
    const fullCircleWidth = 12 * 40; 
    const yawOffset = -fullCircleWidth - ((yaw / 360) * fullCircleWidth);
    document.getElementById('compass-tape').setAttribute('transform', `translate(${yawOffset}, 0)`);

    // Motors (Left / Right Tracks)
    const motL = Math.abs(data.motors.left * 100);
    const motR = Math.abs(data.motors.right * 100);
    
    document.getElementById('hud-bar-mot-l').style.width = `${motL}%`; 
    document.getElementById('hud-mot-l').innerText = `${motL.toFixed(0)}%`;
    document.getElementById('hud-bar-mot-r').style.width = `${motR}%`; 
    document.getElementById('hud-mot-r').innerText = `${motR.toFixed(0)}%`;

    // Accelerometer
    document.getElementById('hud-acc-x').innerText = data.accel.x.toFixed(2);
    document.getElementById('hud-acc-y').innerText = data.accel.y.toFixed(2);
    document.getElementById('hud-acc-z').innerText = data.accel.z.toFixed(2);
    
    // Slope Bar
    document.getElementById('hud-slope-val').innerText = `${maxSlope.toFixed(0)}°`;
    document.getElementById('hud-bar-slope').style.width = `${Math.min((maxSlope/45)*100, 100)}%`;

    // System Status & GPS
    document.getElementById('hud-sys-state').innerText = data.armed ? "ARMED" : "DISARMED";
    document.getElementById('hud-sys-state').style.color = data.armed ? "var(--danger)" : "var(--primary)";
    
    if (data.gps.valid) {
        document.getElementById('hud-gps-lat').innerText = data.gps.lat.toFixed(6) + "°";
        document.getElementById('hud-gps-lng').innerText = data.gps.lng.toFixed(6) + "°";
        document.getElementById('hud-gps-alt').innerText = data.gps.alt.toFixed(0) + " m";
        document.getElementById('hud-gps-sats').innerText = data.gps.satellites;
        document.getElementById('hud-gps-hdop').innerText = data.gps.hdop.toFixed(1);
        document.getElementById('hud-gps-crs').innerText = data.gps.course.toFixed(0) + "°";
        document.getElementById('hud-gps-spd').innerText = data.gps.speed.toFixed(1);
        
        if (data.gps.time) {
            const timeParts = data.gps.time.split('T');
            if (timeParts[1]) document.getElementById('hud-sys-time-local').innerText = timeParts[1].split('Z')[0] + " Z";
        }
    }

    // IMU Temp
    document.getElementById('hud-imu-temp').innerText = data.temperature.toFixed(1) + "°C";

    // Uptime
    const uptimeMs = Date.now() - hudBootTime;
    const hrs = String(Math.floor(uptimeMs / 3600000)).padStart(2, '0');
    const mins = String(Math.floor((uptimeMs % 3600000) / 60000)).padStart(2, '0');
    const secs = String(Math.floor((uptimeMs % 60000) / 1000)).padStart(2, '0');
    document.getElementById('hud-sys-uptime').innerText = `${hrs}:${mins}:${secs}`;
}

// Initial load
document.addEventListener('DOMContentLoaded', () => {
    updateSysInfo();
    loadSettings();
    startPolling();
});
)rawliteral";

#endif
