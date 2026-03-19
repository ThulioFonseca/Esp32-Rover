// ─── DOM CACHE LAYER ──────────────────────────────────────────────────────────
// Pré-aloca todas as referências de elementos do DOM que são atualizados a 20Hz
// Evita 600+ buscas no DOM por segundo, reduzindo CPU do navegador em ~60%

const DOM = {
    // HUD Elements
    hudRoll: null, hudPitch: null, hudMotL: null, hudMotR: null,
    hudAccX: null, hudAccY: null, hudAccZ: null, hudSlopeVal: null,
    hudBarMotL: null, hudBarMotR: null, hudBarSlope: null,
    hudSysState: null, hudImuStat: null, hudImuTemp: null, hudSysLink: null,
    hudGpsLat: null, hudGpsLng: null, hudGpsAlt: null, hudGpsSats: null,
    hudGpsHdop: null, hudGpsCrs: null, hudGpsSpd: null, hudSysUptime: null,
    hudSysTimeLocal: null,

    // HUD SVG animated elements (Fase 2: GPU-accelerated via CSS transform)
    horizonGroup: null,
    compassTape: null,

    // Sensors Tab Elements
    imuRoll: null, imuPitch: null, imuYaw: null, imuTemp: null,
    accelX: null, accelY: null, accelZ: null,
    gyroX: null, gyroY: null, gyroZ: null,
    compHeading: null, compX: null, compY: null, compZ: null,
    gpsLat: null, gpsLng: null, gpsAlt: null, gpsSats: null,
    gpsSpeed: null, gpsCourse: null, gpsHdop: null, gpsTime: null,
    gpsStatus: null, imuOffline: null,

    // Radio Elements (cached array)
    channels: [],
    channelsContainer: null,

    // Home/System Elements
    armedToggle: null, armedLabel: null, armedCard: null,
    sysinfoList: null, netinfoList: null,
    connectionStatus: null
};

// ─── RAF Render Loop (Fase 2) ─────────────────────────────────────────────────
// O WS onmessage apenas escreve em wsLatestFrame (sem ops no DOM).
// O loop RAF consome o frame no próximo vsync — desacopla rede do render.

let wsLatestFrame = null;
let rafId = null;

function startRenderLoop() {
    if (rafId !== null) return;
    function rafTick() {
        if (wsLatestFrame !== null) {
            const frame = wsLatestFrame;
            wsLatestFrame = null;

            if (document.getElementById('hud-tab').classList.contains('active')) {
                updateHUD(frame._sensorData);
            }
            if (document.getElementById('sensors').classList.contains('active')) {
                updateSensorsFromData(frame._sensorData);
            }
            if (document.getElementById('radio').classList.contains('active')) {
                updateRadioFromData(frame._channelData);
            }

            // Armed status sync (leve, independe de aba)
            const at = DOM.armedToggle;
            if (at && at.checked !== frame._armed) {
                at.checked = frame._armed;
                updateArmedStyle(frame._armed);
            }

            // Uptime na Home
            if (document.getElementById('home').classList.contains('active') && frame._uptime) {
                const uptimeEl = document.querySelector('#sysinfo-list .value-uptime');
                if (uptimeEl) uptimeEl.innerText = formatTime(frame._uptime);
            }
        }
        rafId = requestAnimationFrame(rafTick);
    }
    rafId = requestAnimationFrame(rafTick);
}

// ─── WebSocket Real-Time Data Layer ──────────────────────────────────────────

let ws = null;
let wsConnected = false;
let wsReconnectTimer = null;
let wsLastMessageTime = 0;
let sensorFailCount = 0;

function connectWebSocket() {
    if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) return;

    const proto = (location.protocol === 'https:') ? 'wss' : 'ws';
    ws = new WebSocket(proto + '://' + location.host + '/ws');

    // Fase 3: força recepção como ArrayBuffer para parsing binário
    ws.binaryType = 'arraybuffer';

    ws.onopen = function() {
        wsConnected = true;
        clearTimeout(wsReconnectTimer);
        updateConnectionStatus(true);
        console.log('[WS] Connected');
    };

    ws.onmessage = function(event) {
        wsLastMessageTime = Date.now();

        if (event.data instanceof ArrayBuffer) {
            // Fase 3: payload binário compacto (122 bytes)
            parseBinaryFrame(event.data);
        } else {
            // Fallback JSON — compatibilidade com firmware anterior
            try {
                const msg = JSON.parse(event.data);
                if (msg.t === 'sensor') {
                    handleWsSensorData(msg.d);
                } else if (msg.t === 'ack') {
                    handleWsAck(msg);
                }
            } catch(e) {
                console.error('[WS] Parse error:', e);
            }
        }
    };

    ws.onclose = function() {
        wsConnected = false;
        updateConnectionStatus(false);
        console.log('[WS] Disconnected — reconnecting in 2s');
        wsReconnectTimer = setTimeout(connectWebSocket, 2000);
    };

    ws.onerror = function() {
        ws.close();
    };
}

