# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Development Commands

```bash
# Build firmware (runs web asset pipeline automatically first)
pio run

# Upload firmware to ESP32
pio run -t upload

# Upload + Monitor serial output (115200 baud)
pio run -t upload && pio device monitor

# Monitor serial output only
pio device monitor

# Clean build artifacts
pio run -t clean

# Re-generate web_pages.h from web_src/ (standalone, no build required)
python3 tools/build_web_impl.py
```

**Note**: Upload speed is configured at 921600 baud in `platformio.ini`. Serial monitor runs at 115200 baud.

---

## Web UI Development

**`src/web/web_pages.h` is auto-generated — never edit it directly.**

The build pipeline (`tools/build_web_impl.py`) runs automatically before every `pio run`. It:
1. Reads source files from `web_src/` (HTML, CSS, JS)
2. Minifies and gzip-compresses them
3. Embeds the result as C arrays in `web_pages.h`

To modify the web interface, edit files in `web_src/`. Run `python3 tools/build_web_impl.py` standalone for quick iteration without a full firmware build.

---

## Web UI Design System (MANDATORY)

**All changes and new implementations in `web_src/` MUST adhere to the design system documented in `design-system/`.**

The reference files are:
- `design-system/design-system.md` — Full human-readable specification (colors, typography, spacing, components, naming conventions)
- `design-system/tokens.json` — W3C DTCG token file (machine-readable)

### Non-negotiable rules

1. **CSS Variables only** — Never hardcode color, border, or surface values. Always use the existing `--token-name` variables defined in `:root` and `[data-theme="dark"]`. Both themes must work correctly after any change.

2. **Zero border-radius** — All new UI elements must use `border-radius: 0`. The only exceptions already in the codebase are the pill toggle (`.slider.round`) and map layer buttons — do not add new exceptions without explicit justification.

3. **Typography** — Use only the three established font families:
   - `'Rajdhani', 'Arial Narrow', Arial, sans-serif` → labels, headings, tabs, buttons
   - `'JetBrains Mono', 'Courier New', monospace` → data readouts, inputs, badges
   - `'Share Tech Mono', 'Courier New', monospace` → HUD SVG text and terminal only

4. **Glow-based shadows** — Elevation is expressed as centered glows using `var(--accent-color)`, `var(--watermelon)`, or HUD `--primary`. No offset drop shadows on new elements.

5. **Component naming** — Follow the existing prefix conventions:
   - `hud-*` for HUD elements, `map-*`/`minimap-*` for map, `data-*` for HUD panel pairs, `btn-*` for buttons, `stroke-*` for SVG stroke classes. State modifiers use the value directly (e.g., `.armed-true`, `.fix-ok`, `.active`).

6. **HUD isolation** — The `.hud-viewport` scope has its own token layer (`--primary`, `--glow`, `--fpv-aberration`, etc.) fixed to phosphor-green, independent of the app theme. HUD internals must use these local tokens, not the app-level `--accent-color`.

7. **Spacing** — Follow the existing base-4 rhythm (4, 8, 12, 16, 20, 24 px) for structural spacing. Fine internal spacing uses 2–3 px increments. Do not introduce arbitrary values outside the scale in `design-system/tokens.json`.

8. **New components** — Before adding a new UI pattern, check the Component Inventory in `design-system/design-system.md` (Section 3). Reuse existing patterns (`.card`, `.sensor-row`, `.data-row`, `.btn-primary`, etc.) before creating new ones.

---

## Architecture Overview

This is an **ESP32-based Tank Robot Controller** using **differential drive** (left + right motor control via throttle + steering mixing).

### Core Data Flow

```
RC Receiver (iBUS)
    ↓
ChannelManager (reads 10 iBUS channels)
    ↓
TankController (main orchestrator & state machine)
    ↓ (with IMU/GPS/Compass sensor data)
    ↓
MotorController (mixing, deadband compensation, arming)
    ↓
PWM Output → ESCs → Motors
    ↓
WebServerManager (telemetry over WiFi)
StatusLedManager (visual feedback)
```

### Dual-Core FreeRTOS Architecture

- **Core 0**: `webServerTask` — Wi-Fi, async web server (non-blocking)
- **Core 1**: `tankControlTask` — Real-time control loop at 50 Hz (strict timing via `vTaskDelayUntil`)

---

## Project Structure

