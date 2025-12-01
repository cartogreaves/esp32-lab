#ifndef SURF_FORECAST_H
#define SURF_FORECAST_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "epaper_display.h"
#include "sensor_interface.h"

// Global refresh interval for both data fetch and display update (in milliseconds)
const unsigned long REFRESH_INTERVAL_MS = 60000; // 1 minute

struct SurfLocation {
    float latitude;
    float longitude;
    String name;
};

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

class SurfForecast : public SensorInterface {
private:
    EPaperDisplay* display;
    SurfConditions conditions;
    String lastFetchTime; // Store the UK time when data was last fetched
    
    // API settings
    const String API_URL = "https://marine-api.open-meteo.com/v1/marine";
    
    static const SurfLocation* getSurfLocations();
    static int getNumLocations();
    int currentLocationIndex = 0;
    
    // Helper methods
    String getRatingFromHeight(float heightMeters);
    float metersToFeet(float meters);
    float calculateAverage(JsonArray& heights, int startHour, int endHour);
    String getCurrentTimeString();
    // Removed unused helper methods
    
public:
    SurfForecast(EPaperDisplay* displayPtr);
    virtual ~SurfForecast() {}

    // Implement SensorInterface
    void begin(const char* ssid, const char* password) override;
    void update() override;
    void displayCurrentData() override;
    bool isDataReady() const override;

    // SurfForecast-specific methods
    bool fetchForecastData();
    void displayCurrentConditions(); // Legacy method for backward compatibility
    bool isWiFiConnected();
    void nextLocation();
};

#endif
