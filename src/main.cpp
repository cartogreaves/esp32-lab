#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>
#include "../include/led_controller.h"
#include "../include/epaper_display.h"
#include "../include/time_utils.h"

// Constants for magic numbers
const int LED_PIN_VALUE = 25;

// E-paper pin assignments
// Camera mode uses GPIO 12 for DC (freeing GPIO 14 for HSPI_CLK)
// Non-camera modes keep GPIO 14 for DC (GPIO 18/23 available for default VSPI)
#ifdef DEPLOYMENT_CAMERA
const int EPD_CS_VALUE = 32;
const int EPD_DC_VALUE = 12;   // Moved from 14 to free it for HSPI_CLK
const int EPD_RST_VALUE = 2;
const int EPD_BUSY_VALUE = 33;
#else
const int EPD_CS_VALUE = 32;
const int EPD_DC_VALUE = 14;
const int EPD_RST_VALUE = 2;
const int EPD_BUSY_VALUE = 33;
#endif
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long INITIAL_DELAY_MS = 1000;
const int DHT_SENSOR_PIN = 13;
const unsigned long LOOP_DELAY_MS = 10000;
const unsigned long DISPLAY_REFRESH_INTERVAL_MS = 30000;

// Deployment mode selection via build flags
// Available modes: DEPLOYMENT_TEMPERATURE_HUMIDITY, DEPLOYMENT_SURF_FORECAST, or DEPLOYMENT_CAMERA
// Configure in platformio.ini with: build_flags = -DDEPLOYMENT_TEMPERATURE_HUMIDITY
// Or: build_flags = -DDEPLOYMENT_SURF_FORECAST
// Or: build_flags = -DDEPLOYMENT_CAMERA

// Include the appropriate sensor header based on deployment mode
#ifdef DEPLOYMENT_TEMPERATURE_HUMIDITY
#include "../include/temperature_and_humidity.h"
#include "../include/sensor_interface.h"
#include "../include/datalogger.h"
#endif

#ifdef DEPLOYMENT_SURF_FORECAST
#include "../include/surf_forecast.h"
#include "../include/sensor_interface.h"
#endif

#ifdef DEPLOYMENT_CAMERA
#include <SPI.h>
#include "../include/camera_capture.h"
#include "../include/sensor_interface.h"
// HSPI bus for e-paper display - completely independent from camera's I2S
// Uses HSPI native pins: SCK=GPIO14, MOSI=GPIO13 (no conflict with camera)
SPIClass displaySPI(HSPI);
#endif

// Pin definitions (using constexpr instead of macros)
constexpr int LED_PIN = LED_PIN_VALUE;

// E-paper display pin definitions (SAFE pins for Freenove ESP32 Wrover)
constexpr int EPD_CS = EPD_CS_VALUE;      // Chip Select
constexpr int EPD_DC = EPD_DC_VALUE;      // Data/Command
constexpr int EPD_RST = EPD_RST_VALUE;    // Reset
constexpr int EPD_BUSY = EPD_BUSY_VALUE;  // Busy signal

// Environment variables are loaded from .env file at runtime
// These pointers will point to the loaded values
const char* WIFI_SSID;
const char* WIFI_PASSWORD;
const char* SUPABASE_URL;
const char* SUPABASE_API_KEY;

// Create module instances
LEDController led(LED_PIN);
EPaperDisplay epaperDisplay(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

// DataLogger is only needed for modules that log to Supabase
#if defined(DEPLOYMENT_TEMPERATURE_HUMIDITY) || defined(DEPLOYMENT_SURF_FORECAST)
DataLogger datalogger;
#endif

// Create sensor instance based on deployment mode
#ifdef DEPLOYMENT_TEMPERATURE_HUMIDITY
TemperatureHumiditySensor sensor(&epaperDisplay, &datalogger, DHT_SENSOR_PIN);  // DHT11 on pin 13
#endif

#ifdef DEPLOYMENT_SURF_FORECAST
SurfForecast sensor(&epaperDisplay, &datalogger);  // Surf forecast with WiFi
#endif

#ifdef DEPLOYMENT_CAMERA
CameraCapture sensor(&epaperDisplay);  // Camera capture module
#endif

// Default environment variable values
const String DEFAULT_WIFI_SSID = "default_ssid";
const String DEFAULT_WIFI_PASSWORD = "default_password";
const String DEFAULT_SUPABASE_URL = "https://your-project.supabase.co";
const String DEFAULT_SUPABASE_API_KEY = "your-anon-key";
const int DEFAULT_BATCH_SIZE = 5;
const unsigned long DEFAULT_LOG_INTERVAL_MS = 300000;

// Environment variable storage
String envWifiSSID = DEFAULT_WIFI_SSID;
String envWifiPassword = DEFAULT_WIFI_PASSWORD;
String envSupabaseURL = DEFAULT_SUPABASE_URL;
String envSupabaseAPIKey = DEFAULT_SUPABASE_API_KEY;
int envBatchSize = DEFAULT_BATCH_SIZE;
unsigned long envLogInterval = DEFAULT_LOG_INTERVAL_MS;

// Function to load environment variables from .env file
void loadEnvironmentVariables() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    File envFile = SPIFFS.open("/.env", "r");
    if (!envFile) {
        Serial.println("Failed to open .env file");
        return;
    }

    Serial.println("Loading environment variables from .env file...");

    while (envFile.available()) {
        String line = envFile.readStringUntil('\n');
        line.trim();

        // Skip comments and empty lines
        if (line.startsWith("#") || line.length() == 0) {
            continue;
        }

        // Parse key=value pairs
        int separatorIndex = line.indexOf('=');
        if (separatorIndex > 0) {
            String key = line.substring(0, separatorIndex);
            String value = line.substring(separatorIndex + 1);

            // Remove quotes if present
            value.replace("\"", "");
            value.replace("'", "");

            // Store values based on key
            if (key == "WIFI_SSID") {
                envWifiSSID = value;
            } else if (key == "WIFI_PASSWORD") {
                envWifiPassword = value;
            } else if (key == "SUPABASE_URL") {
                envSupabaseURL = value;
            } else if (key == "SUPABASE_API_KEY") {
                envSupabaseAPIKey = value;
            } else if (key == "DATALOGGER_BATCH_SIZE") {
                envBatchSize = value.toInt();
            } else if (key == "DATALOGGER_LOG_INTERVAL") {
                envLogInterval = strtoul(value.c_str(), nullptr, 10);
            }
        }
    }

    envFile.close();
    Serial.println("Environment variables loaded successfully");
}

