# Design System: UGV COMMAND SYSTEM (GCS)
*Extracted from: `web_src/style.css`, `web_src/index.html`, `web_src/script.js` on 2026-04-01*

---

## Summary

A **dual-theme tactical HUD interface** for an ESP32-based ground control station (GCS). The design language is deliberately militaristic — inspired by fighter jet MFDs (Multi-Function Displays) and FPV drone ground stations. Two fully-realized themes coexist:

- **Light ("Field Command")**: F-35 daylight MFD aesthetic — amber/orange on slate-grey.
- **Dark ("Phantom Ops")**: Night-vision phosphor-green on near-black.

The system is **fully token-based** via CSS Custom Properties. All colors, spacing, and surface values are defined at `:root` / `[data-theme="dark"]` — there are very few magic numbers. Typography is intentionally split between a condensed sans-serif (`Rajdhani` / `Arial Narrow`) for labels and UI chrome, and a monospace (`JetBrains Mono` / `Share Tech Mono`) for readout values. The HUD viewport declares its own isolated token layer (`--primary`, `--glow`, `--fpv-aberration`) fixed to phosphor-green, independent of the app theme. All `border-radius` values are `0` — sharp military/technical aesthetic throughout.

---

## 1. Design Tokens

### 1.1 Colors

#### Light Theme (`:root` — "Field Command")

| Token | Value | Semantic Role |
|-------|-------|---------------|
| `--base-bg` | `#c8d0d8` | App background — slate grey |
| `--card-bg` | `rgba(198, 210, 220, 0.97)` | Card / surface background |
| `--card-border` | `rgba(160, 90, 10, 0.22)` | Card border — amber tint |
| `--text-main` | `#1a1208` | Primary text — near-black warm |
| `--text-muted` | `#60504a` | Secondary/muted text — warm brown |
| `--accent-color` | `#c87800` | Primary accent — amber/orange |
| `--accent-rgb` | `200, 120, 0` | Accent as raw RGB (for rgba usage) |
| `--accent-hover` | `#a86200` | Accent hover state — deeper amber |
| `--accent-text` | `#ffffff` | Text on accent backgrounds |
| `--accent-bg-glow` | `rgba(200, 120, 0, 0.12)` | Accent glow / subtle fill |
| `--watermelon` | `#c41828` | Danger / armed / error — red |
| `--watermelon-rgb` | `196, 24, 40` | Danger as raw RGB |
| `--sapphire` | `#0055aa` | Throttle/steering channel highlight — blue |
| `--iron-grey` | `#8a8070` | Neutral border / unit text |
| `--warning` | `#b06000` | Warning state — dark amber |
| `--switch-bg` | `rgba(0, 0, 0, 0.12)` | Toggle switch track — off state |
| `--switch-thumb` | `#ffffff` | Toggle switch thumb |
| `--header-border` | `rgba(160, 90, 10, 0.18)` | Header / frame dividers |
| `--list-border` | `rgba(160, 90, 10, 0.1)` | List item separators |
| `--input-bg` | `rgba(255, 255, 255, 0.75)` | Input field background |
| `--input-border` | `rgba(160, 90, 10, 0.28)` | Input field border |

#### Dark Theme (`[data-theme="dark"]` — "Phantom Ops")

| Token | Value | Semantic Role |
|-------|-------|---------------|
| `--base-bg` | `#060810` | App background — near-black navy |
| `--card-bg` | `rgba(12, 19, 32, 0.96)` | Card / surface background |
| `--card-border` | `rgba(0, 255, 157, 0.15)` | Card border — phosphor green tint |
| `--text-main` | `#c8e8d8` | Primary text — pale phosphor green |
| `--text-muted` | `#3a6050` | Secondary/muted text — dark teal |
| `--accent-color` | `#00ff9d` | Primary accent — neon phosphor green |
| `--accent-rgb` | `0, 255, 157` | Accent as raw RGB |
| `--accent-hover` | `#00d985` | Accent hover — slightly dimmer green |
| `--accent-text` | `#060810` | Text on accent backgrounds — near-black |
| `--accent-bg-glow` | `rgba(0, 255, 157, 0.1)` | Accent glow fill |
| `--watermelon` | `#ff2040` | Danger / armed / error — bright red |
| `--watermelon-rgb` | `255, 32, 64` | Danger as raw RGB |
| `--sapphire` | `#00b8e6` | Throttle/steering channel highlight — cyan |
| `--iron-grey` | `#1a3828` | Neutral border / unit text — dark teal |
| `--warning` | `#ffb300` | Warning state — amber |
| `--switch-bg` | `rgba(0, 0, 0, 0.6)` | Toggle switch track — off state |
| `--switch-thumb` | `#3a6050` | Toggle switch thumb — teal |
| `--header-border` | `rgba(0, 255, 157, 0.12)` | Header / frame dividers |
| `--list-border` | `rgba(0, 255, 157, 0.06)` | List item separators |
| `--input-bg` | `rgba(0, 0, 0, 0.45)` | Input field background |
| `--input-border` | `rgba(0, 255, 157, 0.2)` | Input field border |

