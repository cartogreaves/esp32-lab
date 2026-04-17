#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include <Arduino.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "epaper_display.h"
#include "sensor_interface.h"

struct CaptureData {
    bool captureSuccess;
    size_t imageSize;       // bytes
    int frameWidth;
    int frameHeight;
    uint32_t captureCount;
    String lastCaptureTime;
};

class CameraCapture : public SensorInterface {
private:
    EPaperDisplay* display;
    CaptureData currentData;
    WebServer server;

    // PSRAM buffer to store last captured JPEG
    uint8_t* imageBuffer;
    size_t imageBufferSize;

    unsigned long lastCaptureTime;
    static const unsigned long CAPTURE_INTERVAL_MS = 60000; // 60 seconds
    bool initialized;
    bool cameraActive;

    bool initCamera();
    void deinitCamera();
    bool takePhoto();
    void reinitDisplay();
    void setupWebServer();

public:
    CameraCapture(EPaperDisplay* displayPtr);
    virtual ~CameraCapture();

    // Implement SensorInterface
    void begin(const char* ssid, const char* password) override;
    void update() override;
    void displayCurrentData() override;
    bool isDataReady() const override;

    // Camera-specific methods
    CaptureData getCurrentData() const;
};

#endif