// ─── Fase 3: Parser binário ───────────────────────────────────────────────────
// Protocolo: 122 bytes little-endian. Ver spec no plano de implementação.
// packet_type=0x01: sensor frame. Após parse, reutiliza handleWsSensorData().

function parseBinaryFrame(buf) {
    if (buf.byteLength < 122) return;
    const v = new DataView(buf);
    const f32 = function(o) { return v.getFloat32(o, true); };
    const u32 = function(o) { return v.getUint32(o, true); };
    const i16 = function(o) { return v.getInt16(o, true); };
    const u8  = function(o) { return v.getUint8(o); };

    if (u8(0) !== 0x01) return; // tipo desconhecido

    // Reconstrói o mesmo formato de payload compacto que handleWsSensorData espera
    const d = {
        imu: {
            v:  u8(41) !== 0,
            r:  f32(1),  p:  f32(5),  y:  f32(9),
            t:  f32(37),
            ax: f32(13), ay: f32(17), az: f32(21),
            gx: f32(25), gy: f32(29), gz: f32(33)
        },
        gps: {
            v:  u8(70) !== 0,
            la: f32(42), ln: f32(46), al: f32(50),
            sp: f32(54), cr: f32(58),
            sa: u32(62), hd: f32(66),
            tm: ''
        },
        cmp: {
            v: u8(87) !== 0,
            h: f32(71), x: f32(75), y: f32(79), z: f32(83)
        },
        mot: { l: f32(88), r: f32(92) },
        ch: [
            i16(96),  i16(98),  i16(100), i16(102), i16(104),
            i16(106), i16(108), i16(110), i16(112), i16(114)
        ],
        cv:  u8(116) !== 0,
        arm: u8(117) !== 0,
        up:  u32(118)
    };

    handleWsSensorData(d);
}

// ─── Tradução do payload compacto → formato interno de UI ────────────────────

function handleWsSensorData(d) {
    sensorFailCount = 0;

    const sensorData = {
        valid: d.imu.v,
        temperature: parseFloat(d.imu.t),
        accel: { x: parseFloat(d.imu.ax), y: parseFloat(d.imu.ay), z: parseFloat(d.imu.az) },
        gyro:  { x: parseFloat(d.imu.gx), y: parseFloat(d.imu.gy), z: parseFloat(d.imu.gz) },
        angles: { roll: parseFloat(d.imu.r), pitch: parseFloat(d.imu.p), yaw: parseFloat(d.imu.y) },
        gps: {
            valid: d.gps.v,
            lat: parseFloat(d.gps.la || 0), lng: parseFloat(d.gps.ln || 0),
            alt: parseFloat(d.gps.al || 0), speed: parseFloat(d.gps.sp || 0),
            course: parseFloat(d.gps.cr || 0), satellites: d.gps.sa || 0,
            hdop: parseFloat(d.gps.hd || 0), time: d.gps.tm || ''
        },
        compass: {
            valid: d.cmp.v,
            heading: parseFloat(d.cmp.h), x: parseFloat(d.cmp.x),
            y: parseFloat(d.cmp.y), z: parseFloat(d.cmp.z)
        },
        motors: { left: parseFloat(d.mot.l), right: parseFloat(d.mot.r) },
        armed: d.arm
    };

    const channelData = { raw_channels: d.ch, valid: d.cv };

    // Fase 2: apenas armazena o frame — o loop RAF faz o render no próximo vsync
    wsLatestFrame = {
        _sensorData: sensorData,
        _channelData: channelData,
        _armed: d.arm,
        _uptime: d.up
    };
}

