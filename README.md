# Smart Pump DIY

This project is a DIY smart pump system built using an ESP32 microcontroller. It includes features such as WiFi connectivity, pump control, and a display interface.

## Features
- WiFi connectivity for remote control and monitoring
- Pump speed and calibration management
- OLED display for real-time status updates
- Integration with REST APIs for data synchronization

---

## Prerequisites
Before you begin, ensure you have the following installed:
1. [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
2. Python 3.7+ with `pip` installed
3. `python-dotenv` library for managing environment variables:
   ```bash
   # Activate PlatformIO's virtual environment
   source ~/.platformio/penv/bin/activate

   # Install python-dotenv in PlatformIO's environment
   pip install python-dotenv

   # Deactivate the virtual environment after installation
   deactivate
   ```

---

## Setup Instructions

### 1. Clone the Repository
Clone this repository to your local machine:
```bash
git clone https://github.com/your-repo/smart-pump-diy.git
cd smart-pump-diy
```

### 2. Create a `.env` File
Create a `.env` file in the root directory to store your WiFi credentials:
```bash
touch .env
```

Add the following content to the `.env` file:
```properties
WIFI_SSID=your wifi ssid
WIFI_PASSWORD=your wifi password
```

### 3. Install Dependencies
Install the required libraries and dependencies using PlatformIO:
```bash
pio lib install
```

### 4. Build and Upload the Firmware
Build and upload the firmware to your ESP32 board:
```bash
pio run --target upload
```

### 5. Monitor Serial Output
To monitor the serial output, use:
```bash
pio device monitor
```

---

## Usage
1. Power on the ESP32 board.
2. The device will attempt to connect to the WiFi network specified in the `.env` file.
3. Use the buttons to control the pump or navigate the menu.
4. The OLED display will show the current status and settings.

---

## Troubleshooting
- **ModuleNotFoundError: No module named 'dotenv'**  
  Ensure `python-dotenv` is installed in PlatformIO's Python environment:
  ```bash
  source ~/.platformio/penv/bin/activate
  pip install python-dotenv
  deactivate
  ```

- **WiFi Connection Issues**  
  Double-check the WiFi credentials in the `.env` file.

- **Build Errors**  
  Ensure all dependencies are installed using:
  ```bash
  pio lib install
  ```

---

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.