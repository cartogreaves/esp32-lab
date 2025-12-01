#include <Arduino.h>
#include <WiFi.h>
#include <GxEPD2_BW.h>
#include "../include/temperature_and_humidity.h"
#include "../include/time_utils.h"

TemperatureHumiditySensor::TemperatureHumiditySensor(EPaperDisplay* displayPtr, int sensorPin, uint8_t sensorType)
    : display(displayPtr), dhtPin(sensorPin), dhtType(sensorType), initialized(false) {
    dhtSensor = new DHT(sensorPin, sensorType);
    currentData = {0.0f, 0.0f, "", true};
}

TemperatureHumiditySensor::~TemperatureHumiditySensor() {
    if (dhtSensor) {
        delete dhtSensor;
    }
}

void TemperatureHumiditySensor::begin(const char* ssid, const char* password) {
    Serial.println("Initializing Temperature & Humidity Sensor...");

    // Connect to WiFi for NTP time sync
    if (ssid != nullptr && strlen(ssid) > 0) {
        Serial.printf("Connecting to WiFi: %s", ssid);
        WiFi.begin(ssid, password);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());

            // Wait for NTP time sync
            Serial.println("Waiting for NTP time sync...");
            struct tm timeinfo;
            attempts = 0;
            while (!getLocalTime(&timeinfo) && attempts < 10) {
                delay(1000);
                attempts++;
                Serial.print(".");
            }

            if (attempts < 10) {
                Serial.println();
                Serial.printf("Time synchronized: %02d:%02d:%02d\n",
                             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            } else {
                Serial.println();
                Serial.println("Failed to sync time with NTP");
            }
        } else {
            Serial.println();
            Serial.println("WiFi connection failed - using fallback timestamps");
        }
    }

    // Initialize DHT sensor
    dhtSensor->begin();
    initialized = true;

    Serial.println("DHT11 sensor initialized, performing initial reading...");

    // Initial sensor reading
    readSensor();

    Serial.println("Temperature & Humidity Sensor initialized!");
}

void TemperatureHumiditySensor::update() {
    if (!initialized) return;

    unsigned long currentTime = millis();

    // Handle millis() overflow (every ~49.7 days)
    if (currentTime < lastUpdateTime) {
        lastUpdateTime = currentTime;
    }

    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL_MS) {
        readSensor();
        lastUpdateTime = currentTime;
    }
}

void TemperatureHumiditySensor::readSensor() {
    // Similar to MicroPython: measure() then read values
    float temp = dhtSensor->readTemperature(); // Celsius
    float hum = dhtSensor->readHumidity();

    Serial.printf("DHT11 raw readings - Temp: %.2f, Hum: %.2f\n", temp, hum);

    // Check if readings are valid (similar to checking for None/null in MicroPython)
    if (isnan(temp) || isnan(hum) || temp < -40 || temp > 80 || hum < 0 || hum > 100) {
        Serial.println("Invalid DHT11 reading - sensor may be faulty or wiring issue");
        currentData.sensorError = true;
        currentData.temperature = 0.0f;
        currentData.humidity = 0.0f;
        currentData.lastUpdateTime = "ERROR";
        return;
    }

    currentData.temperature = temp;
    currentData.humidity = hum;
    currentData.sensorError = false;

    // Update timestamp
    currentData.lastUpdateTime = TimeUtils::getCurrentTimestamp();

    Serial.printf("Valid sensor reading: %.1f°C, %.1f%% RH at %s\n",
                  temp, hum, currentData.lastUpdateTime.c_str());
}

