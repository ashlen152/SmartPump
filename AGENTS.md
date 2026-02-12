# AGENTS.md

## Project Overview

**SmartPump** is an ESP32-based peristaltic/dosing pump controller for automated liquid dosing (e.g., aquarium fertilizer dosing). It uses a TMC2209 stepper driver with AccelStepper for precise volumetric control, an SSD1306 OLED display for UI, 4-button navigation, WiFi connectivity for time sync and remote API communication, and Bluetooth Serial for local control.

### Hardware
- **MCU**: ESP32 DevKit v1 (dual-core, WiFi + BT)
- **Stepper Driver**: TMC2209 (UART mode via Serial2)
- **Stepper Motor**: Peristaltic pump motor
- **Display**: SSD1306 128x64 OLED (I2C)
- **Buttons**: 4x physical buttons (Enable, Speed Up, Speed Down, Menu)
- **Communication**: WiFi (HTTP client), Bluetooth Classic (Serial)

### Key Features
- **Manual Dosing**: User sets volume (mL) and duration, pump dispenses precisely
- **Calibration**: Run motor for known steps, measure actual mL dispensed, calculate steps/mL
- **Auto Dosing** (WIP): Weighted schedule across day/night periods (70/30 split)
- **WiFi Sync**: NTP time sync (Asia/Vietnam timezone), HTTP API for remote settings
- **OLED UI**: State-driven display with menu navigation, progress screens, signal strength indicator

---

## Architecture

### Directory Structure

```
SmartPump/
├── platformio.ini                  # PlatformIO config (ESP32, Arduino framework)
├── load_env.py                     # Pre-build script for WiFi credentials from env
├── AGENTS.md                       # This file
├── include/
│   ├── Config.h                    # Pin assignments, EEPROM map, timing constants
│   ├── ButtonConfig.h              # Button pin definitions
│   └── WifiConfig.h                # WiFi credentials, API endpoints, intervals
├── lib/
│   ├── PumpController/             # Stepper motor control (TMC2209 + AccelStepper)
│   │   ├── PumpController.h
│   │   └── PumpController.cpp
│   ├── DisplayManager/             # SSD1306 OLED display management
│   │   ├── DisplayManager.h
│   │   └── DisplayManager.cpp
│   ├── WiFiManager/                # WiFi connection, HTTP client, NTP time
│   │   ├── WiFiManager.h
│   │   └── WiFiManager.cpp
│   ├── NetworkTaskManager/         # Core 0 network task manager (multi-core)
│   │   ├── NetworkTaskManager.h
│   │   └── NetworkTaskManager.cpp
│   ├── AutoDosingManager/          # Automated dosing scheduler (WIP)
│   │   ├── AutoDosingManager.h
│   │   └── AutoDosingManager.cpp
│   └── BluetoothManager/           # Bluetooth Classic serial communication
│       ├── BluetoothManager.h
│       └── BluetoothManager.cpp
├── src/
│   ├── main.cpp                    # Entry point: setup() and loop()
│   ├── ButtonController/           # Button press/hold abstraction layer
│   │   ├── ButtonController.h      # High-level: pressButtonUp(), holdButtonDown(), etc.
│   │   ├── ButtonController.cpp
│   │   ├── ButtonHandler.h         # Low-level: checkButtonPress(), checkButtonPressOrHold()
│   │   └── ButtonHandler.cpp
│   ├── DisplayController/          # Main loop display update orchestrator
│   │   ├── DisplayUpdater.h
│   │   └── DisplayUpdater.cpp
│   ├── WifiController/             # WiFi connection handler and data sync
│   │   ├── WiFiSync.h
│   │   └── WiFiSync.cpp
│   ├── ManualDosingController/     # Manual dosing flow (volume setup -> start -> progress -> complete)
│   │   ├── ManualDosingController.h
│   │   └── ManualDosingController.cpp
│   ├── CalibrateDosingController/  # Calibration flow (begin -> progress -> complete/input)
│   │   ├── CalibrateDosingController.h
│   │   └── CalibrateDosingController.cpp
│   └── ViewController/             # UI screen handlers (state machine navigation)
│       ├── Home/
│       │   ├── HomeHandler.h
│       │   └── HomeHandler.cpp
│       ├── Menu/
│       │   ├── MenuHandler.h
│       │   └── MenuHandler.cpp
│       └── Manual/
│           ├── ManualHandler.h
│           └── ManualHandler.cpp
├── backup/                         # Old/legacy code (not compiled)
│   ├── main.cpp
│   ├── PumpController.h/cpp
│   └── src/Calibration.h/cpp
└── autoschedule.cpp                # Standalone C++ simulation of weighted dosing schedule
```

