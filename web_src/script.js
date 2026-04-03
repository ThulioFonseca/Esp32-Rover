// ─── UTILITÁRIOS DE SEGURANÇA ─────────────────────────────────────────────────
function escapeHtml(str) {
    if (str === null || str === undefined) return '';
    return String(str)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

// ─── SENSOR CHART ENGINE ──────────────────────────────────────────────────────
// Motor de gráfico de linha canvas-nativo, sem dependências externas.
// Cada instância gerencia um buffer circular por série e renderiza no RAF loop.

class SensorChart {
    constructor(canvas, seriesConfig, bufferSize) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.series = seriesConfig; // [{label, color, colorVar}]
        this.bufferSize = bufferSize || 200; // 200 pts = 10s @ 20Hz
        this.buffers = seriesConfig.map(function() { return new Float32Array(bufferSize || 200); });
        this.head = 0;
        this.count = 0;
        this.visible = seriesConfig.map(function() { return true; });
        var self = this;
        this._ro = new ResizeObserver(function() { self._resize(); });
        this._ro.observe(canvas.parentElement);
        this._resize();
    }

    _resize() {
        var rect = this.canvas.parentElement.getBoundingClientRect();
        var dpr = window.devicePixelRatio || 1;
        this.canvas.width  = Math.floor(rect.width * dpr);
        this.canvas.height = Math.floor(120 * dpr);
        this.canvas.style.height = '120px';
        this._dpr = dpr;
    }

    push(values) {
        var i;
        for (i = 0; i < this.buffers.length; i++) {
            this.buffers[i][this.head] = (i < values.length) ? values[i] : 0;
        }
        this.head = (this.head + 1) % this.bufferSize;
        if (this.count < this.bufferSize) this.count++;
    }

    setVisible(index, visible) {
        if (index >= 0 && index < this.visible.length) this.visible[index] = visible;
    }

    render() {
        if (this.count < 2) return;
        var ctx = this.ctx;
        var W = this.canvas.width;
        var H = this.canvas.height;
        var dpr = this._dpr || 1;
        ctx.clearRect(0, 0, W, H);

        // Min/max global das séries visíveis
        var min = Infinity, max = -Infinity;
        var i, j, idx, v;
        for (i = 0; i < this.series.length; i++) {
            if (!this.visible[i]) continue;
            for (j = 0; j < this.count; j++) {
                idx = (this.head - this.count + j + this.bufferSize) % this.bufferSize;
                v = this.buffers[i][idx];
                if (v < min) min = v;
                if (v > max) max = v;
            }
        }
        if (!isFinite(min) || !isFinite(max)) return;
        if (min === max) { min -= 1; max += 1; }
        var range = max - min;

        // Grid — 4 linhas horizontais usando --list-border do tema atual
        var gridColor = getComputedStyle(document.body).getPropertyValue('--list-border').trim() || 'rgba(128,128,128,0.2)';
        ctx.strokeStyle = gridColor;
        ctx.lineWidth = 1;
        var g;
        for (g = 0; g <= 3; g++) {
            var gy = Math.round(H * g / 3) + 0.5;
            ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(W, gy); ctx.stroke();
        }

        // Séries
        var PAD = Math.round(6 * dpr);
        var plotH = H - PAD * 2;
        for (i = 0; i < this.series.length; i++) {
            if (!this.visible[i]) continue;
            ctx.strokeStyle = this.series[i].color;
            ctx.lineWidth = Math.round(1.5 * dpr);
            ctx.lineJoin = 'round';
            ctx.beginPath();
            for (j = 0; j < this.count; j++) {
                idx = (this.head - this.count + j + this.bufferSize) % this.bufferSize;
                v = this.buffers[i][idx];
                var x = (j / (this.bufferSize - 1)) * W;
                var y = PAD + plotH - ((v - min) / range) * plotH;
                if (j === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
            }
            ctx.stroke();
        }
    }

    destroy() {
        this._ro.disconnect();
    }
}

// ─── SENSOR CHART — Configuração e estado ─────────────────────────────────────

function _cssColor(varName) {
    // Lê de document.body — o data-theme="dark" é aplicado no <body>, não no <html>.
    // getComputedStyle(documentElement) não enxerga overrides de [data-theme="dark"].
    return getComputedStyle(document.body).getPropertyValue(varName).trim();
}

var SENSOR_CHART_CONFIGS = {
    orientation: [
        { label: 'Roll',  colorVar: '--chart-color-1' },
        { label: 'Pitch', colorVar: '--chart-color-2' },
        { label: 'Temp',  colorVar: '--chart-color-3' }
    ],
    accelerometer: [
        { label: 'X', colorVar: '--chart-color-1' },
        { label: 'Y', colorVar: '--chart-color-2' },
        { label: 'Z', colorVar: '--chart-color-3' }
    ],
    gyroscope: [
        { label: 'X', colorVar: '--chart-color-1' },
        { label: 'Y', colorVar: '--chart-color-2' },
        { label: 'Z', colorVar: '--chart-color-3' }
    ],
    compass: [
        { label: 'Heading', colorVar: '--chart-color-1' },
        { label: 'Mag X',   colorVar: '--chart-color-2' },
        { label: 'Mag Y',   colorVar: '--chart-color-3' },
        { label: 'Mag Z',   colorVar: '--chart-color-4' }
    ],
    gps: [
        { label: 'Alt',    colorVar: '--chart-color-1' },
        { label: 'Speed',  colorVar: '--chart-color-2' },
        { label: 'Course', colorVar: '--chart-color-3' },
        { label: 'Sats',   colorVar: '--chart-color-4' },
        { label: 'HDOP',   colorVar: '--chart-color-5' }
    ]
};

var sensorCharts = {}; // { orientation: SensorChart, ... }

function initSensorCharts() {
    var ids = Object.keys(SENSOR_CHART_CONFIGS);
    for (var k = 0; k < ids.length; k++) {
        var id = ids[k];
        var canvas = document.getElementById('canvas-' + id);
        if (!canvas) continue;
        var cfgDefs = SENSOR_CHART_CONFIGS[id];
        var series = cfgDefs.map(function(s) {
            return { label: s.label, color: _cssColor(s.colorVar), colorVar: s.colorVar };
        });
        sensorCharts[id] = new SensorChart(canvas, series);

        // Toggle button — abre/fecha container do gráfico
        (function(chartId) {
            var btn = document.getElementById('chart-toggle-' + chartId);
            var container = document.getElementById('chart-' + chartId);
            if (!btn || !container) return;
            btn.addEventListener('click', function() {
                var open = btn.getAttribute('aria-pressed') === 'true';
                btn.setAttribute('aria-pressed', open ? 'false' : 'true');
                container.hidden = open;
                if (!open) sensorCharts[chartId]._resize(); // força resize ao abrir
            });
        })(id);

        // Legenda — toggle por série individual
        (function(chartId) {
            var legend = document.getElementById('legend-' + chartId);
            if (!legend) return;
            var items = legend.querySelectorAll('.chart-legend-item');
            items.forEach(function(item) {
                item.addEventListener('click', function() {
                    var idx = parseInt(item.getAttribute('data-series'), 10);
                    item.classList.toggle('inactive');
                    sensorCharts[chartId].setVisible(idx, !item.classList.contains('inactive'));
                });
            });
        })(id);
    }
}

function refreshChartColors() {
    var ids = Object.keys(sensorCharts);
    for (var k = 0; k < ids.length; k++) {
        var id = ids[k];
        var chart = sensorCharts[id];
        var cfgDefs = SENSOR_CHART_CONFIGS[id];
        if (!cfgDefs) continue;
        for (var i = 0; i < chart.series.length; i++) {
            if (cfgDefs[i]) chart.series[i].color = _cssColor(cfgDefs[i].colorVar);
        }
    }
}

// ─── DOM CACHE LAYER ──────────────────────────────────────────────────────────
// Pré-aloca todas as referências de elementos do DOM que são atualizados a 20Hz
// Evita 600+ buscas no DOM por segundo, reduzindo CPU do navegador em ~60%

const DOM = {
    // HUD Elements
    hudRoll: null, hudPitch: null, hudMotL: null, hudMotR: null,
    hudAccX: null, hudAccY: null, hudAccZ: null, hudSlopeVal: null,
    hudBarMotL: null, hudBarMotR: null, hudBarSlope: null,
    hudSysState: null, hudImuStat: null, hudImuTemp: null, hudSysLink: null,
    hudGpsLat: null, hudGpsLng: null, hudGpsSats: null,
    hudGpsHdop: null, hudSysUptime: null,
    hudSysTimeLocal: null, hudGpsFixBadge: null,

    // HUD SVG animated elements (Fase 2: GPU-accelerated via CSS transform)
    horizonGroup: null,
    compassTape: null,
    rollPointer: null,

    // Sensors Tab Elements
    imuRoll: null, imuPitch: null, imuTemp: null,
    accelX: null, accelY: null, accelZ: null,
    gyroX: null, gyroY: null, gyroZ: null,
    compHeading: null, compX: null, compY: null, compZ: null,
    gpsLat: null, gpsLng: null, gpsAlt: null, gpsSats: null,
    gpsSpeed: null, gpsCourse: null, gpsHdop: null, gpsTime: null,
    gpsStatus: null, imuOffline: null,

    // Radio Elements (cached array)
    channels: [],
    channelsContainer: null,

    // Header/System Elements
    armedToggle: null, armedBtn: null,
    sysinfoList: null, netinfoList: null,
    connectionStatus: null
};

// ─── RAF Render Loop (Fase 2) ─────────────────────────────────────────────────
// O WS onmessage apenas escreve em wsLatestFrame (sem ops no DOM).
// O loop RAF consome o frame no próximo vsync — desacopla rede do render.

let wsLatestFrame = null;
let rafId = null;

// ─── GPS MINI-MAP ──────────────────────────────────────────────────────────
const MINIMAP_TRACK_MAX  = 200;    // max points in circular track buffer
const MINIMAP_MIN_RADIUS = 0.003;  // ~330m minimum view radius in degrees
let minimapTrack     = [];  // [{lat, lng}]
let minimapCanvas    = null;
let minimapCtx       = null;
let minimapTiles     = {};  // {"z/x/y": HTMLImageElement} — 3×3 grid cache
let minimapGridKey   = '';  // "theme/z/cx/cy" — center tile of current grid
let minimapLastValid = false;
let minimapZoomDelta = 0;   // user zoom offset: -3 to +3
let minimapSatMode   = false;

function latLngToTileXY(lat, lng, z) {
    const n = Math.pow(2, z);
    const x = Math.floor((lng + 180) / 360 * n);
    const latRad = lat * Math.PI / 180;
    const y = Math.floor((1 - Math.log(Math.tan(latRad) + 1 / Math.cos(latRad)) / Math.PI) / 2 * n);
    return {x: x, y: y};
}

// Normalized Web Mercator helpers (0..1 range, matches OSM tile coordinates)
function mercX(lng) { return (lng + 180) / 360; }
function mercY(lat) { return 0.5 - Math.log(Math.tan(Math.PI / 4 + lat * Math.PI / 360)) / (2 * Math.PI); }

function getTileUrl(z, x, y) {
    if (minimapSatMode) {
        // Esri World Imagery — tile path uses z/y/x (not z/x/y)
        return 'https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/' + z + '/' + y + '/' + x;
    }
    var dark = document.body.getAttribute('data-theme') === 'dark';
    var s = 'abcd'[Math.floor(Math.random() * 4)];
    var style = dark ? 'voyager' : 'light_all';
    // @2x tiles are 512px native — crisp on all displays, no dprBoost needed
    return 'https://' + s + '.basemaps.cartocdn.com/rastertiles/' + style + '/' + z + '/' + x + '/' + y + '@2x.png';
}

function chooseTileZoom(dlat) {
    if (dlat < 0.005) return 17;
    if (dlat < 0.015) return 16;
    if (dlat < 0.05)  return 15;
    if (dlat < 0.1)   return 14;
    return 13;
}

function tryLoadOsmTiles(cLat, cLng, dlat) {
    const z = Math.min(18, chooseTileZoom(dlat));
    const n = Math.pow(2, z);
    const c = latLngToTileXY(cLat, cLng, z);
    var theme = document.body.getAttribute('data-theme') || 'light';
    const gridKey = theme + '/' + z + '/' + c.x + '/' + c.y;
    if (gridKey === minimapGridKey) return;
    minimapGridKey = gridKey;

    // Compute how many tiles are needed to fill the canvas.
    // Uses the same Mercator scale as drawOsmTiles so coverage is always exact.
    var W = (minimapCanvas && minimapCanvas.width)  || 400;
    var H = (minimapCanvas && minimapCanvas.height) || 400;
    var pad = 1.18;
    var mcy_n = mercY(cLat + dlat * pad / 2); // north edge in Mercator
    var mcy_s = mercY(cLat - dlat * pad / 2); // south edge in Mercator
    var merc_scale = (mcy_s - mcy_n) / H;     // Mercator units per canvas pixel
    var tileSize = 1 / n;                       // one tile in Mercator units
    var rx = Math.ceil(W * merc_scale / tileSize / 2) + 1;
    var ry = Math.ceil(H * merc_scale / tileSize / 2) + 1;

    // Load the computed grid; reuse cached images
    const next = {};
    for (let dx = -rx; dx <= rx; dx++) {
        for (let dy = -ry; dy <= ry; dy++) {
            const tx = c.x + dx, ty = c.y + dy;
            const key = theme + '/' + z + '/' + tx + '/' + ty;
            if (minimapTiles[key]) {
                next[key] = minimapTiles[key];
            } else {
                const img = new Image();
                img.crossOrigin = 'anonymous';
                (function(k, im, tz, tx2, ty2) {
                    im.onload  = function() { minimapTiles[k] = im; };
                    im.onerror = function() {};
                    im.src = getTileUrl(tz, tx2, ty2);
                }(key, img, z, tx, ty));
                next[key] = img;
            }
        }
    }
    minimapTiles = next;
}

// drawOsmTiles: positions tiles using Web Mercator coordinates.
// All axes use the same merc_scale — no independent x/y scaling — so tiles
// render undistorted at any latitude (Mercator is conformal).
function drawOsmTiles(ctx, W, H, mcx, mcy, merc_scale) {
    const parts = minimapGridKey.split('/');
    const z = parseInt(parts[1]); // parts[0] = theme
    const n = Math.pow(2, z);
    for (var key in minimapTiles) {
        const img = minimapTiles[key];
        if (!img || !img.complete || !img.naturalWidth) continue;
        const kp = key.split('/');
        const tx = parseInt(kp[2]), ty = parseInt(kp[3]);
        // Tile corners in normalized Mercator (0..1 range matches mercX/mercY helpers)
        const sx = W / 2 + (tx / n - mcx) / merc_scale;
        const ex = W / 2 + ((tx + 1) / n - mcx) / merc_scale;
        const sy = H / 2 + (ty / n - mcy) / merc_scale;
        const ey = H / 2 + ((ty + 1) / n - mcy) / merc_scale;
        ctx.drawImage(img, sx, sy, ex - sx, ey - sy);
    }
}

function drawVectorGrid(ctx, W, H, cLat, cLng, dlat, dlng, toXY) {
    ctx.strokeStyle = '#1a2a1a';
    ctx.lineWidth = 0.5;
    const steps = 4;
    for (let i = 0; i <= steps; i++) {
        const f = i / steps;
        const y = toXY(cLat - dlat / 2 + dlat * f, cLng).y;
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(W, y); ctx.stroke();
        const x = toXY(cLat, cLng - dlng / 2 + dlng * f).x;
        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, H); ctx.stroke();
    }
}