```
src/
├── main.cpp                      # FreeRTOS task setup, mutex protection
├── config/
│   ├── config.h                  # System constants (timeouts, loop rates, PWM limits)
│   ├── config.cpp                # Config implementations
│   └── pins.h                    # GPIO pin mappings (ESC, iBUS, IMU, etc.)
├── types/
│   ├── types.h                   # Data structures: SystemState, SensorData, MotorCommand
│   └── types.cpp                 # Constructor implementations
├── controllers/
│   ├── tank_controller.h/cpp     # Main coordinator (state machine, sensor integration)
│   └── motor_controller.h/cpp    # ESC control (mixing, deadband, arming)
├── communication/
│   └── channel_manager.h/cpp     # iBUS protocol reading, stick normalization, timeout detection
├── sensors/
│   ├── imu_sensor.h/cpp          # IMU (accel/gyro) via I2C
│   ├── compass_sensor.h/cpp      # Magnetometer
│   └── gps_sensor.h/cpp          # GPS via UART serial
├── utils/
│   ├── status_led_manager.h/cpp  # LED blink patterns based on SystemState
│   ├── kalman_filter.h           # Sensor filtering (unused, for reference)
│   └── utils.h/cpp               # Math: stick normalization, PWM conversion
├── web/
│   ├── web_server_manager.h/cpp  # Web telemetry server (AsyncWebServer)
│   └── web_pages.h               # HTML/CSS/JS embedded in firmware
└── debug/
    └── debug_manager.h/cpp       # Timed console output (avoids log flooding)
```

---

## Key Classes & Responsibilities

### **TankController** (`tank_controller.h/cpp`)
Central coordinator. Manages:
- **State Machine**: `INITIALIZING → ARMING → ARMED` (or `TIMEOUT`/`ERROR`)
- Sensor updates (IMU, GPS, Compass)
- Motor commands via `MotorController`
- Web telemetry via `WebServerManager`
- Status LED feedback via `StatusLedManager`
- **Key methods**:
  - `initialize()` — Setup all sub-systems
  - `update()` — Main non-blocking loop (called at 50 Hz)
  - `setSystemArmed(bool)` / `isSystemArmed()` — Arming state control

### **ChannelManager** (`communication/channel_manager.h/cpp`)
Reads iBUS protocol (typically 10 channels from RC receiver).
- Normalizes raw PWM (1000–2000 µs) → `-1.0` to `1.0` float range
- Detects timeout (>400 ms without signal)
- Applies deadzone around center (1500 µs) to eliminate stick jitter
- **Key data**: `throttle`, `steering`, raw channel array

### **MotorController** (`controllers/motor_controller.h/cpp`)
Converts control commands to PWM for ESCs.
- **Tank Mixing**: `left = throttle + steering`, `right = throttle - steering`
- **Normalization**: If mixed commands exceed ±1.0, scales proportionally to maintain turning radius
- **Deadband Compensation**: Maps idealized `-1.0…1.0` to active PWM regions (skipping ESC deadband at 1482–1582 µs)
- **Arming**: Enforces neutral signal hold for 1.5 seconds before motor commands are allowed
- **Fail-Safe**: Outputs 1500 µs (neutral) on timeout

### **WebServerManager** (`web/web_server_manager.h/cpp`)
Hosts telemetry dashboard over WiFi.
- AsyncWebServer running on Core 0
- Serves embedded HTML/CSS/JS from `web_pages.h` (gzip-compressed, auto-generated)
- Exposes real-time telemetry at **20 Hz via WebSocket** (binary frames, 121 bytes fixed-size)
- WiFi operates in **AP mode** (default SSID: `ESP32-ROVER`, password: `rover1234`) or **STA mode** (joins existing network). Mode is configurable via web UI and persisted in NVS.
- Protected by mutex to avoid race conditions with `TankController` on Core 1

### **ImuSensor** / **GpsSensor** / **CompassSensor**
Read hardware sensors via I2C/UART, filter, and provide data to `TankController`.

---

## Important Algorithms

### Stick Normalization
Converts RC PWM (1000–2000 µs) to normalized float (`-1.0` to `1.0`):
```cpp
normalized = (pwm - 1500) / 500;  // [-1, 1]
// With deadzone applied around center
```

### Differential (Tank) Mixing
Combines throttle + steering into left/right motor commands:
```cpp
left = throttle + steering;
right = throttle - steering;
// If either > 1.0, scales both proportionally
```

### ESC Deadband Compensation
Many ESCs have a large inactive region (e.g., 1482–1582 µs). Algorithm maps:
- `-1.0` → 1000 µs (full reverse)
- `0.0` → 1500 µs (neutral, skips deadband)
- `+1.0` → 2000 µs (full forward)