### Design Patterns
- **Singleton**: `PumpController`, `DisplayManager`, `WiFiManager`, `NetworkTaskManager` all use `getInstance()`
- **State Machine**: `DisplayManager::DisplayState` enum drives which screen is shown
- **MVC-like**: ViewController handles user input, Controllers handle business logic, DisplayManager handles rendering
- **Context Pattern**: `DisplayContext` struct passes display data to the state-driven renderer

### Multi-Core Architecture

**SmartPump uses ESP32's dual-core architecture to separate time-critical pump control from blocking network operations.**

#### Core Assignment
- **Core 0** (Network Task): WiFi, HTTP, NTP, MQTT - all blocking network operations
- **Core 1** (Main Loop): Pump control, display updates, button handling - time-critical operations

#### Architecture Benefits
1. **No Blocking**: Network operations (WiFi connect, HTTP requests, NTP sync) run on Core 0 and never block pump control on Core 1
2. **Thread Safety**: All WiFi operations are mutex-protected for safe access from both cores
3. **Auto Reconnect**: Core 0 automatically reconnects WiFi every 5 seconds if disconnected
4. **Auto Sync**: Core 0 automatically syncs time every hour and checks server health every 3 minutes
5. **Precision**: Pump control loop runs uninterrupted for accurate step timing

#### Inter-Core Communication
- **FreeRTOS Queues**: Commands flow Core 1 → Core 0, responses flow Core 0 → Core 1
- **Non-Blocking**: All queue operations use 0ms timeout to avoid blocking pump control
- **Message Types**:
  - `NetworkCommand`: CONNECT_WIFI, SYNC_TIME, HTTP_GET_SETTINGS, HTTP_POST_SETTINGS, HEALTH_CHECK, GET_STATUS
  - `NetworkStatus`: WIFI_CONNECTED, TIME_SYNCED, HTTP_OK, HTTP_ERROR, etc.