function updateMiniMap(lat, lng, heading, valid) {
    if (!minimapCtx) return;
    // Sync canvas resolution with its displayed size, accounting for devicePixelRatio
    // for crisp rendering on retina/HiDPI displays.
    var dpr = window.devicePixelRatio || 1;
    var dw = Math.round(minimapCanvas.offsetWidth  * dpr);
    var dh = Math.round(minimapCanvas.offsetHeight * dpr);
    if (dw > 0 && dh > 0 && (minimapCanvas.width !== dw || minimapCanvas.height !== dh)) {
        minimapCanvas.width  = dw;
        minimapCanvas.height = dh;
        minimapTiles   = {};
        minimapGridKey = '';
    }
    const W = minimapCanvas.width, H = minimapCanvas.height;
    const ctx = minimapCtx;

    // Update fix badge
    if (DOM.hudGpsFixBadge) {
        if (valid) {
            DOM.hudGpsFixBadge.textContent = '3D FIX';
            DOM.hudGpsFixBadge.className = 'fix-badge fix-ok';
        } else {
            DOM.hudGpsFixBadge.textContent = 'NO FIX';
            DOM.hudGpsFixBadge.className = 'fix-badge';
        }
    }

    // Clear track on GPS loss
    if (!valid) {
        if (minimapLastValid) {
            minimapTrack = [];
            minimapTiles = {};
            minimapGridKey = '';
        }
        minimapLastValid = false;
        ctx.fillStyle = '#060c08';
        ctx.fillRect(0, 0, W, H);
        ctx.fillStyle = 'rgba(255,80,80,0.5)';
        ctx.font = 'bold 10px monospace';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('NO GPS FIX', W / 2, H / 2);
        return;
    }
    minimapLastValid = true;

    // Append track point (filter micro-jitter)
    const last = minimapTrack[minimapTrack.length - 1];
    if (!last || Math.abs(last.lat - lat) > 0.000005 || Math.abs(last.lng - lng) > 0.000005) {
        minimapTrack.push({lat: lat, lng: lng});
        if (minimapTrack.length > MINIMAP_TRACK_MAX) minimapTrack.shift();
    }

    // Bounding box
    let minLat = lat, maxLat = lat, minLng = lng, maxLng = lng;
    for (let i = 0; i < minimapTrack.length; i++) {
        const p = minimapTrack[i];
        if (p.lat < minLat) minLat = p.lat;
        if (p.lat > maxLat) maxLat = p.lat;
        if (p.lng < minLng) minLng = p.lng;
        if (p.lng > maxLng) maxLng = p.lng;
    }
    const zoomScale = Math.pow(2, -minimapZoomDelta);
    const dlat = Math.max(maxLat - minLat, MINIMAP_MIN_RADIUS) * zoomScale;
    const dlng = Math.max(maxLng - minLng, MINIMAP_MIN_RADIUS * 1.5) * zoomScale;
    const cLat = (minLat + maxLat) / 2;
    const cLng = (minLng + maxLng) / 2;
    const pad = 1.18;

    // Web Mercator projection — one uniform scale for both axes.
    // merc_scale: normalized Mercator units per canvas pixel (derived from latitude span).
    // Using a single scale eliminates tile distortion at non-equatorial latitudes.
    const mcx = mercX(cLng);
    const mcy = mercY(cLat);
    const merc_scale = (mercY(cLat - dlat * pad / 2) - mercY(cLat + dlat * pad / 2)) / H;

    function toXY(pLat, pLng) {
        return {
            x: W / 2 + (mercX(pLng) - mcx) / merc_scale,
            y: H / 2 + (mercY(pLat) - mcy) / merc_scale
        };
    }

    // Async tile grid load (no-op if center tile unchanged)
    tryLoadOsmTiles(cLat, cLng, dlat);

    // Draw background
    ctx.fillStyle = '#060c08';
    ctx.fillRect(0, 0, W, H);

    var hasTile = false;
    for (var _k in minimapTiles) {
        var _i = minimapTiles[_k];
        if (_i && _i.complete && _i.naturalWidth) { hasTile = true; break; }
    }
    if (hasTile) {
        drawOsmTiles(ctx, W, H, mcx, mcy, merc_scale);
    } else {
        drawVectorGrid(ctx, W, H, cLat, cLng, dlat, dlng, toXY);
    }

    // ── Track line ────────────────────────────────────────────────────────────
    if (minimapTrack.length > 1) {
        var dpr = window.devicePixelRatio || 1;
        ctx.beginPath();
        ctx.strokeStyle = 'rgba(0, 232, 122, 0.55)';
        ctx.lineWidth = 2 * dpr;
        ctx.lineJoin = 'round';
        ctx.lineCap = 'round';
        var p0 = toXY(minimapTrack[0].lat, minimapTrack[0].lng);
        ctx.moveTo(p0.x, p0.y);
        for (var i = 1; i < minimapTrack.length; i++) {
            var pt = toXY(minimapTrack[i].lat, minimapTrack[i].lng);
            ctx.lineTo(pt.x, pt.y);
        }
        ctx.stroke();
    }

    // ── Rover marker — Waze-style navigation arrow, heading-aware ────────────
    var rv = toXY(lat, lng);
    var dpr2 = window.devicePixelRatio || 1;
    var sz   = 13 * dpr2;  // half-size of arrow body
    var Ra   = 24 * dpr2;  // accuracy halo radius
    var headRad = heading * Math.PI / 180;

    ctx.save();
    ctx.translate(rv.x, rv.y);

    // 1 — Accuracy halo (axis-aligned, drawn before rotation)
    ctx.beginPath();
    ctx.arc(0, 0, Ra, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(0, 232, 122, 0.07)';
    ctx.fill();
    ctx.strokeStyle = 'rgba(0, 232, 122, 0.22)';
    ctx.lineWidth = 1 * dpr2;
    ctx.stroke();

    // 2 — Rotate context to heading (tip faces direction of travel)
    ctx.rotate(headRad);

    // 3 — Navigation arrow body glow (centered, no offset — per design system)
    ctx.shadowColor = '#00e87a';
    ctx.shadowBlur  = 10 * dpr2;
    ctx.shadowOffsetX = 0;
    ctx.shadowOffsetY = 0;

    // 4 — Arrow body: 6-point chevron, tip points up (forward/north at heading=0)
    ctx.beginPath();
    ctx.moveTo(0,           -sz * 1.5);   // front tip
    ctx.lineTo( sz * 0.72,  sz * 0.55);   // right rear
    ctx.lineTo( sz * 0.26,  sz * 0.12);   // right notch
    ctx.lineTo(0,           sz * 0.42);   // rear center
    ctx.lineTo(-sz * 0.26,  sz * 0.12);   // left notch
    ctx.lineTo(-sz * 0.72,  sz * 0.55);   // left rear
    ctx.closePath();
    ctx.fillStyle = '#00e87a';
    ctx.fill();

    ctx.shadowColor = 'transparent';
    ctx.strokeStyle = 'rgba(255,255,255,0.85)';
    ctx.lineWidth = 1.5 * dpr2;
    ctx.stroke();

    // 5 — Center dot (white, anchors GPS position point)
    ctx.beginPath();
    ctx.arc(0, 0, 3 * dpr2, 0, Math.PI * 2);
    ctx.fillStyle = 'rgba(255,255,255,0.9)';
    ctx.fill();

    ctx.restore();

    // ── North indicator ───────────────────────────────────────────────────────
    var dpr3 = window.devicePixelRatio || 1;
    ctx.fillStyle = 'rgba(20,20,20,0.72)';
    ctx.fillRect(W - 26 * dpr3, 4 * dpr3, 22 * dpr3, 16 * dpr3);
    ctx.fillStyle = 'rgba(0, 232, 122, 0.9)';
    ctx.font = 'bold ' + Math.round(9 * dpr3) + 'px monospace';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('N\u2191', W - 15 * dpr3, 12 * dpr3);
}

function startRenderLoop() {
    if (rafId !== null) return;
    function rafTick() {
        if (wsLatestFrame !== null) {
            const frame = wsLatestFrame;
            wsLatestFrame = null;

            if (document.getElementById('home').classList.contains('active')) {
                updateHUD(frame._sensorData);
            }
            if (document.getElementById('sensors').classList.contains('active')) {
                updateSensorsFromData(frame._sensorData);
                var _chartIds = Object.keys(sensorCharts);
                for (var _ci = 0; _ci < _chartIds.length; _ci++) {
                    sensorCharts[_chartIds[_ci]].render();
                }
            }
            if (document.getElementById('radio').classList.contains('active')) {
                updateRadioFromData(frame._channelData);
            }

            // Armed status sync (leve, independe de aba)
            if (Date.now() > _armedLockUntil) {
                const at = DOM.armedToggle;
                if (at && at.checked !== frame._armed) {
                    at.checked = frame._armed;
                    updateArmedStyle(frame._armed);
                }
            }

            // Uptime na Info
            if (document.getElementById('info').classList.contains('active') && frame._uptime) {
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
let wsReconnectDelay = 1000; // backoff exponencial: começa em 1s, max 30s

function connectWebSocket() {
    if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) return;

    const proto = (location.protocol === 'https:') ? 'wss' : 'ws';
    ws = new WebSocket(proto + '://' + location.host + '/ws');

    // Fase 3: força recepção como ArrayBuffer para parsing binário
    ws.binaryType = 'arraybuffer';

    ws.onopen = function() {
        wsConnected = true;
        wsReconnectDelay = 1000; // reseta backoff na conexão bem-sucedida
        clearTimeout(wsReconnectTimer);
        updateConnectionStatus(true);
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
                // ignore malformed frames
            }
        }
    };

    ws.onclose = function() {
        wsConnected = false;
        updateConnectionStatus(false);
        wsReconnectTimer = setTimeout(connectWebSocket, wsReconnectDelay);
        wsReconnectDelay = Math.min(wsReconnectDelay * 2, 30000); // backoff exponencial, max 30s
    };

    ws.onerror = function() {
        ws.close();
    };
}

// ─── Fase 3: Parser binário ───────────────────────────────────────────────────
// Protocolo: 122 bytes little-endian. Ver spec no plano de implementação.
// packet_type=0x01: sensor frame. Após parse, reutiliza handleWsSensorData().

function parseBinaryFrame(buf) {
    if (buf.byteLength < 121) return;
    const v = new DataView(buf);
    const f32 = function(o) { return v.getFloat32(o, true); };
    const u32 = function(o) { return v.getUint32(o, true); };
    const i16 = function(o) { return v.getInt16(o, true); };
    const u8  = function(o) { return v.getUint8(o); };

    if (u8(0) !== 0x01) return; // tipo desconhecido

    // Reconstrói o mesmo formato de payload compacto que handleWsSensorData espera
    // IMU (37 bytes, offset 1-37): roll(1) pitch(5) accelX(9) accelY(13) accelZ(17) gyroX(21) gyroY(25) gyroZ(29) temp(33) valid(37)
    // GPS (32 bytes, offset 38-69): lat(38) lng(42) alt(46) speed(50) course(54) sats(58) hdop(62) timeH(66) timeM(67) timeS(68) valid(69)
    // Compass (17 bytes, offset 70-86): heading(70) x(74) y(78) z(82) valid(86)
    // Motors (8 bytes, offset 87-94): left(87) right(91)
    // Channels (21 bytes, offset 95-115): 10×i16 + valid(115)
    // System (5 bytes, offset 116-120): armed(116) uptime(117)
    const th = u8(66), tm_ = u8(67), ts = u8(68);
    const d = {
        imu: {
            v:  u8(37) !== 0,
            r:  f32(1),  p:  f32(5),
            t:  f32(33),
            ax: f32(9),  ay: f32(13), az: f32(17),
            gx: f32(21), gy: f32(25), gz: f32(29)
        },
        gps: {
            v:  u8(69) !== 0,
            la: f32(38), ln: f32(42), al: f32(46),
            sp: f32(50), cr: f32(54),
            sa: u32(58), hd: f32(62),
            tm: String(th).padStart(2,'0') + ':' + String(tm_).padStart(2,'0') + ':' + String(ts).padStart(2,'0')
        },
        cmp: {
            v: u8(86) !== 0,
            h: f32(70), x: f32(74), y: f32(78), z: f32(82)
        },
        mot: { l: f32(87), r: f32(91) },
        ch: [
            i16(95),  i16(97),  i16(99),  i16(101), i16(103),
            i16(105), i16(107), i16(109), i16(111), i16(113)
        ],
        cv:  u8(115) !== 0,
        arm: u8(116) !== 0,
        up:  u32(117)
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
        angles: { roll: parseFloat(d.imu.r), pitch: parseFloat(d.imu.p) },
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
        el.innerText = 'CONNECTED';
        el.style.borderColor = 'var(--accent-color)';
        el.style.color = 'var(--accent-color)';
        el.style.backgroundColor = 'var(--accent-bg-glow)';
        if (linkEl) linkEl.innerText = 'OK';
    } else {
        el.innerText = 'DISCONNECTED';
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

        if(DOM.imuRoll)  DOM.imuRoll.innerText  = formatSensorValue(d.angles.roll);
        if(DOM.imuPitch) DOM.imuPitch.innerText = formatSensorValue(d.angles.pitch);
        if(DOM.imuTemp)  DOM.imuTemp.innerText  = d.temperature.toFixed(1);

        if(DOM.accelX) DOM.accelX.innerText = formatSensorValue(d.accel.x);
        if(DOM.accelY) DOM.accelY.innerText = formatSensorValue(d.accel.y);
        if(DOM.accelZ) DOM.accelZ.innerText = formatSensorValue(d.accel.z);

        if(DOM.gyroX) DOM.gyroX.innerText = formatSensorValue(d.gyro.x);
        if(DOM.gyroY) DOM.gyroY.innerText = formatSensorValue(d.gyro.y);
        if(DOM.gyroZ) DOM.gyroZ.innerText = formatSensorValue(d.gyro.z);
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
                gpsStatus.className = 'fix-badge fix-ok';
                gpsStatus.textContent = '3D FIX';
            }
            if(DOM.gpsLat)    DOM.gpsLat.innerText    = d.gps.lat.toFixed(6);
            if(DOM.gpsLng)    DOM.gpsLng.innerText    = d.gps.lng.toFixed(6);
            if(DOM.gpsAlt)    DOM.gpsAlt.innerText    = d.gps.alt.toFixed(1);
            if(DOM.gpsSats)   DOM.gpsSats.innerText   = d.gps.satellites;
            if(DOM.gpsSpeed)  DOM.gpsSpeed.innerText  = d.gps.speed.toFixed(1);
            if(DOM.gpsCourse) DOM.gpsCourse.innerText = d.gps.course.toFixed(1);
            if(DOM.gpsHdop)   DOM.gpsHdop.innerText   = d.gps.hdop.toFixed(2);
            if (d.gps.time && DOM.gpsTime) {
                DOM.gpsTime.innerText = d.gps.time;
            }
        } else {
            if(gpsStatus) {
                gpsStatus.className = 'fix-badge';
                gpsStatus.textContent = 'NO FIX';
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

    // ── Push de dados para os gráficos (apenas se o container estiver visível) ──
    if (d.valid) {
        if (sensorCharts.orientation && !document.getElementById('chart-orientation').hidden)
            sensorCharts.orientation.push([d.angles.roll, d.angles.pitch, d.temperature]);

        if (sensorCharts.accelerometer && !document.getElementById('chart-accelerometer').hidden)
            sensorCharts.accelerometer.push([d.accel.x, d.accel.y, d.accel.z]);

        if (sensorCharts.gyroscope && !document.getElementById('chart-gyroscope').hidden)
            sensorCharts.gyroscope.push([d.gyro.x, d.gyro.y, d.gyro.z]);
    }

    if (d.compass && d.compass.valid) {
        if (sensorCharts.compass && !document.getElementById('chart-compass').hidden)
            sensorCharts.compass.push([d.compass.heading, d.compass.x, d.compass.y, d.compass.z]);
    }

    if (d.gps && d.gps.valid) {
        if (sensorCharts.gps && !document.getElementById('chart-gps').hidden)
            sensorCharts.gps.push([d.gps.alt, d.gps.speed, d.gps.course, d.gps.satellites, d.gps.hdop]);
    }
}

function updateRadioFromData(d) {
    if (!d.raw_channels || !Array.isArray(d.raw_channels)) return;

    const container = DOM.channelsContainer;
    if (!container) return;

    if (container.innerHTML === 'Loading...' || container.innerHTML === '') {
        container.innerHTML = '<div class="ch-grid">' +
            d.raw_channels.map(function(val, i) {
                return '<div class="channel-group"><label>CH ' + (i+1) + '</label>' +
                    '<div class="channel-row"><div class="progress-bar"><div id="ch-' + i + '" class="fill" style="width:50%"></div></div>' +
                    '<span class="val" id="val-' + i + '">' + parseInt(val) + '</span></div></div>';
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

// Wrapper com timeout de 5s para todos os fetch() — evita hang indefinido se o ESP32 não responder.
function fetchWithTimeout(url, options) {
    var controller = new AbortController();
    var timer = setTimeout(function() { controller.abort(); }, 5000);
    var opts = Object.assign({}, options || {}, { signal: controller.signal });
    return fetch(url, opts).finally(function() { clearTimeout(timer); });
}

function updateSysInfo() {
    fetchWithTimeout('/api/sysinfo')
        .then(function(response) { return response.json(); })
        .then(function(data) {
            const list = document.getElementById('sysinfo-list');
            list.innerHTML =
                '<li><span class="label">Firmware</span> <span class="value">' + escapeHtml(data.firmware_version || '—') + '</span></li>' +
                '<li><span class="label">Chip Model</span> <span class="value">' + escapeHtml(data.chip_model) + ' (Rev ' + escapeHtml(data.chip_revision) + ')</span></li>' +
                '<li><span class="label">CPU Freq</span> <span class="value">' + escapeHtml(data.cpu_freq) + ' MHz</span></li>' +
                '<li><span class="label">RAM (Total)</span> <span class="value">' + formatBytes(data.heap_total) + '</span></li>' +
                '<li><span class="label">Flash (Total)</span> <span class="value">' + formatBytes(data.flash_size) + '</span></li>' +
                '<li><span class="label">Sketch Size</span> <span class="value">' + formatBytes(data.sketch_size) + '</span></li>' +
                '<li><span class="label">Uptime</span> <span class="value value-uptime">' + formatTime(data.uptime) + '</span></li>';

            const netList = document.getElementById('netinfo-list');
            let netHTML =
                '<li><span class="label">Mode</span> <span class="value">' + escapeHtml(data.mode) + '</span></li>' +
                '<li><span class="label">SSID</span> <span class="value">' + escapeHtml(data.ssid) + '</span></li>' +
                '<li><span class="label">IP Address</span> <span class="value">' + escapeHtml(data.ip) + '</span></li>' +
                '<li><span class="label">MAC Address</span> <span class="value">' + escapeHtml(data.mac) + '</span></li>' +
                '<li><span class="label">Channel</span> <span class="value">' + escapeHtml(data.channel) + '</span></li>';

            if (data.mode === 'Station') {
                netHTML += '<li><span class="label">Gateway</span> <span class="value">' + escapeHtml(data.gateway) + '</span></li>';
            } else {
                netHTML += '<li><span class="label">Clients</span> <span class="value">' + escapeHtml(data.clients) + '</span></li>';
            }
            netList.innerHTML = netHTML;
        })
        .catch(function() {});
}

function loadSettings() {
    fetchWithTimeout('/api/settings')
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
            refreshChartColors(); // re-resolve chart colors após tema inicial ser aplicado
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

        // Carrega Google Fonts apenas em modo STA (internet disponível).
        // Em modo AP o rover não tem acesso externo, usamos as fontes de fallback do CSS.
        if (d.wifi_mode === 1 && !document.querySelector('link[href*="fonts.googleapis.com"]')) {
            var pc1 = document.createElement('link'); pc1.rel = 'preconnect'; pc1.href = 'https://fonts.googleapis.com';
            var pc2 = document.createElement('link'); pc2.rel = 'preconnect'; pc2.href = 'https://fonts.gstatic.com'; pc2.crossOrigin = 'anonymous';
            var fl  = document.createElement('link'); fl.rel  = 'stylesheet'; fl.href = 'https://fonts.googleapis.com/css2?family=Rajdhani:wght@400;600;700&family=JetBrains+Mono:wght@400;600&display=swap';
            document.head.appendChild(pc1);
            document.head.appendChild(pc2);
            document.head.appendChild(fl);
        }
    })
    .catch(function() {});
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

    fetchWithTimeout('/api/settings', {
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
    .catch(function() {});
}

function showNvsPendingMsg() {
    const msg = document.getElementById('nvs-pending-msg');
    if (msg) msg.style.display = 'block';
}

function toggleTheme() {
    const enabled = document.getElementById('theme-toggle').checked;
    if(enabled) {
        document.body.setAttribute('data-theme', 'dark');
    } else {
        document.body.removeAttribute('data-theme');
    }
    refreshChartColors();
    fetchWithTimeout('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({dark_theme: enabled})
    }).then(function() { showNvsPendingMsg(); }).catch(console.error);
}

function toggleDebug() {
    const enabled = document.getElementById('debug-toggle').checked;
    fetchWithTimeout('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({debug: enabled})
    }).then(function() { showNvsPendingMsg(); }).catch(console.error);
}

function rebootSystem() {
    if (!confirm('Reboot the rover to save settings?')) return;
    fetchWithTimeout('/api/reboot', { method: 'POST' })
        .then(function(r) { return r.json(); })
        .then(function(d) {
            if (d.status === 'rebooting') {
                const msg = document.getElementById('nvs-pending-msg');
                if (msg) { msg.style.color = 'var(--primary)'; msg.textContent = 'Rebooting... reconnecting in 5s.'; }
                setTimeout(function() { location.reload(); }, 5000);
            }
        })
        .catch(console.error);
}

function updateLogs() {
    const logConsole = document.getElementById('log-console');
    if (!logConsole || !document.getElementById('config').classList.contains('active')) return;

    const autoRefresh = document.getElementById('auto-refresh-logs');
    if (autoRefresh && !autoRefresh.checked) return;

    fetchWithTimeout('/api/logs')
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

                html += '<div style="color: ' + color + '; font-weight: ' + weight + '; margin-bottom: 2px; border-bottom: 1px solid rgba(255,255,255,0.03);">' + escapeHtml(line) + '</div>';
            });

            if (logConsole.innerHTML !== html) {
                logConsole.innerHTML = html;
                logConsole.scrollTop = logConsole.scrollHeight;
            }
        })
        .catch(function() {});
}

function clearSystemLogs() {
    fetchWithTimeout('/api/clear-logs', { method: 'POST' })
        .then(function() {
            document.getElementById('log-console').innerHTML = '<div style="color: #6a9955">Console limpo.</div>';
        })
        .catch(function() {});
}

var _armedLockUntil = 0;

function toggleArmed() {
    const toggle = document.getElementById('armed-toggle');
    toggle.checked = !toggle.checked;
    const enabled = toggle.checked;
    _armedLockUntil = Date.now() + 1000;
    updateArmedStyle(enabled);
    fetchWithTimeout('/api/settings', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({armed: enabled})
    }).catch(console.error);
}

