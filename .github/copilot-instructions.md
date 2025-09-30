# Copilot Instructions for SmartPump

## Project Overview
SmartPump is a DIY smart pump system for ESP32, featuring WiFi/Bluetooth connectivity, stepper pump control, auto-dosing, and an OLED display. The codebase is organized for PlatformIO and uses custom managers for hardware and scheduling.

## Architecture & Key Components
- **src/main.cpp**: Entry point, orchestrates hardware setup, button/menu logic, and main loop.
- **lib/**: Contains core modules:
  - `PumpController`: Stepper motor control (peristaltic/dosing modes)
  - `AutoDosingManager`: Schedules and tracks daily dosing, persists state in EEPROM
  - `DisplayManager`: OLED UI, status, calibration, and menu navigation
  - `WiFiManager`: WiFi connection, NTP time sync, REST API integration
  - `BluetoothManager`: Bluetooth Classic pairing and simple command protocol
- **include/Config.h**: Pin mappings, calibration constants, EEPROM addresses, and build-time WiFi credentials
- **backup/**: Legacy/experimental code, not used in main build

## Developer Workflows
- **Build**: `platformio run` (see AGENTS.md)
- **Test**: `platformio test` (unit tests in `test/`)
- **Upload**: `platformio run --target upload`
- **Monitor**: `platformio device monitor` for serial output
- **Environment**: WiFi credentials via `.env` and build flags; loaded by `load_env.py` (see platformio.ini)

## Coding Conventions
- **Includes**: Order: stdlib, Arduino/ESP32, third-party, project headers
- **Indentation**: 4 spaces, braces for all control blocks
- **Types**: Use explicit sizes (`uint8_t`, `float`), prefer `#define` for constants
- **Naming**: UPPER_SNAKE_CASE for macros, camelCase for functions/vars, PascalCase for classes, `m_` prefix for private members
- **Error Handling**: Use return codes, log errors via `Serial.println`
- **EEPROM**: State and calibration values are persisted; addresses defined in `Config.h`

## Integration & Data Flow
- **WiFiManager**: Connects to WiFi, syncs time (Asia NTP pools, Indochina Time), checks API health, fetches pump settings from REST API
- **AutoDosingManager**: Schedules doses (day/night split), tracks daily totals, logs events, and updates display
- **DisplayManager**: UI for status, calibration, dosing progress, and menu navigation
- **PumpController**: Controls stepper motor in peristaltic/dosing modes, supports calibration and speed adjustment
- **Button Handling**: All button logic is debounced and supports press/hold actions for menu navigation and pump control

## Patterns & Examples
- **Calibration**: Both peristaltic and dosing modes have interactive calibration flows (see `main.cpp` functions)
- **Menu System**: Menu items and navigation are managed via arrays and index logic in `main.cpp`
- **REST API**: WiFiManager uses `ArduinoHttpClient` for GET/POST/PUT/DELETE, with timeouts and error handling
- **EEPROM Usage**: All persistent state (modes, calibration, dosing) is read/written via `EEPROM.get`/`EEPROM.put` and committed

## External Dependencies
- PlatformIO (build/test/upload)
- Arduino libraries: SSD1306, HttpClient, Json, TMCStepper, AccelStepper
- Python-dotenv (for build-time env loading)

## Quick Start for AI Agents
- Start in `src/main.cpp` for control flow and hardware orchestration
- Reference `lib/` for hardware abstraction and scheduling logic
- Use `Config.h` for pinouts, constants, and EEPROM addresses
- Follow build/test/upload commands from AGENTS.md and README.md
- Adhere to code style and naming conventions above

---
For questions or unclear patterns, ask for clarification or examples from maintainers.
