#include <Arduino.h>
#include "../include/surf_forecast.h"
#include "../include/time_utils.h"

// Define surf locations array
static const SurfLocation surfLocations[] = {
    {50.425998, -5.103096, "Cribbar, Newquay"},
    {50.079780, -5.698678, "Sennen Cove"},
    {50.229620, -5.394250, "Gwithian"},
    {50.445738, -5.045831, "Watergate Bay"},
    {50.042637, -5.649877, "Porthcurno"},
    {51.115862, -4.227910, "Saunton Sands"},
    {51.130414, -4.238428, "Croyde Bay"}
};

const SurfLocation* SurfForecast::getSurfLocations() {
    return surfLocations;
}

int SurfForecast::getNumLocations() {
    return sizeof(surfLocations) / sizeof(surfLocations[0]);
}

SurfForecast::SurfForecast(EPaperDisplay* displayPtr) : display(displayPtr) {
}

void SurfForecast::begin(const char* ssid, const char* password) {
    Serial.println("Initializing Surf Forecast...");
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.printf("Connecting to WiFi: %s", ssid);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
        
        // Initialize NTP time sync
        TimeUtils::begin();
        
        Serial.println("Waiting for NTP time sync...");
        struct tm timeinfo;
        int attempts = 0;
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
        
        // Fetch initial data
        if (fetchForecastData()) {
            Serial.println("Initial surf data fetched successfully!");
        } else {
            Serial.println("Failed to fetch initial surf data");
        }
    } else {
        Serial.println();
        Serial.println("WiFi connection failed!");
    }
}

