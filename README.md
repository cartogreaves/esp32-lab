# ESP32 Modular Sensor Display 🌡️🏄‍♂️

A modular ESP32 system with e-paper display supporting multiple sensor deployments: **Temperature & Humidity** and **Surf Forecast**. Built with clean architecture for easy deployment switching.

## 🌡️🏄‍♂️ What it does

This project creates modular sensor displays that:
- **Temperature & Humidity Mode**: Monitors room/office temperature and humidity using DHT11 sensor
- **Surf Forecast Mode**: Fetches live wave height data from Open-Meteo Marine API for Cribbar, Newquay
- Displays data on low-power e-paper display with 30-second refresh intervals
- Uses modular architecture for easy deployment switching
- Supports multiple PlatformIO environments for different configurations

## 🏗️ Hardware Requirements

- **ESP32 Board**: Freenove ESP32 Wrover (with PSRAM)
- **E-paper Display**: 2.9" Black/White e-paper display (296x128px)
- **LED**: Status indicator LED
- **DHT11 Sensor**: Temperature and humidity sensor (for temperature/humidity deployment)
- **WiFi Connection**: Required for surf forecast deployment

### Pin Configuration

The project uses safe GPIO pins that avoid conflicts with PSRAM:

```
E-paper Display:
├── CS (Chip Select): GPIO 5
├── DC (Data/Command): GPIO 2
├── RST (Reset): GPIO 15
├── BUSY: GPIO 4
├── MOSI: GPIO 23 (default SPI)
└── SCK: GPIO 18 (default SPI)

Temperature & Humidity:
├── DHT11 DATA: GPIO 13
├── DHT11 VCC: 3.3V
├── DHT11 GND: GND
└── Pull-up: 10kΩ between DATA and VCC

LED Controller:
└── LED: GPIO 25
```

> **Note**: GPIO 16/17 are intentionally avoided as they're used for PSRAM on the ESP32 Wrover.

## 📊 Features

### Modular Architecture
The project uses a clean, modular design supporting multiple sensor deployments:

- **`SensorInterface`**: Common interface for all sensor types
- **`TemperatureHumiditySensor`**: DHT11 temperature and humidity monitoring
- **`SurfForecast`**: WiFi-based surf condition monitoring
- **`TimeUtils`**: NTP time synchronization and timestamp formatting
- **`EPaperDisplay`**: Unified e-paper display management
- **`LEDController`**: Simple LED control (on/off/toggle/flash)

### Deployment Modes

#### 🌡️ Temperature & Humidity Mode
- **Sensor**: DHT11 digital temperature and humidity sensor
- **Accuracy**: ±2°C temperature, ±5% humidity
- **Update Rate**: Sensor readings every 30 seconds
- **Display**: Large, clear temperature and humidity values
- **Timestamp**: Last update time tracking

#### 🏄‍♂️ Surf Forecast Mode
- **Location**: Cribbar, Newquay (50.426°N, 5.103°W)
- **Data Source**: Open-Meteo Marine API (free, no API key needed)
- **Current Conditions**: Real-time wave height and rating
- **Today's Forecast**: 12-hour average wave conditions
- **Tomorrow's Forecast**: Next day's wave predictions
- **Automatic Updates**: Fresh data every 30 minutes
- **UK Time**: Proper timezone handling (GMT/BST)

### Wave Rating System
```
FLAT:  < 1.0 ft
SMALL: 1.0 - 2.0 ft
GOOD:  2.0 - 4.0 ft
GREAT: 4.0 - 6.0 ft
EPIC:  6.0 - 8.0 ft
HUGE:  > 8.0 ft
```

