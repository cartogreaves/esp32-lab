#ifndef SENSOR_INTERFACE_H
#define SENSOR_INTERFACE_H

#include <Arduino.h>
#include "epaper_display.h"

class SensorInterface {
public:
    virtual ~SensorInterface() {}

    // Common lifecycle methods
    virtual void begin(const char* ssid, const char* password) = 0;
    virtual void update() = 0;
    virtual void displayCurrentData() = 0;
    virtual bool isDataReady() const = 0;

    // Optional: deployment-specific methods can be added by subclasses
};

#endif
