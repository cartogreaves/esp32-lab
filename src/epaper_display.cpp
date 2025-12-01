#include <Arduino.h>
#include "../include/epaper_display.h"

EPaperDisplay::EPaperDisplay(int cs, int dc, int rst, int busy) 
    : csPin(cs), dcPin(dc), rstPin(rst), busyPin(busy) {
    display = new GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT>(GxEPD2_290_BS(cs, dc, rst, busy));
}

EPaperDisplay::~EPaperDisplay() {
    delete display;
}

void EPaperDisplay::begin() {
    Serial.println("Initializing e-paper display with CORRECTED driver...");
    
    display->init(115200); // Enable diagnostic output
    Serial.println("Display init completed");
    
    printPinAssignments();
}

void EPaperDisplay::printPinAssignments() {
    Serial.printf("E-Paper Pin assignments:\n");
    Serial.printf("  CS: GPIO %d\n", csPin);
    Serial.printf("  DC: GPIO %d\n", dcPin);
    Serial.printf("  RST: GPIO %d\n", rstPin);
    Serial.printf("  BUSY: GPIO %d\n", busyPin);
    Serial.printf("  MOSI: GPIO 23 (default SPI)\n");
    Serial.printf("  SCK: GPIO 18 (default SPI)\n");
    Serial.println("*** AVOIDING GPIO16/17 - they are used for PSRAM! ***");
}

void EPaperDisplay::clear() {
    display->setFullWindow();
    display->firstPage();
    do {
        display->fillScreen(GxEPD_WHITE);
    } while (display->nextPage());
    Serial.println("Display cleared");
}

void EPaperDisplay::fillScreen(uint16_t color) {
    display->setFullWindow();
    display->firstPage();
    do {
        display->fillScreen(color);
    } while (display->nextPage());
    Serial.printf("Screen filled with color: %d\n", color);
}

void EPaperDisplay::showText(const char* text, int x, int y, int textSize) {
    display->setRotation(1); // Landscape orientation
    display->setFullWindow();
    display->firstPage();
    
    do {
        display->fillScreen(GxEPD_WHITE);
        display->setTextColor(GxEPD_BLACK);
        display->setFont(); // Use default font
        display->setTextSize(textSize);
        display->setCursor(x, y);
        display->print(text);
    } while (display->nextPage());
    
    Serial.printf("Displayed text: '%s' at (%d, %d) with size %d\n", text, x, y, textSize);
}

void EPaperDisplay::showHelloWorld() {
    Serial.println("Displaying HELLO WORLD on e-paper...");
    
    display->setRotation(1); // Landscape orientation
    display->setFullWindow();
    display->firstPage();
    
    do {
        display->fillScreen(GxEPD_WHITE);
        display->setTextColor(GxEPD_BLACK);
        display->setFont(); // Use default font
        display->setTextSize(3); // Make it bigger
        display->setCursor(10, 30);
        display->print("HELLO");
        display->setCursor(10, 80);
        display->print("JACQUI");
        
        Serial.println("Drawing text to display buffer");
    } while (display->nextPage());
    
    sleep();
    Serial.println("E-paper display update completed!");
}

void EPaperDisplay::testDisplay() {
    Serial.println("Testing display with simple fill...");
    display->setRotation(0); // Portrait orientation
    fillScreen(GxEPD_BLACK); // Fill with black first
    Serial.println("Filled screen black");
    
    delay(3000); // Wait 3 seconds to see black screen
    
    // Now show hello world
    showHelloWorld();
}

void EPaperDisplay::sleep() {
    display->hibernate(); // Put display in low power mode
    Serial.println("Display put to sleep");
}

// Advanced layout methods for complex displays
// NOTE: These helper methods are for simple single-page content only.
// For complex multi-element layouts (like surf forecast), use getDisplay() 
// and manage your own do...while(nextPage()) loop.

void EPaperDisplay::startUpdate() {
    display->setRotation(1); // Landscape orientation
    display->setFullWindow();
    display->firstPage();
    // Clear the screen at the start
    display->fillScreen(GxEPD_WHITE);
    display->setTextColor(GxEPD_BLACK);
    display->setFont();
}

void EPaperDisplay::drawText(const char* text, int x, int y, int textSize) {
    display->setTextSize(textSize);
    display->setCursor(x, y);
    display->print(text);
}

void EPaperDisplay::drawLine(int x1, int y1, int x2, int y2) {
    display->drawLine(x1, y1, x2, y2, GxEPD_BLACK);
}

void EPaperDisplay::finishUpdate() {
    // For simple single-page updates, just complete the current page
    // This assumes startUpdate() was called and content was drawn
    // Note: For paged displays, the do...while loop should be managed by the caller
    // if they need to redraw content on each page
    display->nextPage(); // Complete the single page update
    display->hibernate(); // Put display in low power mode
}

int EPaperDisplay::getTextWidth(const char* text, int textSize) {
    display->setFont(); // Use default font
    display->setTextSize(textSize);
    
    int16_t x1, y1;
    uint16_t w, h;
    display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    
    return w;
}

GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT>* EPaperDisplay::getDisplay() {
    return display;
}