### Display Features
- **Refresh Rate**: 30-second intervals for both deployment types
- **Low Power**: E-paper display with sleep modes
- **Resolution**: 296x128px (2.9" display)
- **Always-On**: Perfect for continuous monitoring
- **Clean Layout**: Optimized layouts for each sensor type

## 🚀 Getting Started

### Prerequisites
- [PlatformIO](https://platformio.org/) IDE or extension
- ESP32 development environment
- DHT11 sensor (for temperature/humidity deployment)

### Choose Your Deployment

The project supports two deployment modes:

#### 🌡️ Temperature & Humidity (Default)
- Requires DHT11 sensor connected to GPIO 13
- No WiFi required
- Perfect for room/office monitoring

#### 🏄‍♂️ Surf Forecast
- Requires WiFi connection
- Fetches data from Open-Meteo API
- No additional sensors needed

### Dependencies

Libraries are automatically managed by PlatformIO based on deployment mode:

**Temperature & Humidity Mode:**
```ini
lib_deps =
    zinggjm/GxEPD2@^1.5.3
    adafruit/Adafruit GFX Library@^1.11.9
    adafruit/DHT sensor library@^1.4.4
```

**Surf Forecast Mode:**
```ini
lib_deps =
    zinggjm/GxEPD2@^1.5.3
    adafruit/Adafruit GFX Library@^1.11.9
    bblanchon/ArduinoJson@^7.0.4
```

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/yourusername/esp32-lab.git
   cd esp32-lab
   ```

2. **Configure deployment mode** in `platformio.ini`:
   ```ini
   ; For temperature & humidity (default)
   build_flags = -DDEPLOYMENT_TEMPERATURE_HUMIDITY

   ; For surf forecast
   ; build_flags = -DDEPLOYMENT_SURF_FORECAST
   ```

3. **Configure WiFi** (surf forecast mode only) in `src/main.cpp`:
   ```cpp
   const char* WIFI_SSID = "your-wifi-name";
   const char* WIFI_PASSWORD = "your-wifi-password";
   ```

4. **Build and upload**:
   ```bash
   pio run --target upload
   ```

5. **Monitor serial output**:
   ```bash
   pio device monitor
   ```

### Alternative: Use Specific Environments

PlatformIO provides dedicated environments for each deployment:

```bash
# Temperature & humidity deployment
pio run --environment temperature_humidity --target upload

# Surf forecast deployment
pio run --environment surf_forecast --target upload

# Default environment (temperature & humidity)
pio run --target upload
```

## 🔧 Project Structure

```
esp32-lab/
├── src/
│   ├── main.cpp                         # Main application with deployment selection
│   ├── led_controller.cpp               # LED control implementation
│   ├── epaper_display.cpp               # E-paper display driver
│   ├── time_utils.cpp                   # NTP time synchronization and formatting
│   ├── temperature_and_humidity.cpp     # DHT11 sensor implementation
│   └── surf_forecast.cpp                # Surf forecast API implementation
├── include/
│   ├── sensor_interface.h               # Common sensor interface
│   ├── led_controller.h                 # LED controller header
│   ├── epaper_display.h                 # Display interface header
│   ├── time_utils.h                     # Time utilities header
│   ├── temperature_and_humidity.h       # Temperature/humidity sensor header
│   └── surf_forecast.h                  # Surf forecast header
├── platformio.ini                       # PlatformIO multi-environment config
└── README.md                            # This file
```

## 🌐 Data Sources

### Surf Forecast API
The surf forecast mode uses the [Open-Meteo Marine API](https://marine-api.open-meteo.com/):

- **Endpoint**: `https://marine-api.open-meteo.com/v1/marine`
- **Parameters**: Latitude/longitude for Cribbar, Newquay (50.426°N, 5.103°W)
- **Data**: Hourly wave height forecasts
- **Update Frequency**: Every 30 minutes
- **No API Key Required**: Free tier service

### Temperature & Humidity Sensor
The temperature mode uses a DHT11 digital sensor:

- **Sensor**: DHT11 (AM2302 compatible)
- **Accuracy**: ±2°C temperature, ±5% humidity
- **Range**: 0-50°C temperature, 20-90% humidity
- **Update Frequency**: Every 30 seconds
- **Interface**: Single-wire digital protocol

## 🔋 Power Management

- E-paper display goes to sleep mode after updates (ultra-low power consumption)
- Display refreshes every 30 seconds for both deployment types
- LED stays on to indicate system is running
- WiFi reconnection handling for surf forecast mode
- DHT11 sensor readings every 30 seconds for temperature mode

## 🛠️ Customization

### Switch Deployment Modes

**Option 1: Change build flag in `platformio.ini`:**
```ini
; Temperature & humidity mode
build_flags = -DDEPLOYMENT_TEMPERATURE_HUMIDITY

; Surf forecast mode
; build_flags = -DDEPLOYMENT_SURF_FORECAST
```

**Option 2: Use PlatformIO environments:**
```bash
# Temperature & humidity
pio run --environment temperature_humidity --target upload

# Surf forecast
pio run --environment surf_forecast --target upload
```

### Change Surf Location
Edit coordinates in `include/surf_forecast.h`:
```cpp
const float LATITUDE = 50.425998;   // Your latitude
const float LONGITUDE = -5.103096;  // Your longitude
const String LOCATION_NAME = "Your Spot Name";
```

### Configure DHT11 Sensor
Modify pin assignment in `src/main.cpp`:
```cpp
TemperatureHumiditySensor sensor(&epaperDisplay, 13);  // GPIO 13
```

### Adjust Update Intervals

**Temperature & Humidity Mode:**
- Sensor reading interval: `UPDATE_INTERVAL_MS` in `include/temperature_and_humidity.h`
- Display refresh: `DISPLAY_REFRESH_INTERVAL_MS` in `src/main.cpp`

**Surf Forecast Mode:**
- Data fetch interval: `UPDATE_INTERVAL_MS` in `include/surf_forecast.h`
- Display refresh: `DISPLAY_REFRESH_INTERVAL_MS` in `src/main.cpp`

### Modify Wave Ratings
Update rating thresholds in `SurfForecast::getRatingFromHeight()`:
```cpp
if (heightFeet < 1.0) return "FLAT";
else if (heightFeet < 2.0) return "SMALL";
// Add your custom ratings...
```

## 📝 Serial Output

The system provides detailed logging based on deployment mode:

### Temperature & Humidity Mode
```
ESP32 Modular Sensor Display Started!
Using MODULAR CODE STRUCTURE!
DHT11 sensor initialized, performing initial reading...
DHT11 raw readings - Temp: 22.00, Hum: 62.30
Valid sensor reading: 22.0°C, 62.3% RH at 14s
Display refreshed with current sensor data
```

### Surf Forecast Mode
```
ESP32 Modular Sensor Display Started!
Using MODULAR CODE STRUCTURE!
Initializing Surf Forecast...
WiFi connected! IP: 192.168.1.100
Time synchronized: 14:30:25
Data parsed - Current: 3.2ft (GOOD), Today: 2.8ft (GOOD), Tomorrow: 4.1ft (GREAT)
Display refreshed with current sensor data
```

## 🔧 Adding New Sensor Types

The modular architecture makes it easy to add new sensor deployments:

1. Create a new sensor class inheriting from `SensorInterface`
2. Implement the required methods: `begin()`, `update()`, `displayCurrentData()`, `isDataReady()`
3. Add a new build flag and PlatformIO environment
4. Update the conditional compilation in `main.cpp`

Example new sensor template:
```cpp
class MyNewSensor : public SensorInterface {
    // Implement interface methods...
};
```

## 🤝 Contributing

This modular ESP32 project welcomes contributions! Feel free to:
- Add new sensor types (air quality, light levels, etc.)
- Improve the e-paper display layouts
- Add new surf locations or APIs
- Enhance the user interface
- Report issues and suggest features

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

---

**Made with ❤️ for makers who want flexible, modular sensor displays! 🌡️🏄‍♂️🤖**