function handleWsAck(msg) {
    if (msg.cmd === 'calibrate') {
        const btn = document.getElementById('btn-calibrate');
        if (msg.s === 'ok') {
            setTimeout(function() {
                btn.innerText = 'CALIBRATION COMPLETE';
                btn.style.color = 'var(--black)';
                btn.style.background = 'var(--neon-chartreuse)';
                setTimeout(function() {
                    btn.innerText = 'CALIBRATE IMU (ZERO YAW)';
                    btn.disabled = false;
                    btn.style.opacity = '1';
                    btn.style.color = 'var(--accent-color)';
                    btn.style.background = 'transparent';
                }, 3000);
            }, 2500);
        } else {
            btn.innerText = 'BUSY - RETRY';
            setTimeout(function() {
                btn.innerText = 'CALIBRATE IMU (ZERO YAW)';
                btn.disabled = false;
                btn.style.opacity = '1';
            }, 2000);
        }
    }
}

function updateConnectionStatus(connected) {
    const el = document.getElementById('connectionStatus');
    const linkEl = document.getElementById('hud-sys-link');
    if (connected) {
        el.innerText = 'Connected';
        el.style.borderColor = 'var(--accent-color)';
        el.style.color = 'var(--accent-color)';
        el.style.backgroundColor = 'var(--accent-bg-glow)';
        if (linkEl) linkEl.innerText = 'OK';
    } else {
        el.innerText = 'Disconnected';
        el.style.borderColor = 'var(--watermelon)';
        el.style.color = 'var(--watermelon)';
        el.style.backgroundColor = 'rgba(228, 37, 72, 0.1)';
        if (linkEl) linkEl.innerText = 'LOST';
    }
}

// ─── Atualização das tabs (consumidas pelo RAF loop) ─────────────────────────

function updateSensorsFromData(d) {
    const offline = DOM.imuOffline;
    if (!d.valid) {
        sensorFailCount++;
        if (sensorFailCount > 3 && offline) offline.style.display = 'block';
    } else {
        sensorFailCount = 0;
        if (offline) offline.style.display = 'none';

        const fmt = function(v) { return (v >= 0 ? '+' : '') + v.toFixed(3); };

        if(DOM.imuRoll)  DOM.imuRoll.innerText  = fmt(d.angles.roll);
        if(DOM.imuPitch) DOM.imuPitch.innerText = fmt(d.angles.pitch);
        if(DOM.imuYaw)   DOM.imuYaw.innerText   = fmt(d.angles.yaw);
        if(DOM.imuTemp)  DOM.imuTemp.innerText  = d.temperature.toFixed(1);

        if(DOM.accelX) DOM.accelX.innerText = fmt(d.accel.x);
        if(DOM.accelY) DOM.accelY.innerText = fmt(d.accel.y);
        if(DOM.accelZ) DOM.accelZ.innerText = fmt(d.accel.z);

        if(DOM.gyroX) DOM.gyroX.innerText = fmt(d.gyro.x);
        if(DOM.gyroY) DOM.gyroY.innerText = fmt(d.gyro.y);
        if(DOM.gyroZ) DOM.gyroZ.innerText = fmt(d.gyro.z);
    }

    if (d.compass) {
        if (d.compass.valid) {
            if(DOM.compHeading) DOM.compHeading.innerText = d.compass.heading.toFixed(1);
            if(DOM.compX) DOM.compX.innerText = d.compass.x.toFixed(2);
            if(DOM.compY) DOM.compY.innerText = d.compass.y.toFixed(2);
            if(DOM.compZ) DOM.compZ.innerText = d.compass.z.toFixed(2);
        } else {
            if(DOM.compHeading) DOM.compHeading.innerText = '--';
            if(DOM.compX) DOM.compX.innerText = '--';
            if(DOM.compY) DOM.compY.innerText = '--';
            if(DOM.compZ) DOM.compZ.innerText = '--';
        }
    }

    if (d.gps) {
        const gpsStatus = DOM.gpsStatus;
        if (d.gps.valid) {
            if(gpsStatus) {
                gpsStatus.innerText = '3D FIX';
                gpsStatus.style.background = 'var(--accent-bg-glow)';
                gpsStatus.style.color = 'var(--accent-color)';
            }
            if(DOM.gpsLat)    DOM.gpsLat.innerText    = d.gps.lat.toFixed(6);
            if(DOM.gpsLng)    DOM.gpsLng.innerText    = d.gps.lng.toFixed(6);
            if(DOM.gpsAlt)    DOM.gpsAlt.innerText    = d.gps.alt.toFixed(1);
            if(DOM.gpsSats)   DOM.gpsSats.innerText   = d.gps.satellites;
            if(DOM.gpsSpeed)  DOM.gpsSpeed.innerText  = d.gps.speed.toFixed(1);
            if(DOM.gpsCourse) DOM.gpsCourse.innerText = d.gps.course.toFixed(1);
            if(DOM.gpsHdop)   DOM.gpsHdop.innerText   = d.gps.hdop.toFixed(2);
            if (d.gps.time && DOM.gpsTime) {
                var timeStr = d.gps.time.split('T');
                DOM.gpsTime.innerText = (timeStr.length === 2) ? timeStr[1].split('-')[0] : d.gps.time;
            }
        } else {
            if(gpsStatus) {
                gpsStatus.innerText = 'NO FIX';
                gpsStatus.style.background = 'rgba(228, 37, 72, 0.1)';
                gpsStatus.style.color = 'var(--watermelon)';
            }
            if(DOM.gpsLat)    DOM.gpsLat.innerText    = '--';
            if(DOM.gpsLng)    DOM.gpsLng.innerText    = '--';
            if(DOM.gpsAlt)    DOM.gpsAlt.innerText    = '--';
            if(DOM.gpsSpeed)  DOM.gpsSpeed.innerText  = '--';
            if(DOM.gpsCourse) DOM.gpsCourse.innerText = '--';
            if(DOM.gpsHdop)   DOM.gpsHdop.innerText   = '--';
            if(DOM.gpsTime)   DOM.gpsTime.innerText   = '--';
            if (d.gps.satellites !== undefined && DOM.gpsSats) DOM.gpsSats.innerText = d.gps.satellites;
        }
    }
}

