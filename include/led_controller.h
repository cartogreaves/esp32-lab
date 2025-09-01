#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

class LEDController {
private:
    int pin;
    bool state;
    
public:
    LEDController(int ledPin);
    void begin();
    void on();
    void off();
    void toggle();
    void flash(int delayMs = 1000);
    bool getState() const;
};

#endif
