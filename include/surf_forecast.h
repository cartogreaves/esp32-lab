#ifndef SURF_FORECAST_H
#define SURF_FORECAST_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "epaper_display.h"

struct SurfConditions {
    float currentWaveHeight;
    float todayAverage;
    float tomorrowAverage;
    String currentRating;
    String todayRating;
    String tomorrowRating;
    String currentTime;
    String location;
};

class SurfForecast {
private:
    EPaperDisplay* display;
    SurfConditions conditions;
    String lastFetchTime; // Store the UK time when data was last fetched
    
    // API settings - Cribbar, Newquay (famous big wave surf spot)
    const String API_URL = "https://marine-api.open-meteo.com/v1/marine";
    const float LATITUDE = 50.425998;
    const float LONGITUDE = -5.103096;
    const String LOCATION_NAME = "Cribbar, Newquay";
    
    // Helper methods
    String getRatingFromHeight(float heightMeters);
    float metersToFeet(float meters);
    float calculateAverage(JsonArray& heights, int startHour, int endHour);
    String getCurrentTimeString();
    // Removed unused helper methods
    
public:
    SurfForecast(EPaperDisplay* displayPtr);
    void begin(const char* ssid, const char* password);
    bool fetchForecastData();
    void displayCurrentConditions();
    void update();
    bool isWiFiConnected();
};

#endif
