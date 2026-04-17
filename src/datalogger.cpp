#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../include/datalogger.h"

DataLogger::DataLogger()
    : client(nullptr), httpClient(nullptr), initialized(false), lastLogTime(0) {
    // Default configuration
    config.batchSize = 10;
    config.logInterval = 60000; // 1 minute
}

DataLogger::~DataLogger() {
    if (httpClient) {
        delete httpClient;
    }
    if (client) {
        delete client;
    }
}

void DataLogger::setConfig(const SupabaseConfig& newConfig) {
    config = newConfig;
    Serial.printf("DataLogger config set: URL=%s, BatchSize=%d\n",
                  config.url.c_str(), config.batchSize);
}

void DataLogger::begin() {
    if (config.url.isEmpty() || config.apiKey.isEmpty()) {
        Serial.println("DataLogger: Configuration incomplete - missing URL or API key");
        return;
    }

    // Initialize secure client for HTTPS
    client = new WiFiClientSecure();
    client->setInsecure(); // For development - consider proper certificate validation in production

    httpClient = new HTTPClient();

    initialized = true;
    Serial.println("DataLogger initialized successfully");
}

bool DataLogger::logData(const JsonDocument& data, const String& tableName) {
    if (!initialized) {
        Serial.println("DataLogger: Not initialized");
        return false;
    }

    LogEntry entry;
    entry.timestamp = TimeUtils::getCurrentTimestamp();
    entry.data = data;
    entry.tableName = tableName;
    entry.success = false;
    entry.errorMessage = "";

    logBuffer.push_back(entry);

    // Send batch if buffer is full
    if (logBuffer.size() >= config.batchSize) {
        return sendBatchToSupabase(logBuffer);
    }

    return true; // Buffered successfully
}


void DataLogger::flushBuffer() {
    if (!logBuffer.empty()) {
        sendBatchToSupabase(logBuffer);
    }
}

void DataLogger::update() {
    if (!initialized) {
        return;
    }

    unsigned long currentTime = millis();

    // Handle millis() overflow
    if (currentTime < lastLogTime) {
        lastLogTime = currentTime;
    }

    // Periodic flush based on time interval
    if (currentTime - lastLogTime >= config.logInterval && !logBuffer.empty()) {
        sendBatchToSupabase(logBuffer);
        lastLogTime = currentTime;
    }
}

bool DataLogger::sendBatchToSupabase(const std::vector<LogEntry>& entries) {
    if (entries.empty()) {
        return true;
    }

    Serial.printf("DataLogger: Sending batch of %d entries to Supabase\n", entries.size());

    // Construct the full URL
    String url = config.url;
    if (!url.endsWith("/")) url += "/";
    url += "rest/v1/";
    url += entries[0].tableName; // Use table name from the log entry

    // Create JSON payload
    JsonDocument payload;
    JsonArray records = payload.to<JsonArray>();

    for (const auto& entry : entries) {
        JsonObject record = records.add<JsonObject>();
        record["timestamp"] = entry.timestamp;

        // Add the sensor data
        for (JsonPairConst kv : entry.data.as<JsonObjectConst>()) {
            record[kv.key().c_str()] = kv.value();
        }
    }

    String payloadStr;
    serializeJson(payload, payloadStr);

    Serial.printf("DataLogger: Payload size: %d bytes\n", payloadStr.length());

    // Make HTTP request
    if (!makeHttpRequest(url, payloadStr)) {
        Serial.println("DataLogger: Failed to send batch to Supabase");

        // Mark entries as failed in buffer
        for (auto& entry : logBuffer) {
            entry.success = false;
            entry.errorMessage = "HTTP request failed";
        }
        return false;
    }

    Serial.println("DataLogger: Batch sent successfully");

    // Mark entries as successful and clear buffer
    logBuffer.clear();
    return true;
}

bool DataLogger::makeHttpRequest(const String& url, const String& payload) {
    if (!httpClient || !client) {
        return false;
    }

    httpClient->begin(*client, url);
    httpClient->addHeader("Content-Type", "application/json");
    httpClient->addHeader("Authorization", "Bearer " + config.apiKey);
    httpClient->addHeader("apikey", config.apiKey);

    Serial.printf("DataLogger: POST to %s\n", url.c_str());

    int httpResponseCode = httpClient->POST(payload);

    if (httpResponseCode > 0) {
        String response = httpClient->getString();
        Serial.printf("DataLogger: HTTP Response code: %d\n", httpResponseCode);
        Serial.printf("DataLogger: Response: %s\n", response.c_str());

        httpClient->end();
        return (httpResponseCode >= 200 && httpResponseCode < 300);
    } else {
        Serial.printf("DataLogger: HTTP Error: %d\n", httpResponseCode);
        httpClient->end();
        return false;
    }
}


String DataLogger::getLastError() const {
    if (logBuffer.empty()) {
        return "";
    }
    return logBuffer.back().errorMessage;
}