#### Implementation Files
- **lib/NetworkTaskManager/**: Core 0 network task implementation
  - `NetworkTaskManager.h`: Queue message types, command/response enums, class definition
  - `NetworkTaskManager.cpp`: Task loop, command processing, background tasks
- **lib/WiFiManager/**: WiFi/HTTP/NTP operations (called from Core 0 only)
- **src/main.cpp**: Spawns Core 0 task in setup(), handles responses in loop()
- **src/WifiController/WiFiSync.cpp**: DEPRECATED - kept for compatibility but no longer used

#### Initialization Sequence
```cpp
setup() {
  1. Serial, EEPROM, GPIO, Display, Pump initialization
  2. NetworkTaskManager::initialize(commandQueueSize=10, responseQueueSize=10)
  3. NetworkTaskManager::start(stackSize=8192, priority=1) → spawns on Core 0
  4. Send CONNECT_WIFI command to Core 0
}
```

#### Core 0 Task Loop
```cpp
networkTask() {
  while (true) {
    1. Check command queue (100ms timeout)
    2. Process command if available (WiFi connect, HTTP GET/POST, NTP sync, etc.)
    3. Run background tasks:
       - WiFi keepalive check (every 5 seconds)
       - Auto time sync (every 1 hour)
       - Auto health check (every 3 minutes)
    4. Send responses to Core 1 via response queue
    5. Small delay (10ms) to prevent watchdog
  }
}
```

#### Core 1 Main Loop (Updated)
```cpp
loop() {
  1. HomeHandler()           -- Button input handling
  2. ManualHandler()         -- Manual dosing state machine
  3. updateDisplayStatus()   -- OLED refresh
  4. pump.runDosing()        -- Time-critical stepper control
  5. if (DOSING) return      -- Early return during dosing (unchanged)
  6. Check response queue    -- Handle WiFi/HTTP responses from Core 0
     (non-blocking, process all available responses)
  7. No direct WiFi calls    -- All network ops delegated to Core 0
}
```

#### Thread Safety
- **WiFiManager Mutex**: All WiFi operations acquire mutex before accessing WiFi/HTTP
- **Queue Safety**: FreeRTOS queues are inherently thread-safe
- **No Shared State**: Cores communicate only via queues, no shared global state

#### Background Tasks (Core 0)
| Task               | Interval    | Description                                      |
|--------------------|-------------|--------------------------------------------------|
| WiFi Keepalive     | 5 seconds   | Auto-reconnect if WiFi disconnected              |
| Time Sync          | 1 hour      | NTP re-sync for clock accuracy                   |
| Health Check       | 3 minutes   | Server availability check                        |

### Core Flow (main loop)

```
(See Multi-Core Architecture section above for updated flow)

### Pump Modes
- **PERISTALTIC**: Continuous speed mode (constant flow)
- **DOSING**: Position control mode (move exact number of steps for target mL)
- **HOLDING**: Idle/waiting between movements

### Display States
```
NORMAL -> MENU -> (selection) -> CALIBRATE_BEGIN/SETTINGS/DOSING_SETUP
                                      |
DOSING_MANUAL_BEGIN -> DOSING_MANUAL_START -> DOSING_MANUAL_PROGRESS -> DOSING_MANUAL_COMPLETE
                                      |
CALIBRATE_BEGIN -> CALIBRATE_PROGRESS -> CALIBRATE_COMPLETE
```

### Button Mapping
| Button      | Pin | Home Screen        | Menu Screen     | Manual Dosing     |
|-------------|-----|--------------------|-----------------|-------------------|
| Enable      | 25  | Enter Manual Mode  | -               | Cancel/Confirm    |
| Speed Up    | 35  | -                  | Navigate Up     | Increase value    |
| Speed Down  | 34  | -                  | Navigate Down   | Decrease value    |
| Menu        | 14  | Open Menu          | Select Item     | Next step         |

### EEPROM Memory Map (Phase 3 - Updated)
| Address | Size     | Data                                    |
|---------|----------|-----------------------------------------|
| 0       | 4 bytes  | Peristaltic steps/mL (float)            |
| 4       | 4 bytes  | Dosing steps/mL (float)                 |
| 8       | 4 bytes  | Saved speed (float)                     |
| 12      | 1 byte   | Mode (uint8_t)                          |
| 13      | 1 byte   | Auto dosing enabled (bool)              |
| 14      | 4 bytes  | Daily volume (float)                    |
| 18      | 4 bytes  | Last dosing time (uint32_t)             |
| 22      | 4 bytes  | Total dosed volume (float)              |
| 26      | 1 byte   | Day start hour (uint8_t)                |
| 27      | 1 byte   | Day end hour (uint8_t)                  |
| 28      | 1 byte   | Day percent (uint8_t)                   |
| 30-165  | 136 bytes| Wear leveling banks (4×34B, compiled)   |
| 170     | 16 bytes | Pump ID (char[16])                      |
| 186     | 5 bytes  | Pause state (bool + uint32_t)           |
| 191     | 62 bytes | Dose history (1B count + 1B head + 5×12B)|
| 231     | 13 bytes | Speed profiles (3×float + uint8_t)      |
| **Total**| **244/512 bytes (48%)**               |

### WiFi / Network
- Server: `192.168.68.108:3000` (hardcoded in WiFiManager)
- NTP: Asia pool servers, timezone ICT-7 (Vietnam UTC+7)
- API endpoints: `/api/pump-settings`, `/api/pump-settings/getById`, `/api/health`
- Retry interval: 5 seconds
- Sync interval: 3 minutes
- HTTP timeout: 1 second

---

## Build, Lint, and Test Commands

### Build
- Use PlatformIO to build the project:
  ```bash
  platformio run
  ```

### Test
- Run all tests using PlatformIO's test runner:
  ```bash
  platformio test
  ```
- Run a specific test file:
  ```bash
  platformio test -f test_file_name
  ```
- For more information on unit testing, refer to:
  [PlatformIO Unit Testing Documentation](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)

### Lint
- No specific linting tool is configured. Ensure code adheres to the style guidelines below.

---

## Code Style Guidelines

### Imports
- Use `#include` for header files
- Group and order includes:
  1. Standard library headers (`<vector>`, `<time.h>`)
  2. Arduino/ESP32 headers (`<Arduino.h>`, `<EEPROM.h>`, `<WiFi.h>`)
  3. Third-party libraries (`<Adafruit_SSD1306.h>`, `<TMCStepper.h>`, `<AccelStepper.h>`, `<ArduinoJson.h>`)
  4. Project-specific headers (`<Config.h>`, `"PumpController.h"`, `"DisplayManager.h"`)

### Formatting
- Follow consistent indentation (4 spaces)
- Limit line length to 80-120 characters where possible
- Use braces for all control structures, even single-line blocks

### Types
- Use `const` for immutable variables
- Prefer `constexpr` for compile-time constants in namespaces/classes
- Use `#define` for preprocessor macros and conditional compilation flags
- Use `uint8_t`, `uint16_t` etc. for explicit integer sizes
- Use `float` for floating-point values unless double precision needed

### Naming Conventions
- Use `UPPER_SNAKE_CASE` for macros and constants
- Use `camelCase` for variables and functions
- Use `PascalCase` for class names and enum class values
- Prefix class member variables with `m_` (DisplayManager) or no prefix (PumpController - existing pattern)

### Error Handling
- Use return codes or exceptions for error handling
- Log errors where applicable using `Serial.println()` / `Serial.printf()` for debugging
- Check null pointers and array bounds
- Use conditional debug logging with `#ifdef` / `#if` macros

### Environment & Configuration
- WiFi credentials and sensitive data are passed via build flags (see `platformio.ini`)
- Use descriptive comments for configuration settings
- Environment variables are loaded using `load_env.py`
- Pin assignments are centralized in `include/Config.h` and `include/ButtonConfig.h`

---

## Known Issues & Technical Debt

1. ~~**Include guard collision**~~: ✅ **FIXED (Phase 1)** - ButtonConfig.h now uses `#ifndef BUTTONCONFIG_H`
2. ~~**Hardcoded WiFi credentials**~~: ✅ **FIXED (Phase 1)** - WifiConfig.h uses build flags from platformio.ini
3. ~~**DisplayManager state mismatch**~~: ✅ **FIXED (Phase 1)** - Correct enum values now used
4. ~~**AutoDosingManager incomplete**~~: ✅ **FIXED (Phase 1)** - All 6 core functions implemented
5. ~~**BluetoothManager unused**~~: ✅ **REMOVED (Phase 3)** - Deleted from `lib/` directory
6. ~~**Blocking WiFi connect**~~: ✅ **FIXED (Phase 1)** - Multi-core architecture, Core 0 handles WiFi
7. ~~**`updateDisplayState()` bug**~~: ✅ **FIXED (Phase 3 Sprint 1)** - `lastUpdate = 0` removed
8. ~~**CalibrateDosingController.h empty**~~: ✅ **FIXED (Phase 1)** - Function declarations added
9. ~~**ManualHandler state logic**~~: ✅ **FIXED (Phase 3 Sprint 1)** - Changed to `beginManualDosingController(false)`
10. ~~**Display context hardcoded**~~: ✅ **FIXED (Phase 1)** - Uses real pump state now
11. ~~**Manual dosing stop missing**~~: ✅ **FIXED (Phase 1)** - `pump.stop()` added
12. ~~**Calibration progress display**~~: ✅ **FIXED (Phase 1)** - Shows step count and percentage

**Summary**: ✅ **ALL 12 ISSUES RESOLVED** (Phases 1-3)

---

## Server API Specification (Phase 2)

SmartPump firmware POSTs dose history events to a backend server for tracking and analytics. The server implementation is external to this firmware project.

### Dose History Logging Endpoint

**Endpoint**: `POST /api/dose-history`

**Request Headers**:
- `Content-Type: application/json`

**Request Body**:
```json
{
  "timestamp": 1709876543,
  "volume": 0.625,
  "success": true,
  "pumpId": "SmartPump_01"
}
```

**Field Descriptions**:
| Field      | Type    | Description                                    |
|------------|---------|------------------------------------------------|
| timestamp  | uint32  | Unix epoch time (seconds since Jan 1, 1970)   |
| volume     | float   | Volume dispensed in mL                         |
| success    | boolean | Whether dose completed successfully            |
| pumpId     | string  | Device identifier (e.g., "SmartPump_01")       |

**Success Response**:
- HTTP 200 OK or 201 Created
- Body: (any valid response, firmware ignores response body)

**Error Response**:
- HTTP 4xx/5xx
- Firmware will log error to Serial but continue operation

**Notes**:
- Endpoint is currently **mock/placeholder** - server implementation pending
- POSTs are fire-and-forget (non-blocking, no retry on failure)
- If WiFi is disconnected, POST is skipped and error logged to Serial
- Dose events are queued to Core 0 via `NetworkTaskManager::HTTP_POST_DOSE_LOG`

---

## Future Architecture Recommendations

### ~~Multi-Core Separation~~ ✅ IMPLEMENTED
- Multi-core architecture has been successfully implemented (see Multi-Core Architecture section above)
- **Core 0**: WiFi, HTTP, NTP sync (NetworkTaskManager) - DONE
- **Core 1**: Pump control, display updates, button handling - DONE
- Uses `xTaskCreatePinnedToCore()` with FreeRTOS queues for inter-core communication - DONE

### Stay on Arduino Framework ✅
- Current libraries (TMCStepper, AccelStepper, Adafruit SSD1306) are Arduino-native
- ESP-IDF migration would require rewriting all hardware drivers
- Arduino already provides FreeRTOS access for multi-core support

---

## Feature Backlog (Future Phases)

### Phase 3 Candidates (Not Scheduled)

#### 1. Bluetooth Simple Command API
**Description**: Enable Bluetooth serial commands for remote control and schedule inspection without WiFi.

**Status**: Deferred - No current plan for implementation

**Features**:
- Simple text-based command protocol (e.g., `STATUS`, `ENABLE`, `SET_VOLUME 30`)
- Non-blocking command processing on Core 1
- Commands: status query, auto-dosing toggle, volume/period/split configuration, schedule viewing
- Reference implementation exists in `lib/BluetoothManager/` but needs rewrite for simple protocol

**Effort**: ~3-4 hours  
**Priority**: Low  
**Blocker**: None - waiting for user need/request

---

#### 2. Multi-Day Dose Planning
**Description**: Allow scheduling doses across multiple days with different volumes per day (e.g., Monday=100%, Tuesday=80%).

**Status**: Deferred - Unclear if needed for aquarium dosing use case

**Challenges**:
- **EEPROM Constraint**: Would require ~100 bytes for 7-day schedule (only ~482 bytes remaining)
- **UI Complexity**: Menu would need 7+ sub-items for weekly pattern configuration
- **State Machine**: Multi-day dosing state tracking increases complexity

**Minimal Viable Approach** (if pursued):
- Store only weekly multipliers (7 bytes for day-of-week percentages)
- Scale current daily volume by day multiplier (no separate per-day schedules)
- Add menu item: "Weekly Pattern" with 7 sub-screens

**Effort**: ~5-7 hours  
**Priority**: Very Low  
**Blocker**: Unclear use case - single-day scheduling may be sufficient

---

## Phase 2 Completion Summary (Current)

✅ **Sprint 1: Configurable Day/Night Split** - COMPLETE
- EEPROM addresses added for day/night config (Config.h)
- `setDayPeriod()` and `setDayNightSplit()` methods implemented (AutoDosingManager)
- EEPROM persistence in loadState/saveState
- 2 new menu items: "Day Period" (Case 4), "Day/Night %" (Case 5)
- Settings persist across reboots
- Schedule regenerates automatically when settings change
- Supports midnight wrap-around (e.g., 23:00-05:00)

✅ **Sprint 2: Dose History Server Logging** - COMPLETE
- `HTTP_POST_DOSE_LOG` command added to NetworkTaskManager
- `handleHttpPostDoseLog()` handler implemented (Core 0)
- `WiFiManager::postDoseLog()` method added
- `logDosingEvent()` updated to queue JSON POST to `/api/dose-history`
- Fire-and-forget non-blocking POST (WiFi failure doesn't stop dosing)
- Server API spec documented in AGENTS.md

**Build Status**: ✅ SUCCESS  
**Flash Usage**: 858,621 / 1,310,720 bytes (65.5%)  
**RAM Usage**: 46,392 / 327,680 bytes (14.2%)  
**EEPROM Usage**: 30 / 512 bytes (5.9%)

**Files Modified (Phase 2)**: 12 files
1. `include/Config.h` - Added 3 EEPROM addresses
2. `lib/AutoDosingManager/AutoDosingManager.h` - New methods + Config fields
3. `lib/AutoDosingManager/AutoDosingManager.cpp` - Implementation + dose logging
4. `src/ViewController/Menu/MenuHandler.cpp` - 2 new menu items (Cases 4 & 5)
5. `src/main.cpp` - Updated EEPROM config initialization
6. `lib/WiFiManager/WiFiManager.h` - Added postDoseLog declaration
7. `lib/WiFiManager/WiFiManager.cpp` - Implemented postDoseLog
8. `lib/NetworkTaskManager/NetworkTaskManager.h` - HTTP_POST_DOSE_LOG enum + handler declaration
9. `lib/NetworkTaskManager/NetworkTaskManager.cpp` - Handler implementation
10. `AGENTS.md` - Updated Known Issues + added Server API Specification + Backlog

**Next Steps**:
- Hardware testing of menu items 4 & 5
- Server implementation of `/api/dose-history` endpoint
- Verify dose logging during auto-dosing execution
- Test day/night period wrap-around edge cases

---

## Phase 3 Completion Summary (COMPLETE)

✅ **Sprint 1: Bug Fixes** - COMPLETE
- Fixed `DisplayManager.cpp:15` - Removed `lastUpdate = 0` (throttle now works)
- Fixed `ManualHandler.cpp:13` - Changed to `beginManualDosingController(false)` for proper state init
- Fixed `ButtonConfig.h:14-16` - Removed outdated include guard comment

✅ **Sprint 2: Configuration System** - COMPLETE
- Created `lib/ConfigManager/ConfigManager.h` (88 lines) - Pump ID management singleton
- Created `lib/ConfigManager/ConfigManager.cpp` (142 lines) - Validation (alphanumeric + underscore, max 15 chars)
- Updated `include/Config.h` - Added Phase 3 EEPROM addresses (170-243)
- Updated `include/WifiConfig.h` - Added SERVER_ADDRESS/SERVER_PORT build flags
- Updated `load_env.py` - Parses SERVER_ADDRESS/SERVER_PORT from .env
- Menu items: Case 6 "Set Pump ID", Case 7 "Reset Config"

✅ **Sprint 3: EEPROM Wear Leveling** - COMPLETE
- Created `lib/EEPROMManager/EEPROMManager.h` (175 lines) - 4-bank rotation system
- Created `lib/EEPROMManager/EEPROMManager.cpp` (216 lines) - CRC-16-CCITT, bank rotation
- **NOTE**: Compiled but NOT yet integrated into managers (future enhancement)

✅ **Sprint 4: Error Handling** - COMPLETE
- **Time Sync Fallback**: AutoDosingManager uses millis() offset when WiFi down
  - Added `lastSyncMillis` and `lastSyncTime` members
  - checkAndDose() logs every 5 min when using fallback
- **Retry Queue**: NetworkTaskManager 5-entry FIFO for failed dose logs
  - Max 3 attempts, 5min retry interval, drops after final failure
  - Methods: addToRetryQueue(), processRetryQueue()
- **Watchdog Timer**: 10-second timeout with panic on timeout
  - `esp_task_wdt_init(10, true)` in setup()
  - `esp_task_wdt_reset()` at start of loop()

✅ **Sprint 5: Pause/Resume Feature** - COMPLETE
- Added pause/resume methods to AutoDosingManager
  - `pause(uint32_t durationSeconds)` - 0 = indefinite
  - `resume()` - Clears pause and regenerates schedule
  - `isPaused()` - Checks if paused and if timed pause expired
  - `getPauseRemaining()` - Returns seconds remaining (0xFFFFFFFF = indefinite)
- EEPROM persistence at addr 186 (5 bytes: bool + uint32_t)
- checkAndDose() auto-resumes on timed pause expiration
- Menu items: Case 8 "Pause Dosing" (submenu), Case 9 "Resume Dosing"
- DisplayUpdater shows pause status: "PAUSED" or "P:Xh/Xm/Xs"

✅ **Sprint 6: Dose History Display** - COMPLETE
- Added `DoseHistoryEntry` struct (12 bytes: timestamp + volume + success + padding)
- Ring buffer: 5 entries, tracked by `doseHistoryCount` and `doseHistoryHead`
- Methods: `addDoseToHistory()`, `getDoseHistory()`, `clearDoseHistory()`, `loadDoseHistory()`, `saveDoseHistory()`
- EEPROM: addr 191, 62 bytes (1B count + 1B head + 5×12B entries)
- logDosingEvent() now calls addDoseToHistory() automatically
- Menu item: Case 10 "Dose History" - Shows formatted list (MM/DD HH:MM X.XmL OK/X)
- Forward declaration used to avoid circular dependency

✅ **Sprint 7: Speed Profiles** - COMPLETE
- Added `speedProfiles[3]` array and `activeProfile` index to PumpController
- Profile speeds: `{10000.0f, 20000.0f, 40000.0f}` for Slow/Med/Fast
- Implemented methods:
  - `setSpeedProfile(uint8_t profile)` - Switch active profile (0-2)
  - `getActiveProfile()` - Return active index (0-2)
  - `getProfileSpeed(uint8_t profile)` - Get speed for specific profile
  - `setProfileSpeed(uint8_t profile, float speed)` - Customize profile speed
  - `loadSpeedProfiles()` / `saveSpeedProfiles()` - EEPROM persistence
- EEPROM persistence (addr 231, 13 bytes: 3×float + uint8_t)
- Menu items: Case 11 "Speed Profile" (Slow/Med/Fast selector), Case 12 "Edit Profiles"
- PumpController::begin() calls loadSpeedProfiles()

✅ **Sprint 8: Unit Testing** - COMPLETE
- Added `[env:native]` to platformio.ini for native testing
- Created 4 test files with 19 total tests (all passing):
  - `test/test_schedule_generation/test_schedule.cpp` - 6 tests (schedule distribution, day/night split, midnight wrap)
  - `test/test_eeprom/test_eeprom_manager.cpp` - 5 tests (CRC-16, bank rotation, corruption detection)
  - `test/test_time/test_auto_dosing_time.cpp` - 4 tests (time fallback, dose timing, wrap-around)
  - `test/test_config/test_config_manager.cpp` - 4 tests (pump ID validation)
- All tests passed: `platformio test -e native` ✅ 19/19 PASSED

✅ **Sprint 9: Cleanup** - COMPLETE
- Deleted `lib/BluetoothManager/` directory (unused, deferred to backlog)
- Updated AGENTS.md:
  - EEPROM map now shows Phase 3 addresses (170-243) and 244/512 bytes (48% usage)
  - Known Issues section updated: ALL 12 ISSUES RESOLVED
  - Added Phase 3 Completion Summary (this section)
  - Menu structure updated to show 13 menu items (Cases 0-12)

**Build Status**: ✅ SUCCESS  
**Flash Usage**: 874,669 / 1,310,720 bytes (66.7%)  
**RAM Usage**: 46,640 / 327,680 bytes (14.2%)  
**EEPROM Usage**: 244 / 512 bytes (48%)  
**Unit Tests**: ✅ 19/19 PASSED

**Files Modified (Phase 3)**: 31 files
1. `lib/DisplayManager/DisplayManager.cpp` - Fixed lastUpdate bug (Sprint 1)
2. `src/ViewController/Manual/ManualHandler.cpp` - Fixed state init bug (Sprint 1)
3. `include/ButtonConfig.h` - Removed outdated comment (Sprint 1)
4. `include/Config.h` - Added Phase 3 EEPROM addresses (Sprint 2-7)
5. `include/WifiConfig.h` - Added SERVER build flags (Sprint 2)
6. `load_env.py` - Parse SERVER_ADDRESS/PORT from .env (Sprint 2)
7. `lib/ConfigManager/ConfigManager.h` - NEW (Sprint 2, 88 lines)
8. `lib/ConfigManager/ConfigManager.cpp` - NEW (Sprint 2, 142 lines)
9. `lib/EEPROMManager/EEPROMManager.h` - NEW (Sprint 3, 175 lines)
10. `lib/EEPROMManager/EEPROMManager.cpp` - NEW (Sprint 3, 216 lines)
11. `lib/AutoDosingManager/AutoDosingManager.h` - Updated for Sprints 4-6 (time fallback, pause, history)
12. `lib/AutoDosingManager/AutoDosingManager.cpp` - Updated for Sprints 4-6 (~400 lines added)
13. `lib/NetworkTaskManager/NetworkTaskManager.h` - Retry queue structures (Sprint 4)
14. `lib/NetworkTaskManager/NetworkTaskManager.cpp` - Retry queue methods (Sprint 4, ~110 lines)
15. `src/main.cpp` - Watchdog timer init (Sprint 4)
16. `src/DisplayController/DisplayUpdater.cpp` - Pause status display (Sprint 5)
17. `lib/DisplayManager/DisplayManager.h` - DOSE_HISTORY state + showDoseHistory() (Sprint 6)
18. `lib/DisplayManager/DisplayManager.cpp` - showDoseHistory() implementation (Sprint 6)
19. `src/ViewController/Menu/MenuHandler.cpp` - Cases 6-12 added (Sprints 2, 5-7, ~300 lines)
20. `lib/PumpController/PumpController.h` - Speed profiles members + methods (Sprint 7)
21. `lib/PumpController/PumpController.cpp` - Speed profiles implementation (Sprint 7, ~120 lines)
22. `platformio.ini` - Added [env:native] for unit testing (Sprint 8)
23. `test/test_schedule_generation/test_schedule.cpp` - NEW (Sprint 8, 206 lines, 6 tests)
24. `test/test_eeprom/test_eeprom_manager.cpp` - NEW (Sprint 8, 137 lines, 5 tests)
25. `test/test_time/test_auto_dosing_time.cpp` - NEW (Sprint 8, 122 lines, 4 tests)
26. `test/test_config/test_config_manager.cpp` - NEW (Sprint 8, 85 lines, 4 tests)
27-31. **Deleted**: `lib/BluetoothManager/` directory (Sprint 9)

**Total Lines Added**: ~2,400 lines  
**Total Lines Deleted**: ~200 lines (BluetoothManager)  
**Net Change**: ~2,200 lines

**Menu Structure (Final - 13 items)**:
```
0.  Dosing Cal         - Calibration flow
1.  Settings Info      - Display current settings
2.  Auto Dosing        - Toggle auto-dosing on/off
3.  Set Daily Vol      - Configure daily volume (mL)
4.  Day Period         - Set day start/end hours
5.  Day/Night %        - Configure day/night split percentage
6.  Set Pump ID        - Edit pump identifier (char editor)
7.  Reset Config       - Factory reset (with confirmation)
8.  Pause Dosing       - Pause for 1h/6h/12h/24h/indefinite
9.  Resume Dosing      - Resume from pause
10. Dose History       - View last 5 doses (formatted)
11. Speed Profile      - Select Slow/Medium/Fast profile
12. Edit Profiles      - Customize profile speeds
```

**Architecture Highlights**:
- **Multi-core**: Core 0 = Network/WiFi, Core 1 = Pump/Display/Buttons
- **Singleton Pattern**: All managers use getInstance()
- **State Machine**: DisplayManager::DisplayState drives UI
- **Ring Buffer**: 5-entry dose history with head/count tracking
- **Wear Leveling**: 4-bank EEPROM system (compiled but not integrated)
- **Error Resilience**: Time fallback, retry queue, watchdog timer
- **Forward Declarations**: Used to avoid circular dependencies

**Next Steps**:
- Hardware testing of all new menu items (Cases 6-12)
- Integration testing of pause/resume feature
- Validation of speed profiles on hardware
- Performance testing of dose history display
- Optional: Integrate EEPROMManager wear leveling into managers

