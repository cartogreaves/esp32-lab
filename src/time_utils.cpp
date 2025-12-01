#include <Arduino.h>
#include <WiFi.h>
#include "../include/time_utils.h"

void TimeUtils::begin() {
    // Configure NTP for UK time (GMT/BST automatically handled)
    configTime(0, 3600, "pool.ntp.org", "time.nist.gov"); // UTC+1 for BST
    setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1);
    tzset();

    Serial.println("NTP time sync initialized");
}

String TimeUtils::getCurrentTimestamp() {
    struct tm timeinfo;

    if (getLocalTime(&timeinfo)) {
        // Format: HH:MM:SS Day DDth Month YYYY with ordinal suffix
        char timeStr[60];
        char dayStr[10];

        // Create day string with ordinal suffix
        sprintf(dayStr, "%d%s", timeinfo.tm_mday, getOrdinalSuffix(timeinfo.tm_mday).c_str());

        // Build full timestamp
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S %A ", &timeinfo);
        strcat(timeStr, dayStr);
        strftime(timeStr + strlen(timeStr), sizeof(timeStr) - strlen(timeStr), " %B %Y", &timeinfo);

        return String(timeStr);
    } else {
        // Fallback to milliseconds since boot
        return getFallbackTimestamp();
    }
}

String TimeUtils::getOrdinalSuffix(int day) {
    if (day >= 11 && day <= 13) return "th";
    switch (day % 10) {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}

bool TimeUtils::isTimeSynced() {
    struct tm timeinfo;
    return getLocalTime(&timeinfo);
}

String TimeUtils::getFallbackTimestamp() {
    char fallback[30];
    sprintf(fallback, "%lus ago", millis() / 1000);
    return String(fallback);
}
