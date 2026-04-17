#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time_utils.h"

struct SupabaseConfig {
    String url;          // Supabase project URL
    String apiKey;       // Supabase anon/public key
    int batchSize;       // Number of records to batch before sending
    unsigned long logInterval; // Logging interval in milliseconds
};

struct LogEntry {
    String timestamp;
    JsonDocument data;   // Flexible JSON data structure
    String tableName;    // Target table name
    bool success;
    String errorMessage;
};

class DataLogger {
private:
    SupabaseConfig config;
    WiFiClientSecure* client;
    HTTPClient* httpClient;

    // Logging state
    std::vector<LogEntry> logBuffer;
    unsigned long lastLogTime;
    bool initialized;

    // HTTP helper methods
    bool sendBatchToSupabase(const std::vector<LogEntry>& entries);
    String createInsertQuery(const std::vector<LogEntry>& entries);
    bool makeHttpRequest(const String& url, const String& payload);

public:
    DataLogger();
    ~DataLogger();

    // Configuration and initialization
    void setConfig(const SupabaseConfig& newConfig);
    void begin();

    // Logging methods
    bool logData(const JsonDocument& data, const String& tableName);

    // Batch operations
    void flushBuffer();  // Force send all buffered logs
    size_t getBufferSize() const { return logBuffer.size(); }

    // Status methods
    bool isInitialized() const { return initialized; }
    bool isBufferFull() const { return logBuffer.size() >= config.batchSize; }
    String getLastError() const;

    // Periodic update (call in main loop)
    void update();
};

#endif