function updateArmedStyle(isArmed) {
    const btn = document.getElementById('armed-btn');
    if (!btn) return;
    btn.className = 'armed-btn ' + (isArmed ? 'armed-true' : 'armed-false');
    const label = document.getElementById('armed-toggle-label');
    if (label) label.textContent = isArmed ? 'ARMED' : 'DISARMED';
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

function formatSensorValue(v) {
    return (v >= 0 ? '+' : '') + v.toFixed(3);
}

let sysinfoPollingInterval = null;
let logsPollingInterval = null;

function startPolling() {
    if(sysinfoPollingInterval) clearInterval(sysinfoPollingInterval);
    if(logsPollingInterval) clearInterval(logsPollingInterval);

    sysinfoPollingInterval = setInterval(function() {
        if(document.getElementById('info').classList.contains('active')) {
            updateSysInfo();
        }
    }, 5000);

    logsPollingInterval = setInterval(function() {
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

    // Reset scroll after DOM repaints — rAF ensures layout is recalculated first.
    // All three resets needed: window covers Chrome/Android, body+documentElement cover iOS Safari.
    requestAnimationFrame(function() {
        window.scrollTo(0, 0);
        document.body.scrollTop = 0;
        document.documentElement.scrollTop = 0;
    });

    if (tabName === 'home') {
        initHUD();
    }
    if (tabName === 'info') {
        updateSysInfo();
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
        // Width table indexed by absI/10 (index 1..9 → 10°..90°)
        var pitchWidths = [0, 55, 65, 80, 68, 58, 52, 46, 42, 75];
        for (var i = -90; i <= 90; i += 10) {
            var absI = Math.abs(i);
            var g = document.createElementNS("http://www.w3.org/2000/svg", "g");

            if (i === 0) {
                // Horizon: wide split line, thicker, wider center gap
                g.innerHTML = '<path d="M -130 0 L -28 0 M 28 0 L 130 0" class="stroke-main" stroke-width="2.5"/>';
                pitchGroup.appendChild(g);
                continue;
            }

            // 10° increments: main pitch bars with T-bar end caps
            var yPos = i * 3;
            var w = pitchWidths[absI / 10] || 50;
            var labelSize = 17;

            if (i > 0) {
                // Nose up: solid lines with upward end caps
                g.innerHTML =
                    '<path d="M-' + w + ',' + (-yPos) + ' L-24,' + (-yPos) +
                    ' M24,' + (-yPos) + ' L' + w + ',' + (-yPos) +
                    ' M-' + w + ',' + (-yPos) + ' l0,-6' +
                    ' M' + w + ',' + (-yPos) + ' l0,-6" class="stroke-main" stroke-width="1.5"/>' +
                    '<text x="-' + (w + 12) + '" y="' + (-yPos + 4) + '" class="center-text-readout" font-size="' + labelSize + '" text-anchor="end">' + i + '</text>' +
                    '<text x="' + (w + 12) + '" y="' + (-yPos + 4) + '" class="center-text-readout" font-size="' + labelSize + '" text-anchor="start">' + i + '</text>';
            } else {
                // Nose down: dashed lines with downward end caps
                var absY = Math.abs(yPos);
                g.innerHTML =
                    '<path d="M-' + w + ',' + absY + ' L-24,' + absY +
                    ' M24,' + absY + ' L' + w + ',' + absY +
                    ' M-' + w + ',' + absY + ' l0,6' +
                    ' M' + w + ',' + absY + ' l0,6" class="stroke-main" stroke-dasharray="8 4" stroke-width="1.5"/>' +
                    '<text x="-' + (w + 12) + '" y="' + (absY + 4) + '" class="center-text-readout" font-size="' + labelSize + '" text-anchor="end">' + absI + '</text>' +
                    '<text x="' + (w + 12) + '" y="' + (absY + 4) + '" class="center-text-readout" font-size="' + labelSize + '" text-anchor="start">' + absI + '</text>';
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
                '<line x1="' + xPos + '" y1="25" x2="' + xPos + '" y2="35" class="stroke-main" stroke-width="1" />' +
                '<text x="' + xPos + '" y="55" class="center-text-readout" font-size="16" text-anchor="middle">' + h + '</text>';
            compassTapeEl.appendChild(mark);
        });

        // Populamos o cache após criação dos elementos
        DOM.compassTape = compassTapeEl;
    }

    // Cache do horizon-group (criado no HTML, não precisa esperar initHUD)
    DOM.horizonGroup = document.getElementById('horizon-group');
    DOM.rollPointer  = document.getElementById('roll-pointer');

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
            'translate(300px, 330px) rotate(' + roll + 'deg) translate(0px, ' + pitchY + 'px)';
    }
    // Roll pointer: polygon tip at SVG (300,125); CSS transform-box:view-box + origin:300px 300px
    if (DOM.rollPointer) {
        DOM.rollPointer.style.transform = 'rotate(' + roll + 'deg)';
    }

    if(DOM.hudPitch) DOM.hudPitch.textContent = 'P: ' + (pitch > 0 ? '+' : '') + pitch.toFixed(1) + '°';
    if(DOM.hudRoll)  DOM.hudRoll.textContent  = 'R: ' + (roll  > 0 ? '+' : '') + roll.toFixed(1)  + '°';

    const speedDash = Math.min((speed / 50) * 200, 200);
    document.getElementById('arc-speed').setAttribute('stroke-dasharray', speedDash + ' 200');
    document.getElementById('center-spd').textContent = data.gps.valid ? speed.toFixed(1) : '--';

    // CSS transform para compass tape — usa heading do compass externo (HMC5883L)
    const compassHeading = Math.max(0, Math.min(360, parseFloat(data.compass.heading) || 0));
    const compassValid   = data.compass.valid;
    const fullCircleWidth = 12 * 40;
    const yawOffset = -fullCircleWidth - ((compassHeading / 360) * fullCircleWidth);
    if (DOM.compassTape) {
        DOM.compassTape.style.transform = 'translateX(' + yawOffset + 'px)';
        DOM.compassTape.style.filter = compassValid ? '' : 'sepia(1) saturate(5) hue-rotate(-20deg)';
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
        if(DOM.hudGpsLat)  DOM.hudGpsLat.innerText  = data.gps.lat.toFixed(5) + '\xb0';
        if(DOM.hudGpsLng)  DOM.hudGpsLng.innerText  = data.gps.lng.toFixed(5) + '\xb0';
        if(DOM.hudGpsSats) DOM.hudGpsSats.innerText = data.gps.satellites;
        if(DOM.hudGpsHdop) DOM.hudGpsHdop.innerText = data.gps.hdop !== undefined ? data.gps.hdop.toFixed(1) : '--';
        if (data.gps.time && DOM.hudSysTimeLocal) {
            DOM.hudSysTimeLocal.innerText = data.gps.time;
        }
    }
    // Mini-map (always called — handles NO FIX state internally)
    var minimapHeading = (data.compass && data.compass.valid) ? data.compass.heading : (data.gps.course || 0);
    updateMiniMap(data.gps.lat || 0, data.gps.lng || 0, minimapHeading, data.gps.valid);

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
    DOM.hudGpsLat       = document.getElementById('hud-gps-lat');
    DOM.hudGpsLng       = document.getElementById('hud-gps-lng');
    DOM.hudGpsSats      = document.getElementById('hud-gps-sats');
    DOM.hudGpsHdop      = document.getElementById('hud-gps-hdop');
    DOM.hudSysUptime    = document.getElementById('hud-sys-uptime');
    DOM.hudSysTimeLocal = document.getElementById('hud-sys-time-local');
    DOM.hudGpsFixBadge  = document.getElementById('hud-gps-fix-badge');
    minimapCanvas = document.getElementById('hud-minimap');
    if (minimapCanvas) minimapCtx = minimapCanvas.getContext('2d');
    var zmIn  = document.getElementById('minimap-zoom-in');
    var zmOut = document.getElementById('minimap-zoom-out');
    if (zmIn)  zmIn.onclick  = function() { if (minimapZoomDelta < 3)  { minimapZoomDelta++; minimapTiles = {}; minimapGridKey = ''; } };
    if (zmOut) zmOut.onclick = function() { if (minimapZoomDelta > -3) { minimapZoomDelta--; minimapTiles = {}; minimapGridKey = ''; } };
    var btnMap = document.getElementById('map-btn-map');
    var btnSat = document.getElementById('map-btn-sat');
    if (btnMap) btnMap.onclick = function() { if (minimapSatMode) { minimapSatMode = false; btnMap.classList.add('active'); btnSat.classList.remove('active'); minimapTiles = {}; minimapGridKey = ''; } };
    if (btnSat) btnSat.onclick = function() { if (!minimapSatMode) { minimapSatMode = true; btnSat.classList.add('active'); btnMap.classList.remove('active'); minimapTiles = {}; minimapGridKey = ''; } };

    // HUD SVG animated elements (populados aqui; compassTape pode ser null até initHUD)
    DOM.horizonGroup = document.getElementById('horizon-group');

    // Sensors Tab Elements
    DOM.imuRoll    = document.getElementById('imu-roll');
    DOM.imuPitch   = document.getElementById('imu-pitch');
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

    // Header/System Elements
    DOM.armedToggle      = document.getElementById('armed-toggle');
    DOM.armedBtn         = document.getElementById('armed-btn');
    DOM.sysinfoList      = document.getElementById('sysinfo-list');
    DOM.netinfoList      = document.getElementById('netinfo-list');
    DOM.connectionStatus = document.getElementById('connectionStatus');

    initHUD(); // HUD está na aba Home (ativa por padrão)
    initSensorCharts(); // Gráficos de sensor com toggle e legenda interativa
    loadSettings();
    startPolling();
    startRenderLoop();      // Fase 2: inicia loop RAF desacoplado
    connectWebSocket();     // Inicia conexão push realtime via WebSocket
});

window.addEventListener('beforeunload', function() {
    if (rafId !== null) {
        cancelAnimationFrame(rafId);
        rafId = null;
    }
    if (sysinfoPollingInterval) { clearInterval(sysinfoPollingInterval); sysinfoPollingInterval = null; }
    if (logsPollingInterval)    { clearInterval(logsPollingInterval);    logsPollingInterval    = null; }
    if (ws) ws.close();
});

