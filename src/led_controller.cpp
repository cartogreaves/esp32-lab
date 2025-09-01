#include <Arduino.h>
#include "../include/led_controller.h"

LEDController::LEDController(int ledPin) : pin(ledPin), state(false) {
}

void LEDController::begin() {
    pinMode(pin, OUTPUT);
    off(); // Start with LED off
    Serial.printf("LED Controller initialized on GPIO %d\n", pin);
}

void LEDController::on() {
    digitalWrite(pin, HIGH);
    state = true;
    Serial.println("LED ON");
}

void LEDController::off() {
    digitalWrite(pin, LOW);
    state = false;
    Serial.println("LED OFF");
}

void LEDController::toggle() {
    if (state) {
        off();
    } else {
        on();
    }
}

void LEDController::flash(int delayMs) {
    on();
    delay(delayMs);
    off();
    delay(delayMs);
}

bool LEDController::getState() const {
    return state;
}
