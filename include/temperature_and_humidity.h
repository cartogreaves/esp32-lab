#ifndef TEMPERATURE_AND_HUMIDITY_H
#define TEMPERATURE_AND_HUMIDITY_H

#include <Arduino.h>
#include <DHT.h>
#include "epaper_display.h"
#include "sensor_interface.h"

struct TempHumidityData {
    float temperature;
    float humidity;
    String lastUpdateTime;
    bool sensorError;
};

class TemperatureHumiditySensor : public SensorInterface {
private:
    EPaperDisplay* display;
    DHT* dhtSensor;
    TempHumidityData currentData;

    int dhtPin;
    uint8_t dhtType;
    unsigned long lastUpdateTime;
    const unsigned long UPDATE_INTERVAL_MS = 30000; // 30 seconds
    bool initialized;

    void readSensor();
    void updateDisplay();

public:
    TemperatureHumiditySensor(EPaperDisplay* displayPtr, int sensorPin = 26, uint8_t sensorType = DHT11);
    // sensorPin: GPIO pin connected to DHT sensor data pin (default: 26)
    // sensorType: DHT sensor type - DHT11, DHT21, DHT22, etc. (default: DHT11)
    virtual ~TemperatureHumiditySensor();

    // Implement SensorInterface
    void begin(const char* ssid, const char* password) override;
    void update() override;
    void displayCurrentData() override;
    bool isDataReady() const override;

    // Sensor-specific methods
    TempHumidityData getCurrentData() const;
    bool isSensorWorking() const;
};

#endif