// put function declarations here:
int myFunction(int, int);

void setup() {
    // put your setup code here, to run once:

    // Initialize serial communication for debugging
    Serial.begin(SERIAL_BAUD_RATE);
    delay(INITIAL_DELAY_MS); // Give serial time to initialize
    Serial.println("ESP32 Modular Sensor Display Started!");
    Serial.println("Using MODULAR CODE STRUCTURE!");

    // Load environment variables from .env file
    loadEnvironmentVariables();

    // Assign loaded values to const pointers
    WIFI_SSID = envWifiSSID.c_str();
    WIFI_PASSWORD = envWifiPassword.c_str();
    SUPABASE_URL = envSupabaseURL.c_str();
    SUPABASE_API_KEY = envSupabaseAPIKey.c_str();
    
    // Initialize LED controller
    led.begin();

    // Initialize NTP time sync
    TimeUtils::begin();

    // Initialize e-paper display
#ifdef DEPLOYMENT_CAMERA
    // Camera mode: use HSPI on dedicated pins (SCK=14, MOSI=13)
    // These pins are NOT used by the camera, so no conflict at all
    displaySPI.begin(14, -1, 13, -1);
    epaperDisplay.begin(displaySPI);
#else
    // Non-camera modes: default VSPI (SCK=18, MOSI=23)
    epaperDisplay.begin();
#endif
    epaperDisplay.showText("Starting...", 10, 30, 2);

    // Initialize sensor (connects to WiFi and fetches/reads data)
    sensor.begin(WIFI_SSID, WIFI_PASSWORD);

    // Initialize datalogger (used by temperature_humidity and surf_forecast modes)
#if defined(DEPLOYMENT_TEMPERATURE_HUMIDITY) || defined(DEPLOYMENT_SURF_FORECAST)
    SupabaseConfig config;
    config.url = SUPABASE_URL;
    config.apiKey = SUPABASE_API_KEY;
    config.batchSize = envBatchSize;
    config.logInterval = envLogInterval;

    datalogger.setConfig(config);
    datalogger.begin();
#endif

    // Display initial sensor data
    sensor.displayCurrentData();
    
    int result = myFunction(2, 3);
    Serial.printf("myFunction result: %d\n", result);
    
    Serial.println("Setup completed! Starting main loop...");
}

void loop() {
    // put your main code here, to run repeatedly:

    // Keep LED on to show the ESP32 is running
    led.on();

    // Update sensor data periodically (handles its own timing)
    sensor.update();

    // Update datalogger (periodic batch sending)
#if defined(DEPLOYMENT_TEMPERATURE_HUMIDITY) || defined(DEPLOYMENT_SURF_FORECAST)
    datalogger.update();
#endif

    
    // Refresh display every 30 seconds when sensor data is ready
    static unsigned long lastDisplayUpdate = 0;
    unsigned long currentTime = millis();

    // Simple refresh logic: update every 30 seconds after first display
    bool shouldRefresh = sensor.isDataReady() &&
        (lastDisplayUpdate == 0 ||
         (currentTime - lastDisplayUpdate) >= DISPLAY_REFRESH_INTERVAL_MS);

    if (shouldRefresh) {
        sensor.displayCurrentData();
        Serial.println("Display refreshed with current sensor data");
        lastDisplayUpdate = currentTime;
    }

    delay(LOOP_DELAY_MS); // Check every 10 seconds
}

// put function definitions here:
int myFunction(int x, int y) {
    return x + y;
}