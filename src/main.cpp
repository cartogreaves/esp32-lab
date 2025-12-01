#include <Arduino.h>
#include "../include/led_controller.h"
#include "../include/epaper_display.h"
#include "../include/time_utils.h"

// Deployment mode selection via build flags
// Available modes: DEPLOYMENT_TEMPERATURE_HUMIDITY or DEPLOYMENT_SURF_FORECAST
// Configure in platformio.ini with: build_flags = -DDEPLOYMENT_TEMPERATURE_HUMIDITY
// Or: build_flags = -DDEPLOYMENT_SURF_FORECAST

// Include the appropriate sensor header based on deployment mode
#ifdef DEPLOYMENT_TEMPERATURE_HUMIDITY
#include "../include/temperature_and_humidity.h"
#include "../include/sensor_interface.h"
#endif

#ifdef DEPLOYMENT_SURF_FORECAST
#include "../include/surf_forecast.h"
#include "../include/sensor_interface.h"
#endif

// Pin definitions
#define LED_PIN 25

// E-paper display pin definitions (SAFE pins for Freenove ESP32 Wrover)
#define EPD_CS     5    // Chip Select
#define EPD_DC     2    // Data/Command
#define EPD_RST    15   // Reset
#define EPD_BUSY   4    // Busy signal

// WiFi credentials (replace with your network)
const char* WIFI_SSID = "EE-38NJQK";
const char* WIFI_PASSWORD = "uT6TAJ7wtRVYgquP";

// Create module instances
LEDController led(LED_PIN);
EPaperDisplay epaperDisplay(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

// Create sensor instance based on deployment mode
#ifdef DEPLOYMENT_TEMPERATURE_HUMIDITY
TemperatureHumiditySensor sensor(&epaperDisplay, 13);  // DHT11 on pin 13
#endif

#ifdef DEPLOYMENT_SURF_FORECAST
SurfForecast sensor(&epaperDisplay);  // Surf forecast with WiFi
#endif

// put function declarations here:
int myFunction(int, int);

void setup() {
    // put your setup code here, to run once:
    
    // Initialize serial communication for debugging
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize
    Serial.println("ESP32 Modular Sensor Display Started!");
    Serial.println("Using MODULAR CODE STRUCTURE!");
    
    // Initialize LED controller
    led.begin();

    // Initialize NTP time sync
    TimeUtils::begin();

    // Initialize e-paper display
    epaperDisplay.begin();
    
    // Show initial message
    epaperDisplay.showText("Starting...", 10, 30, 2);
    
    // Initialize sensor (connects to WiFi and fetches/reads data)
    sensor.begin(WIFI_SSID, WIFI_PASSWORD);

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

    
    // Refresh display every 30 seconds when sensor data is ready
    static unsigned long lastDisplayUpdate = 0;
    unsigned long currentTime = millis();
    const unsigned long DISPLAY_REFRESH_INTERVAL_MS = 30000; // 30 seconds

    // Simple refresh logic: update every 30 seconds after first display
    bool shouldRefresh = sensor.isDataReady() &&
        (lastDisplayUpdate == 0 ||
         (currentTime - lastDisplayUpdate) >= DISPLAY_REFRESH_INTERVAL_MS);

    if (shouldRefresh) {
        sensor.displayCurrentData();
        Serial.println("Display refreshed with current sensor data");
        lastDisplayUpdate = currentTime;
    }
    
    delay(10000); // Check every 10 seconds
}

// put function definitions here:
int myFunction(int x, int y) {
    return x + y;
}