#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>
#include <time.h>

class TimeUtils {
public:
    // Initialize NTP time sync
    static void begin();

    // Get current timestamp with full date formatting
    static String getCurrentTimestamp();

    // Get ordinal suffix for day of month (1st, 2nd, 3rd, etc.)
    static String getOrdinalSuffix(int day);

    // Check if NTP time is synchronized
    static bool isTimeSynced();

private:
    // Helper method for fallback timestamp
    static String getFallbackTimestamp();
};

#endif