Prevents the "dead zone" feel where small stick movements produce no motor response.

---

## Code Conventions

- **Namespaces**: `Config`, `Utils`, `Types`
- **Classes**: `PascalCase` (`TankController`)
- **Methods/Variables**: `camelCase` (`updateSystem()`)
- **Constants/Macros**: `UPPER_SNAKE_CASE` (`PWM_MIN`, `ESC_DEAD_LOW`)
- **Headers**: `#pragma once` (no traditional guards)
- **Implementation**: `.cpp` file separation for non-inline code

---

## Critical Safety & Performance Notes

1. **Watchdog Timer**: Always use `vTaskDelay()` or `yield()` in loops. Long blocking code will trigger a watchdog reset.
2. **No Blocking I/O**: Avoid `delay()` in `tankControlTask`. Use `millis()`-based state timers instead.
3. **Timeout Fail-Safe**: System MUST enter `TIMEOUT` state and output neutral PWM if iBUS signal is lost for >400 ms. Never bypass this.
4. **Arming Requirement**: ESCs must detect a neutral signal (~1500 µs) for 1.5 seconds before motor commands are accepted. Enforced in `MotorController`.
5. **Mutex Protection**: `TankController` state is accessed by both Core 0 (web) and Core 1 (control). Use `tankMutex` semaphore when reading/writing shared state.

---

## Common Development Tasks

### Adding a New Sensor
1. Define data structure in `types/types.h` (add to `SensorData` struct)
2. Create sensor class in `sensors/` (e.g., `NewSensor.h/cpp`)
3. Instantiate in `TankController` and call `update()` in main loop
4. Expose telemetry in `web_pages.h` if needed

### Adding a New Control Mode
1. Add mode enum to `types/types.h` (in `SystemState` or separate)
2. Implement logic in `TankController::update()` based on current mode
3. Update state machine transitions if needed
4. Add UI control in `web_pages.h` if remote selection is desired

### Debugging
- **Serial Output**: Monitor at 115200 baud with `pio device monitor`
- **Debug Manager**: Use `DebugManager` for timed output (avoids log flooding)
- **Web Dashboard**: Open browser to `http://<ESP32-IP>:80` for live telemetry
- **Status LED**: LED blink pattern indicates system state (check `StatusLedManager` for patterns)

### Finding Configuration Values
- **Compile-time constants**: `Config::CONTROL_INTERVAL_MS`, `Config::PWM_MIN/MAX`, `Config::ESC_DEAD_LOW/HIGH` in `config/config.h`
- **Runtime/NVS-persisted** (set via web UI, survive reboots): `Config::IBUS_TIMEOUT_MS`, `Config::DEBUG_ENABLED`, `Config::DARK_THEME`, `Config::IMU_ENABLED`, `Config::GPS_ENABLED`, `Config::WIFI_MODE`, `Config::STA_SSID`, `Config::STA_PASS`
- **Pins**: All GPIO mappings in `config/pins.h`

### Hardware Pin Conflicts to be Aware Of
GPIO **18** (ESC_RIGHT) and **19** (ESC_LEFT) conflict with standard SPI MISO/MOSI. If adding SPI peripherals (SD card, displays, etc.), the ESC pins must be relocated first — GPIO 25 and 26 are suggested free pins. GPS TX pin (GPIO 5) is shared with SPI_CS.

---

## Design Patterns Used

1. **Dependency Injection**: `TankController` receives dependencies; doesn't create them
2. **State Machine**: Strict `SystemState` transitions (INITIALIZING → ARMING → ARMED / TIMEOUT / ERROR)
3. **Facade**: `TankController` hides complexity of sensors, communication, motors from main loop
4. **Task-Based Concurrency**: FreeRTOS tasks isolate concerns (control vs. web) and prevent blocking

---

## Dependencies (from platformio.ini)

- `IBusBM` — iBUS protocol library
- `ESP32Servo` — PWM servo/ESC control
- `ESPAsyncWebServer` — Non-blocking web server
- `AsyncTCP` — Async TCP for WiFi
- `ArduinoJson` — JSON parsing (telemetry, config)
- `TinyGPSPlus` — GPS parsing

---

## Key Files to Know

- **main.cpp** — FreeRTOS task setup, critical initialization sequence
- **config/config.h** — All configurable constants (easy to find and modify)
- **types/types.h** — Data structures shared across all modules
- **tank_controller.h/cpp** — Entry point for understanding system coordination
- **web_pages.h** — Embedded HTML/CSS/JS for telemetry dashboard
