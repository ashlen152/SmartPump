# AGENTS.md

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

## Code Style Guidelines

### Imports
- Use `#include` for header files
- Group and order includes:
  1. Standard library headers
  2. Arduino/ESP32 headers
  3. Third-party libraries (Adafruit, TMCStepper, etc.)
  4. Project-specific headers

### Formatting
- Follow consistent indentation (4 spaces)
- Limit line length to 80-120 characters where possible
- Use braces for all control structures, even single-line blocks

### Types
- Use `const` for immutable variables
- Prefer `#define` for constants and macros
- Use `uint8_t`, `uint16_t` etc. for explicit integer sizes
- Use `float` for floating-point values unless double precision needed

### Naming Conventions
- Use `UPPER_SNAKE_CASE` for macros and constants
- Use `camelCase` for variables and functions
- Use `PascalCase` for class names
- Prefix class member variables with `m_`

### Error Handling
- Use return codes or exceptions for error handling
- Log errors where applicable using Serial.println() for debugging
- Check null pointers and array bounds

### Environment & Configuration
- WiFi credentials and sensitive data are passed via build flags (see `platformio.ini`)
- Use descriptive comments for configuration settings
- Environment variables are loaded using load_env.py

🤖 Generated with [opencode](https://opencode.ai)