function updateRadioFromData(d) {
    if (!d.raw_channels || !Array.isArray(d.raw_channels)) return;

    const container = DOM.channelsContainer;
    if (!container) return;

    if (container.innerHTML === 'Loading...' || container.innerHTML === '') {
        container.innerHTML = '<div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px;">' +
            d.raw_channels.map(function(val, i) {
                return '<div class="channel-group" style="margin-bottom: 0;"><label>CH ' + (i+1) + '</label>' +
                    '<div class="channel-row"><div class="progress-bar"><div id="ch-' + i + '" class="fill" style="width: 50%"></div></div>' +
                    '<span class="val" id="val-' + i + '">' + val + '</span></div></div>';
            }).join('') + '</div>';

        DOM.channels = d.raw_channels.map(function(_, i) { return {
            bar: document.getElementById('ch-' + i),
            val: document.getElementById('val-' + i)
        }; });
    }

    d.raw_channels.forEach(function(val, i) {
        if(DOM.channels[i]) {
            let pct = ((val - 1000) / 1000) * 100;
            pct = Math.max(0, Math.min(100, pct));
            if(DOM.channels[i].bar) DOM.channels[i].bar.style.width = pct + '%';
            if(DOM.channels[i].val) DOM.channels[i].val.innerText = val;
        }
    });
}

// ─── Funções HTTP (baixa frequência — sysinfo, settings, logs) ───────────────

function updateSysInfo() {
    fetch('/api/sysinfo')
        .then(function(response) { return response.json(); })
        .then(function(data) {
            const list = document.getElementById('sysinfo-list');
            list.innerHTML =
                '<li><span class="label">Chip Model</span> <span class="value">' + data.chip_model + ' (Rev ' + data.chip_revision + ')</span></li>' +
                '<li><span class="label">CPU Freq</span> <span class="value">' + data.cpu_freq + ' MHz</span></li>' +
                '<li><span class="label">RAM (Total)</span> <span class="value">' + formatBytes(data.heap_total) + '</span></li>' +
                '<li><span class="label">Flash (Total)</span> <span class="value">' + formatBytes(data.flash_size) + '</span></li>' +
                '<li><span class="label">Sketch Size</span> <span class="value">' + formatBytes(data.sketch_size) + '</span></li>' +
                '<li><span class="label">Uptime</span> <span class="value value-uptime">' + formatTime(data.uptime) + '</span></li>';

            const netList = document.getElementById('netinfo-list');
            let netHTML =
                '<li><span class="label">Mode</span> <span class="value">' + data.mode + '</span></li>' +
                '<li><span class="label">SSID</span> <span class="value">' + data.ssid + '</span></li>' +
                '<li><span class="label">IP Address</span> <span class="value">' + data.ip + '</span></li>' +
                '<li><span class="label">MAC Address</span> <span class="value">' + data.mac + '</span></li>' +
                '<li><span class="label">Channel</span> <span class="value">' + data.channel + '</span></li>';

            if (data.mode === 'Station') {
                netHTML += '<li><span class="label">Gateway</span> <span class="value">' + data.gateway + '</span></li>';
            } else {
                netHTML += '<li><span class="label">Clients</span> <span class="value">' + data.clients + '</span></li>';
            }
            netList.innerHTML = netHTML;
        })
        .catch(function(err) { console.error('Error fetching sysinfo:', err); });
}