void TemperatureHumiditySensor::displayCurrentData() {
    if (!display || !initialized) return;

    Serial.println("Updating e-paper display...");

    // Get direct access to the GxEPD2 display for proper paged drawing
    auto* gxDisplay = display->getDisplay();
    gxDisplay->setRotation(1); // Landscape orientation
    gxDisplay->setFullWindow();
    gxDisplay->firstPage();

    do {
        gxDisplay->fillScreen(GxEPD_WHITE);
        gxDisplay->setTextColor(GxEPD_BLACK);
        gxDisplay->setFont();

        if (currentData.sensorError) {
            gxDisplay->setTextSize(2);
            gxDisplay->setCursor(10, 20);
            gxDisplay->print("Sensor Error!");
            gxDisplay->setTextSize(1);
            gxDisplay->setCursor(10, 50);
            gxDisplay->print("Check connections");
        } else {
            // Header - "How moist is our home?"
            gxDisplay->setTextSize(1.5);
            gxDisplay->setCursor(2, 6);
            gxDisplay->print("How moist is our home?");

            // Draw horizontal line under header
            gxDisplay->drawLine(2, 20, 294, 20, GxEPD_BLACK);

            // Column centerlines for 2-column layout (296px width)
            // Each column is 148px wide, centered at 74 and 222
            int col1Center = 74;   // Column 1 centerline (TEMPERATURE) - 296/4
            int col2Center = 222;  // Column 2 centerline (HUMIDITY) - 296*3/4
            int colY = 40;         // Start Y position for column content

            // Draw vertical separator between columns at center of display
            gxDisplay->drawLine(148, 20, 148, 108, GxEPD_BLACK); // Center line at 296/2

            // Column 1 - TEMPERATURE
            gxDisplay->setTextSize(1);
            int tempHeaderWidth = display->getTextWidth("TEMPERATURE", 1);
            gxDisplay->setCursor(col1Center - tempHeaderWidth/2, colY);
            gxDisplay->print("TEMPERATURE");

            // Temperature value
            char tempStr[20];
            sprintf(tempStr, "%.1f", currentData.temperature);
            int tempValueWidth = display->getTextWidth(tempStr, 3);
            gxDisplay->setTextSize(3);
            gxDisplay->setCursor(col1Center - tempValueWidth/2, colY + 15);
            gxDisplay->print(tempStr);

            // Temperature unit
            gxDisplay->setTextSize(1);
            gxDisplay->setCursor(col1Center + tempValueWidth/2 + 2, colY + 15);
            gxDisplay->print("C");

            // Column 2 - HUMIDITY
            int humHeaderWidth = display->getTextWidth("HUMIDITY", 1);
            gxDisplay->setCursor(col2Center - humHeaderWidth/2, colY);
            gxDisplay->print("HUMIDITY");

            // Humidity value
            char humStr[20];
            sprintf(humStr, "%.1f", currentData.humidity);
            int humValueWidth = display->getTextWidth(humStr, 3);
            gxDisplay->setTextSize(3);
            gxDisplay->setCursor(col2Center - humValueWidth/2, colY + 15);
            gxDisplay->print(humStr);

            // Humidity unit
            gxDisplay->setTextSize(1);
            gxDisplay->setCursor(col2Center + humValueWidth/2 + 2, colY + 15);
            gxDisplay->print("%");

            // Draw horizontal line above footer
            gxDisplay->drawLine(2, 108, 294, 108, GxEPD_BLACK);

            // Footer - show last updated time (same format as surf forecast)
            String timeStr = currentData.lastUpdateTime.isEmpty() ? "??:??:?? Unknown Date" :
                           (currentData.lastUpdateTime.endsWith("s ago") ? currentData.lastUpdateTime : currentData.lastUpdateTime);
            String updateText = "Last updated: " + timeStr;
            gxDisplay->setTextSize(1.5);
            gxDisplay->setCursor(2, 114);
            gxDisplay->print(updateText);
        }
    } while (gxDisplay->nextPage());

    gxDisplay->hibernate();
    Serial.println("E-paper display updated successfully");
}

bool TemperatureHumiditySensor::isDataReady() const {
    return initialized && !currentData.sensorError && !currentData.lastUpdateTime.isEmpty();
}

TempHumidityData TemperatureHumiditySensor::getCurrentData() const {
    return currentData;
}

bool TemperatureHumiditySensor::isSensorWorking() const {
    return initialized && !currentData.sensorError;
}
