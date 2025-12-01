#ifndef EPAPER_DISPLAY_H
#define EPAPER_DISPLAY_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold12pt7b.h>

class EPaperDisplay {
private:
    GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT>* display;
    int csPin, dcPin, rstPin, busyPin;
    
public:
    EPaperDisplay(int cs, int dc, int rst, int busy);
    ~EPaperDisplay(); // Destructor to clean up memory
    void begin();
    void clear();
    void fillScreen(uint16_t color);
    void showText(const char* text, int x = 10, int y = 30, int textSize = 3);
    void showHelloWorld();
    void testDisplay();
    void sleep();
    void printPinAssignments();
    
    // Advanced layout methods for surf forecast
    void startUpdate();
    void drawText(const char* text, int x, int y, int textSize = 1);
    void drawLine(int x1, int y1, int x2, int y2);
    void finishUpdate();
    int getTextWidth(const char* text, int textSize = 1);
    GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT>* getDisplay(); // Direct access for advanced drawing
};

#endif
