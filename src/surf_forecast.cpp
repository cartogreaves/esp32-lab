#include <Arduino.h>
#include "../include/surf_forecast.h"

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
        
        // Configure NTP for UK time (GMT/BST automatically handled)
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1);
        tzset();
        
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
    
    HTTPClient http;
    String url = API_URL + "?latitude=" + String(LATITUDE) + 
                "&longitude=" + String(LONGITUDE) + 
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
        
        // Store the UK time when data was fetched
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            char timeStr[9];
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            lastFetchTime = String(timeStr);
        } else {
            lastFetchTime = "??:??:??";
        }
        
        // Set time and location (use constant from header)
        conditions.currentTime = lastFetchTime;
        conditions.location = LOCATION_NAME;
        
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
    
    // Start a single display update cycle
    display->startUpdate();
    
    do {
        // Clear the screen
        display->getDisplay()->fillScreen(GxEPD_WHITE);
        
        // Header - smaller and more compact (296x128 display)
        String headerText = "SURF FORECAST @ " + LOCATION_NAME;
        display->drawText(headerText.c_str(), 2, 6, 1.5);
        
        // Draw horizontal line under header
        display->drawLine(2, 20, 294, 20);
        
        // Column centerlines as specified
        int col1Center = 48;   // Column 1 centerline
        int col2Center = 148;  // Column 2 centerline  
        int col3Center = 244;  // Column 3 centerline
        int colY = 30;         // Start Y position for column content
        
        // Draw vertical separators between columns
        display->drawLine(98, 20, 98, 108);   // Between col 1 & 2
        display->drawLine(196, 20, 196, 108); // Between col 2 & 3
        
        // Column 1 - NOW (centered on x=48)
        display->drawText("NOW", col1Center - 9, colY, 1);  // "NOW" is ~18px wide, so -9 to center
        String wave1 = String(conditions.currentWaveHeight, 1);
        display->drawText(wave1.c_str(), col1Center - 8, colY + 15, 2);  // Number centered
        display->drawText("ft", col1Center + 8, colY + 15, 1);  // "ft" offset to right
        // Center the rating text
        int rating1Width = conditions.currentRating.length() * 6;  // Approximate width
        display->drawText(conditions.currentRating.c_str(), col1Center - rating1Width/2, colY + 35, 1);
        
        // Column 2 - TODAY (centered on x=148)
        display->drawText("TODAY", col2Center - 15, colY, 1);  // "TODAY" is ~30px wide, so -15 to center
        String wave2 = String(conditions.todayAverage, 1);
        display->drawText(wave2.c_str(), col2Center - 8, colY + 15, 2);  // Number centered
        display->drawText("ft", col2Center + 8, colY + 15, 1);  // "ft" offset to right
        // Center the rating text
        int rating2Width = conditions.todayRating.length() * 6;  // Approximate width
        display->drawText(conditions.todayRating.c_str(), col2Center - rating2Width/2, colY + 35, 1);
        
        // Column 3 - TOMORROW (centered on x=244)
        display->drawText("TOMORROW", col3Center - 24, colY, 1);  // "TOMORROW" is ~48px wide, so -24 to center
        String wave3 = String(conditions.tomorrowAverage, 1);
        display->drawText(wave3.c_str(), col3Center - 8, colY + 15, 2);  // Number centered
        display->drawText("ft", col3Center + 8, colY + 15, 1);  // "ft" offset to right
        // Center the rating text
        int rating3Width = conditions.tomorrowRating.length() * 6;  // Approximate width
        display->drawText(conditions.tomorrowRating.c_str(), col3Center - rating3Width/2, colY + 35, 1);
        
        // Draw horizontal line above footer
        display->drawLine(2, 108, 294, 108);
        
        // Footer - show last updated time and location
        String updateText = "Last updated: " + getCurrentTimeString();
        display->drawText(updateText.c_str(), 2, 114, 1.5);
        
    } while (display->getDisplay()->nextPage());
    
    display->sleep();
    Serial.println("Surf forecast displayed with proper 3-column layout!");
}

// Removed redundant display methods - keeping only displayCurrentConditions()

void SurfForecast::update() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    // Update every 30 minutes (1800000 ms)
    if (now - lastUpdate > 1800000 || lastUpdate == 0) {
        Serial.println("Updating surf forecast data...");
        if (fetchForecastData()) {
            Serial.println("Surf data updated successfully");
        } else {
            Serial.println("Failed to update surf data");
        }
        lastUpdate = now;
    }
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