bool SurfForecast::fetchForecastData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot fetch data");
        return false;
    }
    
    // Get current location
    const SurfLocation* locations = getSurfLocations();
    const SurfLocation& currentLocation = locations[currentLocationIndex];
    
    HTTPClient http;
    String url = API_URL + "?latitude=" + String(currentLocation.latitude) + 
                "&longitude=" + String(currentLocation.longitude) + 
                "&hourly=wave_height";
    
    Serial.printf("Fetching: %s\n", url.c_str());
    http.begin(url);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.printf("JSON parsing error: %s\n", error.c_str());
            http.end();
            return false;
        }
        
        // Extract wave height data
        JsonArray times = doc["hourly"]["time"];
        JsonArray waveHeights = doc["hourly"]["wave_height"];
        
        if (times.size() == 0 || waveHeights.size() == 0) {
            Serial.println("No wave data received");
            http.end();
            return false;
        }
        
        // Get current conditions (first data point)
        conditions.currentWaveHeight = metersToFeet(waveHeights[0].as<float>());
        conditions.currentRating = getRatingFromHeight(waveHeights[0].as<float>());
        
        // Calculate today's average (next 12 hours)
        float todayAvg = calculateAverage(waveHeights, 1, 12);
        conditions.todayAverage = metersToFeet(todayAvg);
        conditions.todayRating = getRatingFromHeight(todayAvg);
        
        // Calculate tomorrow's average (hours 24-36)
        float tomorrowAvg = calculateAverage(waveHeights, 24, 36);
        conditions.tomorrowAverage = metersToFeet(tomorrowAvg);
        conditions.tomorrowRating = getRatingFromHeight(tomorrowAvg);
        
        // Store the timestamp when data was fetched
        lastFetchTime = TimeUtils::getCurrentTimestamp();
        
        // Set time and location (use current location)
        conditions.currentTime = lastFetchTime;
        const SurfLocation* locations = getSurfLocations();
        conditions.location = locations[currentLocationIndex].name;
        
        Serial.printf("Data parsed - Current: %.1fft (%s), Today: %.1fft (%s), Tomorrow: %.1fft (%s)\n",
                     conditions.currentWaveHeight, conditions.currentRating.c_str(),
                     conditions.todayAverage, conditions.todayRating.c_str(),
                     conditions.tomorrowAverage, conditions.tomorrowRating.c_str());
        
        http.end();
        return true;
    } else {
        Serial.printf("HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }
}

void SurfForecast::displayCurrentConditions() {
    if (!display) return;
    
    Serial.println("Displaying surf forecast on e-paper...");
    
    // Get direct access to the GxEPD2 display for proper paged drawing
    auto* gxDisplay = display->getDisplay();
    gxDisplay->setRotation(1); // Landscape orientation
    gxDisplay->setFullWindow();
    gxDisplay->firstPage();
    
    do {
        // Clear the screen
        gxDisplay->fillScreen(GxEPD_WHITE);
        gxDisplay->setTextColor(GxEPD_BLACK);
        gxDisplay->setFont();
        
        // Header - smaller and more compact (296x128 display)
        String headerText = "SURF FORECAST @ " + conditions.location;
        gxDisplay->setTextSize(1);
        gxDisplay->setCursor(2, 6);
        gxDisplay->print(headerText);
        
        // Draw horizontal line under header
        gxDisplay->drawLine(2, 20, 294, 20, GxEPD_BLACK);
        
        // Column centerlines as specified
        int col1Center = 48;   // Column 1 centerline
        int col2Center = 148;  // Column 2 centerline  
        int col3Center = 244;  // Column 3 centerline
        int colY = 40;         // Start Y position for column content
        
        // Draw vertical separators between columns
        gxDisplay->drawLine(98, 20, 98, 108, GxEPD_BLACK);   // Between col 1 & 2
        gxDisplay->drawLine(196, 20, 196, 108, GxEPD_BLACK); // Between col 2 & 3
        
        // Column 1 - NOW
        gxDisplay->setTextSize(1);
        int nowWidth = display->getTextWidth("NOW", 1);
        gxDisplay->setCursor(col1Center - nowWidth/2, colY);
        gxDisplay->print("NOW");
        
        String wave1 = String(conditions.currentWaveHeight, 1);
        int wave1Width = display->getTextWidth(wave1.c_str(), 2);
        gxDisplay->setTextSize(2);
        gxDisplay->setCursor(col1Center - wave1Width/2, colY + 15);
        gxDisplay->print(wave1);
        gxDisplay->setTextSize(1);
        gxDisplay->setCursor(col1Center + wave1Width/2 + 2, colY + 15);
        gxDisplay->print("ft");
        
        int rating1Width = display->getTextWidth(conditions.currentRating.c_str(), 1);
        gxDisplay->setCursor(col1Center - rating1Width/2, colY + 35);
        gxDisplay->print(conditions.currentRating);
        
        // Column 2 - TODAY
        int todayWidth = display->getTextWidth("TODAY", 1);
        gxDisplay->setCursor(col2Center - todayWidth/2, colY);
        gxDisplay->print("TODAY");
        
        String wave2 = String(conditions.todayAverage, 1);
        int wave2Width = display->getTextWidth(wave2.c_str(), 2);
        gxDisplay->setTextSize(2);
        gxDisplay->setCursor(col2Center - wave2Width/2, colY + 15);
        gxDisplay->print(wave2);
        gxDisplay->setTextSize(1);
        gxDisplay->setCursor(col2Center + wave2Width/2 + 2, colY + 15);
        gxDisplay->print("ft");
        
        int rating2Width = display->getTextWidth(conditions.todayRating.c_str(), 1);
        gxDisplay->setCursor(col2Center - rating2Width/2, colY + 35);
        gxDisplay->print(conditions.todayRating);
        
        // Column 3 - TOMORROW
        int tomorrowWidth = display->getTextWidth("TOMORROW", 1);
        gxDisplay->setCursor(col3Center - tomorrowWidth/2, colY);
        gxDisplay->print("TOMORROW");
        
        String wave3 = String(conditions.tomorrowAverage, 1);
        int wave3Width = display->getTextWidth(wave3.c_str(), 2);
        gxDisplay->setTextSize(2);
        gxDisplay->setCursor(col3Center - wave3Width/2, colY + 15);
        gxDisplay->print(wave3);
        gxDisplay->setTextSize(1);
        gxDisplay->setCursor(col3Center + wave3Width/2 + 2, colY + 15);
        gxDisplay->print("ft");
        
        int rating3Width = display->getTextWidth(conditions.tomorrowRating.c_str(), 1);
        gxDisplay->setCursor(col3Center - rating3Width/2, colY + 35);
        gxDisplay->print(conditions.tomorrowRating);
        
        // Draw horizontal line above footer
        gxDisplay->drawLine(2, 108, 294, 108, GxEPD_BLACK);
        
        // Footer - show last updated time
        String updateText = "Last updated: " + getCurrentTimeString();
        gxDisplay->setCursor(2, 114);
        gxDisplay->print(updateText);
        
    } while (gxDisplay->nextPage());
    
    gxDisplay->hibernate();
    Serial.println("Surf forecast displayed with proper 3-column layout!");
}

// Removed redundant display methods - keeping only displayCurrentConditions()

void SurfForecast::update() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    // Update using the global refresh interval to sync with display refresh
    if (now - lastUpdate > REFRESH_INTERVAL_MS || lastUpdate == 0) {
        // Cycle to next location each refresh
        nextLocation();
        
        Serial.printf("Updating surf forecast data for location %d/%d: %s\n", 
                     currentLocationIndex + 1, getNumLocations(), 
                     getSurfLocations()[currentLocationIndex].name.c_str());
        
        if (fetchForecastData()) {
            Serial.println("Surf data updated successfully");
        } else {
            Serial.println("Failed to update surf data");
        }
        lastUpdate = now;
    }
}

void SurfForecast::nextLocation() {
    currentLocationIndex = (currentLocationIndex + 1) % getNumLocations();
}

bool SurfForecast::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

// Helper method implementations
String SurfForecast::getRatingFromHeight(float heightMeters) {
    float heightFeet = metersToFeet(heightMeters);
    
    if (heightFeet < 1.0) return "FLAT";
    else if (heightFeet < 2.0) return "SMALL";
    else if (heightFeet < 4.0) return "GOOD";
    else if (heightFeet < 6.0) return "GREAT";
    else if (heightFeet < 8.0) return "EPIC";
    else return "HUGE";
}

float SurfForecast::metersToFeet(float meters) {
    return meters * 3.28084;
}

float SurfForecast::calculateAverage(JsonArray& heights, int startHour, int endHour) {
    float sum = 0;
    int count = 0;
    
    for (int i = startHour; i < endHour && i < heights.size(); i++) {
        sum += heights[i].as<float>();
        count++;
    }
    
    return count > 0 ? sum / count : 0;
}

String SurfForecast::getCurrentTimeString() {
    // Return the stored UK time from when data was last fetched
    return lastFetchTime.isEmpty() ? "??:??:??" : lastFetchTime;
}

// SensorInterface implementation
void SurfForecast::displayCurrentData() {
    displayCurrentConditions();
}

bool SurfForecast::isDataReady() const {
    return !lastFetchTime.isEmpty() && WiFi.status() == WL_CONNECTED;
}