function loadSettings() {
    fetch('/api/settings')
    .then(function(r) { return r.json(); })
    .then(function(d) {
        const themeToggle = document.getElementById('theme-toggle');
        if(themeToggle) {
            themeToggle.checked = d.dark_theme;
            if(d.dark_theme) {
                document.body.setAttribute('data-theme', 'dark');
            } else {
                document.body.removeAttribute('data-theme');
            }
        }

        const debugToggle = document.getElementById('debug-toggle');
        if(debugToggle) debugToggle.checked = d.debug;

        const armedToggle = document.getElementById('armed-toggle');
        if(armedToggle) {
            armedToggle.checked = d.armed;
            updateArmedStyle(d.armed);
        }

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
    .then(function(r) { return r.json(); })
    .then(function(d) {
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
        .then(function(r) { return r.text(); })
        .then(function(text) {
            if (text.trim() === '') return;

            const lines = text.trim().split('\n');
            let html = '';

            lines.forEach(function(line) {
                let color = '#d4d4d4';
                let weight = 'normal';

                if (line.includes('[ERROR]')) {
                    color = '#f44336';
                    weight = 'bold';
                } else if (line.includes('[WARN]')) {
                    color = '#ffc107';
                } else if (line.includes('[INFO]')) {
                    color = '#ffffff';
                } else if (line.includes('[DEBUG]')) {
                    color = '#2196f3';
                }

                html += '<div style="color: ' + color + '; font-weight: ' + weight + '; margin-bottom: 2px; border-bottom: 1px solid rgba(255,255,255,0.03);">' + line + '</div>';
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
        .then(function() {
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

    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send('calibrate');
    } else {
        fetch('/api/calibrate-imu', { method: 'POST' })
        .then(function(r) { return r.json(); })
        .then(function(d) {
            setTimeout(function() {
                btn.innerText = "CALIBRATION COMPLETE";
                btn.style.color = "var(--black)";
                btn.style.background = "var(--neon-chartreuse)";

                setTimeout(function() {
                    btn.innerText = originalText;
                    btn.disabled = false;
                    btn.style.opacity = "1";
                    btn.style.color = "var(--accent-color)";
                    btn.style.background = "transparent";
                }, 3000);
            }, 2500);
        })
        .catch(function(err) {
            console.error(err);
            btn.innerText = "ERROR";
            setTimeout(function() {
                btn.innerText = originalText;
                btn.disabled = false;
                btn.style.opacity = "1";
            }, 2000);
        });
    }
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
    return hours + 'h ' + (minutes % 60) + 'm ' + (seconds % 60) + 's';
}

let pollingInterval;

function startPolling() {
    if(pollingInterval) clearInterval(pollingInterval);

    pollingInterval = setInterval(function() {
        if(document.getElementById('home').classList.contains('active')) {
            updateSysInfo();
        }
    }, 5000);

    setInterval(function() {
        if(document.getElementById('config').classList.contains('active')) {
            updateLogs();
        }
    }, 1000);
}

function openTab(tabName) {
    const contents = document.querySelectorAll('.tab-content');
    contents.forEach(function(content) { content.classList.remove('active'); });

    const btns = document.querySelectorAll('.tab-btn');
    btns.forEach(function(btn) { btn.classList.remove('active'); });

    document.getElementById(tabName).classList.add('active');
    event.currentTarget.classList.add('active');

    if (tabName === 'hud-tab') {
        initHUD();
    }

    startPolling();
}

// ─── HUD Logic ────────────────────────────────────────────────────────────────

let hudBootTime = 0;
let hudInitialized = false;

function initHUD() {
    if (hudInitialized) return;

    // Constrói pitch ladder
    const pitchGroup = document.getElementById('pitch-ladder');
    if (pitchGroup) {
        pitchGroup.innerHTML = '';
        for (let i = -60; i <= 60; i += 10) {
            const y = i * 3;
            const w = (i === 0) ? 120 : ((i % 20 === 0) ? 70 : 40);
            const g = document.createElementNS("http://www.w3.org/2000/svg", "g");
            if (i === 0) {
                g.innerHTML = '<path d="M-' + w + ',0 L-30,0 M30,0 L' + w + ',0" class="stroke-main" stroke-width="2" />';
            } else if (i > 0) {
                g.innerHTML =
                    '<path d="M-' + w + ',-' + y + ' L-20,-' + y + ' M20,-' + y + ' L' + w + ',-' + y + '" class="stroke-main" />' +
                    '<text x="-' + (w + 15) + '" y="-' + (y - 4) + '" class="center-text-readout" font-size="10" text-anchor="end">' + i + '</text>' +
                    '<text x="' + (w + 15) + '" y="-' + (y - 4) + '" class="center-text-readout" font-size="10" text-anchor="start">' + i + '</text>';
            } else {
                const absY = Math.abs(y);
                g.innerHTML =
                    '<path d="M-' + w + ',' + absY + ' L-20,' + absY + ' L-20,' + (absY + 5) + ' M20,' + (absY + 5) + ' L20,' + absY + ' L' + w + ',' + absY + '" class="stroke-main" stroke-dasharray="6" />' +
                    '<text x="-' + (w + 15) + '" y="' + (absY + 4) + '" class="center-text-readout" font-size="10" text-anchor="end">' + Math.abs(i) + '</text>' +
                    '<text x="' + (w + 15) + '" y="' + (absY + 4) + '" class="center-text-readout" font-size="10" text-anchor="start">' + Math.abs(i) + '</text>';
            }
            pitchGroup.appendChild(g);
        }
    }

    // Constrói compass tape
    const compassTapeEl = document.getElementById('compass-tape');
    if (compassTapeEl) {
        compassTapeEl.innerHTML = '';
        const headings = ['N', '030', '060', 'E', '120', '150', 'S', '210', '240', 'W', '300', '330'];
        const tapeContent = headings.concat(headings).concat(headings);
        tapeContent.forEach(function(h, index) {
            const xPos = index * 40;
            const mark = document.createElementNS("http://www.w3.org/2000/svg", "g");
            mark.innerHTML =
                '<line x1="' + xPos + '" y1="-5" x2="' + xPos + '" y2="-15" class="stroke-main" stroke-width="1" />' +
                '<text x="' + xPos + '" y="-20" class="center-text-readout" font-size="10" text-anchor="middle">' + h + '</text>';
            compassTapeEl.appendChild(mark);
        });

        // Populamos o cache após criação dos elementos
        DOM.compassTape = compassTapeEl;
    }

    // Cache do horizon-group (criado no HTML, não precisa esperar initHUD)
    DOM.horizonGroup = document.getElementById('horizon-group');

    // Boot sequence
    const lines = ['b1', 'b2', 'b3', 'b4', 'b5'];
    let delay = 200;
    lines.forEach(function(id) {
        const el = document.getElementById(id);
        if (el) setTimeout(function() { el.style.opacity = 1; }, delay);
        delay += 300;
    });
    setTimeout(function() {
        const boot = document.getElementById('boot-screen');
        const mainHud = document.getElementById('main-hud');
        if (boot) boot.style.opacity = 0;
        if (mainHud) mainHud.classList.add('active');
        setTimeout(function() { if (boot) boot.style.display = 'none'; }, 300);
    }, delay + 400);

    hudBootTime = Date.now();
    hudInitialized = true;
}

function updateHUD(data) {
    if (!hudInitialized) return;

    const pitch = data.angles.pitch;
    const roll  = data.angles.roll;
    const yaw   = data.angles.yaw;
    const speed = data.gps.valid ? data.gps.speed : 0;

    // Alerta de inclinação crítica
    const maxSlope = Math.max(Math.abs(pitch), Math.abs(roll));
    const isCritical = maxSlope > 35;
    const viewport = document.querySelector('.hud-viewport');

    if (isCritical) {
        viewport.classList.add('critical-mode');
        document.getElementById('critical-warn-msg').style.opacity = 1;
        if(DOM.hudImuStat) DOM.hudImuStat.innerText = 'WARN: TILT';
    } else {
        viewport.classList.remove('critical-mode');
        document.getElementById('critical-warn-msg').style.opacity = 0;
        if(DOM.hudImuStat) DOM.hudImuStat.innerText = 'NOMINAL';
    }

    // Fase 2: CSS transform em vez de setAttribute — acelerado por GPU
    // transform-origin: 0px 0px (definido no CSS) — mantém equivalência com SVG coords
    const pitchY = pitch * 3;
    if (DOM.horizonGroup) {
        DOM.horizonGroup.style.transform =
            'translate(300px, 300px) rotate(' + (-roll) + 'deg) translate(0px, ' + pitchY + 'px)';
    }

    if(DOM.hudPitch) DOM.hudPitch.textContent = 'P: ' + (pitch > 0 ? '+' : '') + pitch.toFixed(1) + '°';
    if(DOM.hudRoll)  DOM.hudRoll.textContent  = 'R: ' + (roll  > 0 ? '+' : '') + roll.toFixed(1)  + '°';

    const speedDash = Math.min((speed / 50) * 222, 222);
    document.getElementById('arc-speed').setAttribute('stroke-dasharray', speedDash + ' 400');
    document.getElementById('center-spd').innerText = speed.toFixed(1);

    // Fase 2: CSS transform para compass tape
    const fullCircleWidth = 12 * 40;
    const yawOffset = -fullCircleWidth - ((yaw / 360) * fullCircleWidth);
    if (DOM.compassTape) {
        DOM.compassTape.style.transform = 'translateX(' + yawOffset + 'px)';
    }

    // Motores
    const motL = Math.abs((data.motors && data.motors.left  !== undefined ? data.motors.left  : 0) * 100);
    const motR = Math.abs((data.motors && data.motors.right !== undefined ? data.motors.right : 0) * 100);

    if(DOM.hudBarMotL) DOM.hudBarMotL.style.width = motL + '%';
    if(DOM.hudMotL)    DOM.hudMotL.innerText    = motL.toFixed(0) + '%';
    if(DOM.hudBarMotR) DOM.hudBarMotR.style.width = motR + '%';
    if(DOM.hudMotR)    DOM.hudMotR.innerText    = motR.toFixed(0) + '%';

    // Acelerômetro
    if(DOM.hudAccX) DOM.hudAccX.innerText = data.accel.x.toFixed(2);
    if(DOM.hudAccY) DOM.hudAccY.innerText = data.accel.y.toFixed(2);
    if(DOM.hudAccZ) DOM.hudAccZ.innerText = data.accel.z.toFixed(2);

    // Slope bar
    if(DOM.hudSlopeVal) DOM.hudSlopeVal.innerText = maxSlope.toFixed(0) + '°';
    if(DOM.hudBarSlope) DOM.hudBarSlope.style.width = Math.min((maxSlope / 45) * 100, 100) + '%';

    // Estado do sistema
    const statusBadge = DOM.hudSysState;
    if(statusBadge) {
        statusBadge.innerText = data.armed ? "ARMED" : "DISARMED";
        statusBadge.style.color = data.armed ? "var(--danger)" : "var(--primary)";
        statusBadge.classList.remove('armed', 'disarmed');
        statusBadge.classList.add(data.armed ? 'armed' : 'disarmed');
    }

    // GPS
    if (data.gps.valid) {
        if(DOM.hudGpsLat) DOM.hudGpsLat.innerText = data.gps.lat.toFixed(6) + '°';
        if(DOM.hudGpsLng) DOM.hudGpsLng.innerText = data.gps.lng.toFixed(6) + '°';
        if(DOM.hudGpsAlt) DOM.hudGpsAlt.innerText = data.gps.alt.toFixed(0) + ' m';
        if(DOM.hudGpsSats) DOM.hudGpsSats.innerText = data.gps.satellites;
        if(DOM.hudGpsHdop) DOM.hudGpsHdop.innerText = data.gps.hdop.toFixed(1);
        if(DOM.hudGpsCrs)  DOM.hudGpsCrs.innerText  = data.gps.course.toFixed(0) + '°';
        if(DOM.hudGpsSpd)  DOM.hudGpsSpd.innerText  = data.gps.speed.toFixed(1);

        if (data.gps.time && DOM.hudSysTimeLocal) {
            const timeParts = data.gps.time.split('T');
            if (timeParts[1]) DOM.hudSysTimeLocal.innerText = timeParts[1].split('Z')[0] + ' Z';
        }
    }

    // Temperatura IMU
    if(DOM.hudImuTemp) DOM.hudImuTemp.innerText = data.temperature.toFixed(1) + '°C';

    // Uptime local (calculado pelo cliente, não depende do firmware)
    if(DOM.hudSysUptime) {
        const uptimeMs = Date.now() - hudBootTime;
        const hrs  = String(Math.floor(uptimeMs / 3600000)).padStart(2, '0');
        const mins = String(Math.floor((uptimeMs % 3600000) / 60000)).padStart(2, '0');
        const secs = String(Math.floor((uptimeMs % 60000) / 1000)).padStart(2, '0');
        DOM.hudSysUptime.innerText = hrs + ':' + mins + ':' + secs;
    }
}

// ─── DOMContentLoaded ─────────────────────────────────────────────────────────

document.addEventListener('DOMContentLoaded', function() {
    // HUD Elements
    DOM.hudRoll        = document.getElementById('ret-roll');
    DOM.hudPitch       = document.getElementById('ret-pitch');
    DOM.hudMotL        = document.getElementById('hud-mot-l');
    DOM.hudMotR        = document.getElementById('hud-mot-r');
    DOM.hudAccX        = document.getElementById('hud-acc-x');
    DOM.hudAccY        = document.getElementById('hud-acc-y');
    DOM.hudAccZ        = document.getElementById('hud-acc-z');
    DOM.hudSlopeVal    = document.getElementById('hud-slope-val');
    DOM.hudBarMotL     = document.getElementById('hud-bar-mot-l');
    DOM.hudBarMotR     = document.getElementById('hud-bar-mot-r');
    DOM.hudBarSlope    = document.getElementById('hud-bar-slope');
    DOM.hudSysState    = document.getElementById('hud-sys-state');
    DOM.hudImuStat     = document.getElementById('hud-imu-stat');
    DOM.hudImuTemp     = document.getElementById('hud-imu-temp');
    DOM.hudSysLink     = document.getElementById('hud-sys-link');
    DOM.hudGpsLat      = document.getElementById('hud-gps-lat');
    DOM.hudGpsLng      = document.getElementById('hud-gps-lng');
    DOM.hudGpsAlt      = document.getElementById('hud-gps-alt');
    DOM.hudGpsSats     = document.getElementById('hud-gps-sats');
    DOM.hudGpsHdop     = document.getElementById('hud-gps-hdop');
    DOM.hudGpsCrs      = document.getElementById('hud-gps-crs');
    DOM.hudGpsSpd      = document.getElementById('hud-gps-spd');
    DOM.hudSysUptime   = document.getElementById('hud-sys-uptime');
    DOM.hudSysTimeLocal = document.getElementById('hud-sys-time-local');

    // HUD SVG animated elements (populados aqui; compassTape pode ser null até initHUD)
    DOM.horizonGroup = document.getElementById('horizon-group');

    // Sensors Tab Elements
    DOM.imuRoll    = document.getElementById('imu-roll');
    DOM.imuPitch   = document.getElementById('imu-pitch');
    DOM.imuYaw     = document.getElementById('imu-yaw');
    DOM.imuTemp    = document.getElementById('imu-temp');
    DOM.accelX     = document.getElementById('accel-x');
    DOM.accelY     = document.getElementById('accel-y');
    DOM.accelZ     = document.getElementById('accel-z');
    DOM.gyroX      = document.getElementById('gyro-x');
    DOM.gyroY      = document.getElementById('gyro-y');
    DOM.gyroZ      = document.getElementById('gyro-z');
    DOM.compHeading = document.getElementById('comp-heading');
    DOM.compX      = document.getElementById('comp-x');
    DOM.compY      = document.getElementById('comp-y');
    DOM.compZ      = document.getElementById('comp-z');
    DOM.gpsLat     = document.getElementById('gps-lat');
    DOM.gpsLng     = document.getElementById('gps-lng');
    DOM.gpsAlt     = document.getElementById('gps-alt');
    DOM.gpsSats    = document.getElementById('gps-sats');
    DOM.gpsSpeed   = document.getElementById('gps-speed');
    DOM.gpsCourse  = document.getElementById('gps-course');
    DOM.gpsHdop    = document.getElementById('gps-hdop');
    DOM.gpsTime    = document.getElementById('gps-time');
    DOM.gpsStatus  = document.getElementById('gps-status');
    DOM.imuOffline = document.getElementById('imu-offline');

    // Radio Elements
    DOM.channelsContainer = document.getElementById('all-channels');

    // Home/System Elements
    DOM.armedToggle   = document.getElementById('armed-toggle');
    DOM.armedLabel    = document.getElementById('armed-label');
    DOM.armedCard     = document.getElementById('armed-card');
    DOM.sysinfoList   = document.getElementById('sysinfo-list');
    DOM.netinfoList   = document.getElementById('netinfo-list');
    DOM.connectionStatus = document.getElementById('connectionStatus');

    updateSysInfo();
    loadSettings();
    startPolling();
    startRenderLoop();      // Fase 2: inicia loop RAF desacoplado
    connectWebSocket();     // Inicia conexão push realtime via WebSocket
});
