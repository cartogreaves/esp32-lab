#include <Arduino.h>
#include "../include/led_controller.h"
#include "../include/epaper_display.h"
#include "../include/surf_forecast.h"

// Pin definitions
#define LED_PIN 25

// E-paper display pin definitions (SAFE pins for Freenove ESP32 Wrover)
#define EPD_CS     5    // Chip Select
#define EPD_DC     2    // Data/Command
#define EPD_RST    15   // Reset
#define EPD_BUSY   4    // Busy signal

// WiFi credentials (replace with your network)
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// Create module instances
LEDController led(LED_PIN);
EPaperDisplay epaperDisplay(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
SurfForecast surfForecast(&epaperDisplay); // Pass display reference

// put function declarations here:
int myFunction(int, int);

void setup() {
    // put your setup code here, to run once:
    
    // Initialize serial communication for debugging
    Serial.begin(115200);
    delay(1000); // Give serial time to initialize
    Serial.println("ESP32 Surf Forecast Display Started!");
    Serial.println("Using MODULAR CODE STRUCTURE!");
    
    // Initialize LED controller
    led.begin();
    
    // Initialize e-paper display
    epaperDisplay.begin();
    
    // Show initial message
    epaperDisplay.showText("Starting...", 10, 30, 2);
    
    // Initialize surf forecast (connects to WiFi and fetches data)
    surfForecast.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Display initial surf conditions
    surfForecast.displayCurrentConditions();
    
    int result = myFunction(2, 3);
    Serial.printf("myFunction result: %d\n", result);
    
    Serial.println("Setup completed! Starting main loop...");
}

void loop() {
    // put your main code here, to run repeatedly:
    
    // Keep LED on to show the ESP32 is running
    led.on();
    
    // Update surf data periodically (handles its own timing)
    surfForecast.update();
    
    // Refresh display every 5 minutes to show any new data
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 300000 || lastDisplayUpdate == 0) { // 5 minutes
        surfForecast.displayCurrentConditions();
        Serial.println("Display refreshed with current surf conditions");
        lastDisplayUpdate = millis();
    }
    
    delay(10000); // Check every 10 seconds
}

// put function definitions here:
int myFunction(int x, int y) {
    return x + y;
}