#### HUD Viewport Isolated Tokens (`.hud-viewport` — fixed, theme-independent)

| Token | Value | Semantic Role |
|-------|-------|---------------|
| `--primary` | `#00e87a` | HUD phosphor green — all SVG strokes, bars, overlays |
| `--primary-dim` | `rgba(0, 232, 122, 0.2)` | Dimmed HUD elements |
| `--danger` | `var(--watermelon)` | Critical tilt, armed state (maps to theme's watermelon) |
| `--warning` | `#ffb300` | Battery / power bar color |
| `--text-dim` | `rgba(224, 248, 245, 0.65)` | HUD dim text |
| `--glow` | `0 0 5px #00e87a, 0 0 12px rgba(0, 232, 122, 0.35)` | Phosphor glow shadow |
| `--fpv-aberration` | `0.6px 0px 0px rgba(255,0,0,0.3), -0.6px 0px 0px rgba(0,255,255,0.3), ...` | CRT chromatic aberration text-shadow |

#### Chart Series Palette (Sensors Tab — multi-variable line charts)

15 tokens (`--chart-color-1` … `--chart-color-15`) defined in `:root` and `[data-theme="dark"]`. Colors 1–4 reuse semantic tokens; 5–15 are dedicated chart additions. Used for multi-variable sensor charts and channel color assignments.

| Token | Light (Field Command) | Dark (Phantom Ops) |
|-------|-----------------------|--------------------|
| `--chart-color-1`  | `#0055aa` — sapphire blue           | `#00b8e6` — cyan                  |
| `--chart-color-2`  | `#c41828` — watermelon red          | `#ff2040` — bright red            |
| `--chart-color-3`  | `#c87800` — amber                   | `#00ff9d` — neon green            |
| `--chart-color-4`  | `#b06000` — dark amber              | `#ffb300` — amber                 |
| `--chart-color-5`  | `#1a7a4a` — military green          | `#c084fc` — neon purple           |
| `--chart-color-6`  | `#6b21a8` — deep purple             | `#fb923c` — neon orange           |
| `--chart-color-7`  | `#0a7878` — deep teal               | `#00e5d4` — electric teal         |
| `--chart-color-8`  | `#c05010` — burnt orange            | `#ff7048` — neon coral            |
| `--chart-color-9`  | `#7a7800` — military olive          | `#dfff00` — neon yellow           |
| `--chart-color-10` | `#1a6aa0` — steel blue              | `#60a8ff` — periwinkle            |
| `--chart-color-11` | `#8c1068` — deep magenta            | `#ff40c8` — hot pink              |
| `--chart-color-12` | `#3a7200` — forest lime             | `#80ff40` — neon lime             |
| `--chart-color-13` | `#8a6000` — military gold           | `#ffd040` — bright gold           |
| `--chart-color-14` | `#780820` — deep maroon             | `#ff6080` — soft crimson          |
| `--chart-color-15` | `#4a1890` — deep violet             | `#9860ff` — electric violet       |

RGB companion vars (`--chart-color-5-rgb`, `--chart-color-6-rgb`) are provided for `rgba()` usage. The existing `--accent-rgb`, `--watermelon-rgb`, and `--sapphire` cover colors 1–4.

#### Hardcoded Colors (outside token system)

| Value | Location | Usage |
|-------|----------|-------|
| `#020604` | `#log-console`, `#boot-screen` | Console/boot terminal background |
| `#00e87a` | `#log-console` color | Console text |
| `#060810` | HUD background gradient | HUD CRT dark fill |
| `#040608`, `#020406`, `#000000` | HUD radial gradient | CRT depth layers |

---

### 1.2 Typography

#### Font Families

| Role | Primary | Fallback |
|------|---------|---------|
| UI / Labels / Headings | `'Rajdhani'` | `'Arial Narrow'`, `Arial`, `sans-serif` |
| Data readouts / Mono | `'JetBrains Mono'` | `'Courier New'`, `monospace` |
| HUD SVG / Boot console | `'Share Tech Mono'` (embedded `@font-face`) | `'Courier New'`, `monospace` |

> **Note**: Both Rajdhani and JetBrains Mono are referenced as CDN fonts but a comment in the HTML notes they were removed because the rover operates in AP mode (no internet). The CSS uses system fallbacks. Share Tech Mono is embedded as a base64 `@font-face`.

#### Font Sizes

| Context | Size | Usage |
|---------|------|-------|
| `h1` | `1.75rem` | Main page title |
| `h1` (mobile) | `1.4rem` | Main title at ≤600px |
| Card `h3` label | `0.72rem` | Card section headers |
| Tab button | `0.83rem` | Navigation tabs |
| Tab button (mobile) | `0.75rem` | Tabs at ≤600px |
| `.header-tag` | `0.68rem` | "UGV" callsign tag |
| `.fw-version` | `0.58rem` | Firmware version subtitle |
| `.connection-status` | `0.7rem` | Connection badge |
| `.armed-btn` | `0.7rem` | Armed/Disarmed toggle |
| `.sensor-row .label` | `0.8rem` | Sensor row labels |
| `.sensor-row .value` | `0.95rem` | Sensor data values |
| `li span.label` | `0.8rem` | List labels |
| `li span.value` | `0.88rem` | List values |
| `.channel-group label` | `0.72rem` | RC channel labels |
| `#log-console` | `0.82rem` | System log console |
| `.fix-badge` | `0.58rem` | GPS fix status badge |
| `.hud-frame-bar` | `0.58rem` | HUD / map frame title bars |
| HUD SVG readout | `14–31px` | SVG text elements in HUD |
| Critical warn msg | `20px` | HUD critical tilt message |

#### Font Weights

| Weight | Usage |
|--------|-------|
| 700 (Bold) | `h1`, `h3`, `.tab-btn`, `.btn-primary`, `.header-tag`, `.armed-btn`, `.panel-header` |
| 600 (SemiBold) | `.sensor-row .value`, `li span.value`, channel values, `.fw-version`, `.connection-status` |
| Normal | Body text, `.fw-version` |

#### Letter Spacing

| Value | Usage |
|-------|-------|
| `5px` | `.header-tag` |
| `3px` | `h1`, `h3` card headers |
| `2px` | Tabs, `.connection-status`, `.armed-btn`, `.channel-group label`, buttons |
| `1.5px` | Panel headers (mobile) |
| `1px` | `.sensor-row .label`, list labels |

#### Line Height

- `1` — `h1`, `.data-row`
- `1.1` — `.data-row` (mobile)
- `1.5` — `#log-console`
- `font-variant-numeric: tabular-nums` — HUD viewport (numeric alignment)

---

### 1.3 Spacing Scale

The spacing is not based on a strict systematic scale (no Tailwind-style `4px` grid), but recurring values are:

| Value | Used in |
|-------|---------|
| `2px` | Bar fills, gap between grid items, fine borders |
| `3px` | Gaps, minor padding, card bracket border-width |
| `4px` | Small gaps, bar heights, margin |
| `5px` | Header letter-spacing, panel padding, frame bar padding |
| `6px` | Small gaps, badge padding |
| `8px` | Gaps, padding, `minimap-zoom-ctrl` top/right offset |
| `10px` | Panel offset, tab padding |
| `12px` | Padding (cards mobile, inputs, buttons), gap |
| `14px` | Frame bar horizontal padding, list item padding-y |
| `16px` | Card padding (mobile), dashboard gap, header margin |
| `18px` | Channel group margin, list padding |
| `20px` | Card padding, dashboard gap, margin |
| `22px` | Card padding (desktop) |
| `24px` | App container padding, tab margin |
| `28px` | Header margin-bottom |
| `40px` | Boot screen padding |

**Pattern**: Loosely follows a base-4 rhythm (4, 8, 12, 16, 20, 24) for major structural spacing, with finer 2/3px increments for internal micro-spacing.

---

### 1.4 Border Radius

All UI elements use `border-radius: 0` — the design explicitly enforces sharp/square corners throughout as part of the military/tactical aesthetic.

**Exceptions:**
- `.slider.round` / `.slider.round:before` — `34px` / `50%` (pill-shaped toggle, kept round by deliberate override)
- `map-layer-toggle` — `3px` (subtle rounding on internal map layer toggle)
- `map-layer-btn` — `2px`
- `minimap-zoom-btn` — `0` (explicit)

**Design language**: Sharp, angular, zero-radius everywhere except the toggle slider pill. This is a deliberate "no rounded corners" system.

---

### 1.5 Shadows / Elevation

| Token / Value | Usage |
|---------------|-------|
| `0 0 8px rgba(var(--accent-rgb), 0.6)` | Header accent line glow |
| `0 0 10px rgba(var(--accent-rgb), 0.15)` | `.connection-status` ambient glow |
| `0 0 12px rgba(var(--accent-rgb), 0.2)` | `.btn-primary` default shadow |
| `0 0 20px rgba(var(--accent-rgb), 0.4)` | `.btn-primary:hover` elevated glow |
| `0 0 8px rgba(var(--accent-rgb), 0.8)` | Toggle thumb active (checked) glow |
| `0 0 8px rgba(var(--watermelon-rgb), 0.25)` | Armed button glow (armed state) |
| `0 0 8px var(--accent-bg-glow)` | Progress/channel bar fill glow |
| `0 0 5px #00e87a, 0 0 12px rgba(0,232,122,0.35)` | HUD `--glow` (bar fills, SVG glow) |
| `inset 0 0 100px rgba(0,0,0,0.8)` | HUD frame vignette |
| `inset 0 0 30px rgba(0,0,0,0.8)` | Log console inset shadow |
| `0 1px 4px rgba(0,0,0,0.45)` | Minimap zoom buttons |
| SVG `filter: drop-shadow(...)` | HUD SVG phosphor glow (multi-layer) |

**Elevation system**: Glow-based rather than layer-based. No `box-shadow` offset values — all shadows are centered glows using the accent or danger colors, reinforcing the phosphor/neon aesthetic.

---

### 1.6 Breakpoints / Grid

| Name | Condition | Notes |
|------|-----------|-------|
| Desktop | > 960px | Default: 2-column home layout |
| Tablet | `max-width: 960px` and `min-height: 600px` | HUD flex column layout |
| Mobile | `max-width: 680px` | Home layout stacks to 1 column, HUD/map use `aspect-ratio: 1/1.5` |
| Small Mobile | `max-width: 600px` | Reduced padding, font sizes, tabs wrap to 3+2 grid |
| Landscape Phone | `max-height: 550px` | Height-based breakpoint for landscape orientation |

**Grid**: `dashboard-grid` uses `grid-template-columns: repeat(auto-fit, minmax(280px, 1fr))` — fluid auto-fit grid with 280px column minimum. Gap: `20px` (desktop), `14px` (mobile).

**Home Layout**: `grid-template-columns: 1fr 1fr` with `height: 80vh` — fixed-height side-by-side HUD and map panels.

---

### 1.7 Motion / Transitions

| Duration | Easing | Usage |
|----------|--------|-------|
| `0.1s linear` | linear | Bar fills (real-time data) |
| `0.2s ease` | ease | Tab buttons, SVG stroke color, HUD elements |
| `0.25s` | default | Armed button border/color |
| `0.3s ease` | ease | Cards, sensor rows, input focus, tab fade |
| `0.4s ease` | ease | Theme toggle — all global theme properties |
| `0.5s` | `cubic-bezier(0.1, 1, 0.3, 1)` | HUD container reveal (opacity + scale) |

**Animations:**
| Name | Duration | Usage |
|------|----------|-------|
| `fadeIn` | `0.3s ease` | Tab content entry (translateY(4px) → 0) |
| `armed-pulse` | `1.2s ease-in-out infinite` | Armed dot opacity pulse |
| `pulse-danger` | `1.5s infinite` | Status badge danger flicker |
| `blink-fast` | `0.3s infinite alternate` | Critical tilt warning |
| `crt-flicker` | `11s infinite` | CRT viewport subtle flicker |

---

## 2. CSS Variables Index

### `:root` (Light Theme)

```
--accent-bg-glow:    rgba(200, 120, 0, 0.12)
--accent-color:      #c87800
--accent-hover:      #a86200
--accent-rgb:        200, 120, 0
--accent-text:       #ffffff
--base-bg:           #c8d0d8
--black:             var(--base-bg)
--card-bg:           rgba(198, 210, 220, 0.97)
--card-border:       rgba(160, 90, 10, 0.22)
--header-border:     rgba(160, 90, 10, 0.18)
--input-bg:          rgba(255, 255, 255, 0.75)
--input-border:      rgba(160, 90, 10, 0.28)
--iron-grey:         #8a8070
--list-border:       rgba(160, 90, 10, 0.1)
--neon-chartreuse:   var(--accent-color)
--sapphire:          #0055aa
--switch-bg:         rgba(0, 0, 0, 0.12)
--switch-thumb:      #ffffff
--text-main:         #1a1208
--text-muted:        #60504a
--warning:           #b06000
--watermelon:        #c41828
--watermelon-rgb:    196, 24, 40
```

### `[data-theme="dark"]` (Dark Theme)

```
--accent-bg-glow:    rgba(0, 255, 157, 0.1)
--accent-color:      #00ff9d
--accent-hover:      #00d985
--accent-rgb:        0, 255, 157
--accent-text:       #060810
--base-bg:           #060810
--black:             var(--base-bg)
--card-bg:           rgba(12, 19, 32, 0.96)
--card-border:       rgba(0, 255, 157, 0.15)
--header-border:     rgba(0, 255, 157, 0.12)
--input-bg:          rgba(0, 0, 0, 0.45)
--input-border:      rgba(0, 255, 157, 0.2)
--iron-grey:         #1a3828
--list-border:       rgba(0, 255, 157, 0.06)
--neon-chartreuse:   var(--accent-color)
--sapphire:          #00b8e6
--switch-bg:         rgba(0, 0, 0, 0.6)
--switch-thumb:      #3a6050
--text-main:         #c8e8d8
--text-muted:        #3a6050
--warning:           #ffb300
--watermelon:        #ff2040
--watermelon-rgb:    255, 32, 64
```

### `.hud-viewport` (HUD Isolated Scope)

```
--danger:            var(--watermelon)
--font-mono:         'Share Tech Mono', 'Courier New', monospace
--fpv-aberration:    0.6px 0px 0px rgba(255,0,0,0.3), -0.6px 0px 0px rgba(0,255,255,0.3), 0 0 2px rgba(0,0,0,0.8)
--glow:              0 0 5px #00e87a, 0 0 12px rgba(0, 232, 122, 0.35)
--primary:           #00e87a
--primary-dim:       rgba(0, 232, 122, 0.2)
--text-dim:          rgba(224, 248, 245, 0.65)
--warning:           #ffb300
```

---

## 3. Component Inventory

| Component | Detected via | Class / ID examples | Notes |
|-----------|-------------|---------------------|-------|
| App Container | `.app-container` | `max-width: 900px`, centered | Layout wrapper |
| Header | `header` | `.header-title`, `.header-callsign`, `.header-controls` | Title + armed btn + connection status |
| Navigation Tabs | `.tabs` / `.tab-btn` | `active` state, 5 tabs | No border-radius; phosphor glow underline |
| Cards | `.card` | `.dashboard-grid` children | Corner bracket pseudo-elements |
| Sensor Rows | `.sensor-row` | `.label`, `.value`, `.unit` | Two-column label/value layout |
| Progress Bar | `.progress-bar` / `.fill` | RC channel bars (horizontal) | Center tick at 1500µs midpoint |
| Vertical Bar | `.vbar` / `.vbar-fill` | `.ch-vertical`, `#ch-0`, `#ch-2` | RC channels in new layout |
| Channel Grid | `.ch-grid` | `grid-template-columns: 1fr 1fr` | 2-col on desktop, 1-col mobile |
| Toggle Switch | `.switch` / `.slider` | `#theme-toggle`, `#debug-toggle` | Pill variant via `.round` modifier |
| Primary Button | `.btn-primary` | Save/Reboot, Clear Console | Accent-colored, uppercase, 0 radius |
| Custom Input | `.custom-input` | `#sta-ssid`, `#sta-pass`, `#wifi-mode` | Monospace font, accent focus ring |
| Armed Button | `.armed-btn` | `#armed-btn`, `.armed-dot`, `.armed-label` | Two states: `.armed-false` / `.armed-true` |
| Connection Badge | `.connection-status` | `#connectionStatus` | Outline style, accent color |
| Fix Badge | `.fix-badge` | `#gps-status`, `#hud-gps-fix-badge` | GPS fix status; `.fix-ok` variant |
| Status Badge | `.status-badge` | `#hud-sys-state` | Danger pulsing; `.disarmed` variant |
| HUD Frame | `.hud-frame` | CRT background, `border: 1px solid var(--card-border)` | CRT radial gradient background |
| HUD Viewport | `.hud-viewport` | `#main-hud` wrapper | Isolated token scope, CRT effects |
| Side Panels | `.side-panel` | `.panel-left/right/top-left/top-right` | Overlay panels on HUD |
| HUD SVG Center | `.svg-center` | Roll arc, compass tape, speed/power bars | `clamp()` responsive sizing |
| Boot Screen | `#boot-screen` | `.boot-line` elements | Sequential fade-in terminal animation |
| Map Frame | `.map-frame` | `.map-frame-bar`, `.map-viewport` | Theme-aware; no CRT effects |
| Minimap Canvas | `#hud-minimap` | `canvas` | GPS track canvas |
| Zoom Controls | `.minimap-zoom-ctrl` | `.minimap-zoom-btn`, `.minimap-layer-btn` | MAP/SAT layer toggle |
| Log Console | `#log-console` | Dark terminal; `#020604` bg, `#00e87a` text | Hardcoded terminal colors (not tokens) |
| Bar Indicators | `.bar-bg` / `.bar-fill` | `#hud-bar-mot-l/r`, `#hud-bar-slope` | Motor and slope progress bars |
| Data Row | `.data-row` | `.data-label`, `.data-value`, `.data-value-small` | HUD panel data pairs |
| Setting Item | `.setting-item` | Config tab, flex row with label + control | |
| Dashboard Grid | `.dashboard-grid` | Auto-fit minmax grid | Fluid column layout |
| IMU Offline Alert | `#imu-offline` | `border-left: 4px solid var(--watermelon)` | Left-border danger callout |

---

## 4. Naming Conventions

**Component-scoped prefixes** are the primary convention:

- `hud-*` — HUD-specific elements (`#hud-frame`, `#hud-viewport`, `#hud-minimap`, `#hud-sys-state`)
- `map-*` / `minimap-*` — Map panel and canvas elements
- `panel-*` — HUD overlay panel positioning variants
- `armed-*` — Armed state button sub-elements
- `data-*` — HUD panel data display elements
- `ch-*` / `vbar*` — RC channel bar components
- `btn-*` — Button variants (`.btn-primary`)
- `stroke-*` — SVG stroke style variants (`.stroke-main`, `.stroke-dim`, `.stroke-danger`)
- `center-text-*` — HUD SVG text style classes
- `fix-*` — GPS fix badge states
- `accel-*` — Accelerometer data display

**State modifiers** use the value directly as a modifier class:
- `.armed-true` / `.armed-false`
- `.fix-ok`
- `.active` (tabs, map layer buttons)
- `.disarmed` (status badge)
- `.critical-mode` (HUD viewport)

**Pattern**: A hybrid of component-scoped BEM-lite prefixes and semantic state classes. Not strict BEM (no `__element` notation), but consistent. IDs are used freely for unique live-data targets (`#hud-gps-lat`, `#hud-mot-l`, etc.).

---

## 5. Quality Assessment

| Dimension | Score | Reasoning |
|-----------|-------|-----------|
| **Token Coverage** | 5/5 | Near-complete. All theme colors, surfaces, borders, and states are CSS variables. Two fully parallel theme token sets. HUD has its own isolated scope. Very few magic numbers — only the log console and boot screen background are hardcoded. |
| **Naming Consistency** | 4/5 | Prefix conventions are consistent and predictable. Minor inconsistency: `--black` and `--neon-chartreuse` are alias tokens (JS compatibility) that shadow the naming scheme. No unified spacing scale tokens. |
| **Accessibility Signals** | 3/5 | Focus styles exist (`input:focus + .slider`, `.custom-input:focus`). ARIA: minimal (`title` on the armed button). No explicit `prefers-reduced-motion` handling despite heavy animation use. Contrast ratios not formally audited — dark theme neon green on black is high contrast; light theme amber on slate likely passes AA. |
| **Component Completeness** | 5/5 | 25+ distinct component patterns covering navigation, data display, controls, HUD overlays, maps, alerts, and interactive inputs. Very complete for a specialized telemetry app. |
| **Overall Score** | **4.25/5** | Exceptionally well-structured for an embedded firmware project. Strong token architecture, coherent military aesthetic, thoughtful dual-theme implementation. |

---

*Key observation*: The dual-theme token architecture is unusually mature for an ESP32 firmware project — the system rivals production web apps in token discipline. The HUD's isolated CSS scope (`.hud-viewport` re-declaring `--primary`) is a sophisticated pattern that decouples the CRT display from the ambient UI theme cleanly.
