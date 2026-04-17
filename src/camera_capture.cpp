#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "../include/camera_capture.h"
#include "../include/time_utils.h"

// Camera pin definitions for Freenove ESP32-WROVER (fixed by FFC connector)
// E-paper display uses HSPI on GPIO 12/13/14 - NO pin overlap with camera.
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    21
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      19
#define CAM_PIN_D2      18
#define CAM_PIN_D1       5
#define CAM_PIN_D0       4
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

CameraCapture::CameraCapture(EPaperDisplay* displayPtr)
    : display(displayPtr), server(80), imageBuffer(nullptr), imageBufferSize(0),
      initialized(false), cameraActive(false), lastCaptureTime(0) {
    currentData = {false, 0, 0, 0, 0, ""};
}

CameraCapture::~CameraCapture() {
    if (imageBuffer) {
        free(imageBuffer);
    }
}

bool CameraCapture::initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // Use PSRAM for frame buffer if available
    if (psramFound()) {
        config.fb_location = CAMERA_FB_IN_PSRAM;
        Serial.println("PSRAM found - using for camera frame buffer");
    } else {
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.frame_size = FRAMESIZE_QQVGA; // 160x120 if no PSRAM
        Serial.println("No PSRAM - using DRAM with reduced resolution");
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        cameraActive = false;
        return false;
    }

    cameraActive = true;
    Serial.println("Camera initialized successfully");
    return true;
}

void CameraCapture::deinitCamera() {
    if (cameraActive) {
        esp_camera_deinit();
        cameraActive = false;
        delay(100); // Allow pins to settle after release
        Serial.println("Camera deinitialized, shared pins released");
    }
}

bool CameraCapture::takePhoto() {
    Serial.println("--- Camera capture sequence ---");

    // Step 1: Initialize camera (uses GPIO 18/23 via I2S - no conflict with display on HSPI 13/14)
    if (!initCamera()) {
        currentData.captureSuccess = false;
        return false;
    }

    // Step 2: Capture a frame
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed - no frame buffer");
        currentData.captureSuccess = false;
        deinitCamera();
        return false;
    }

    // Step 3: Store capture metadata and copy image to PSRAM buffer
    currentData.captureSuccess = true;
    currentData.imageSize = fb->len;
    currentData.frameWidth = fb->width;
    currentData.frameHeight = fb->height;
    currentData.captureCount++;
    currentData.lastCaptureTime = TimeUtils::getCurrentTimestamp();

    // Copy JPEG data to persistent buffer (survives camera deinit)
    if (imageBuffer) {
        free(imageBuffer);
    }
    imageBuffer = (uint8_t*)ps_malloc(fb->len);
    if (imageBuffer) {
        memcpy(imageBuffer, fb->buf, fb->len);
        imageBufferSize = fb->len;
        Serial.printf("Image saved to PSRAM buffer (%u bytes)\n", fb->len);
    } else {
        Serial.println("WARNING: Failed to allocate PSRAM for image buffer");
        imageBufferSize = 0;
    }

    Serial.printf("Photo captured! Size: %u bytes, Resolution: %dx%d, Count: %u\n",
                  fb->len, fb->width, fb->height, currentData.captureCount);

    // Step 4: Release frame buffer and deinitialize camera
    esp_camera_fb_return(fb);
    deinitCamera();

    Serial.println("--- Capture sequence complete ---");
    return true;
}

void CameraCapture::reinitDisplay() {
    // Display uses HSPI on GPIO 13/14 - completely independent from camera pins.
    // No SPI restoration needed. Just log for debugging.
    Serial.println("Camera released - display HSPI bus unaffected");
}

void CameraCapture::setupWebServer() {
    // Serve the last captured image as JPEG
    server.on("/capture", HTTP_GET, [this]() {
        if (imageBuffer && imageBufferSize > 0) {
            server.send_P(200, "image/jpeg", (const char*)imageBuffer, imageBufferSize);
        } else {
            server.send(404, "text/plain", "No image captured yet");
        }
    });

    // Simple status page
    server.on("/", HTTP_GET, [this]() {
        String html = "<html><body>";
        html += "<h1>ESP32 Camera Capture</h1>";
        if (currentData.captureSuccess) {
            html += "<p>Last capture: " + currentData.lastCaptureTime + "</p>";
            html += "<p>Size: " + String(currentData.imageSize) + " bytes</p>";
            html += "<p>Resolution: " + String(currentData.frameWidth) + "x" + String(currentData.frameHeight) + "</p>";
            html += "<p>Total captures: " + String(currentData.captureCount) + "</p>";
            html += "<p><img src='/capture' /></p>";
        } else {
            html += "<p>No successful capture yet</p>";
        }
        html += "</body></html>";
        server.send(200, "text/html", html);
    });

    server.begin();
    Serial.println("Web server started - view image at http://" + WiFi.localIP().toString() + "/capture");
}

void CameraCapture::begin(const char* ssid, const char* password) {
    Serial.println("Initializing Camera Capture module...");
    Serial.println("NOTE: Camera and e-paper share GPIO 4, 5, 18, 23, 25");

    // Connect to WiFi for NTP time sync
    if (ssid != nullptr && strlen(ssid) > 0) {
        Serial.printf("Connecting to WiFi: %s", ssid);
        WiFi.begin(ssid, password);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());

            Serial.println("Waiting for NTP time sync...");
            struct tm timeinfo;
            attempts = 0;
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
        } else {
            Serial.println();
            Serial.println("WiFi connection failed - using fallback timestamps");
        }
    }

    // Take initial photo (camera claims shared pins)
    takePhoto();

    // Camera is now deinitialized - safe to reinit the e-paper display
    reinitDisplay();

    // Start web server to view captured images
    setupWebServer();

    initialized = true;
    lastCaptureTime = millis();

    Serial.println("Camera Capture module initialized!");
}

void CameraCapture::update() {
    if (!initialized) return;

    // Handle web server requests
    server.handleClient();

    unsigned long currentTime = millis();

    // Handle millis() overflow (~49.7 days)
    if (currentTime < lastCaptureTime) {
        lastCaptureTime = currentTime;
    }

    if (currentTime - lastCaptureTime >= CAPTURE_INTERVAL_MS) {
        Serial.println("Taking new photo...");
        takePhoto();

        // Reinitialize display after camera released shared pins
        reinitDisplay();

        lastCaptureTime = currentTime;
    }
}

void CameraCapture::displayCurrentData() {
    if (!display || !initialized) return;

    Serial.println("Updating e-paper display with capture info...");

    auto* gxDisplay = display->getDisplay();
    gxDisplay->setRotation(1); // Landscape orientation
    gxDisplay->setFullWindow();
    gxDisplay->firstPage();

    do {
        gxDisplay->fillScreen(GxEPD_WHITE);
        gxDisplay->setTextColor(GxEPD_BLACK);
        gxDisplay->setFont();

        // Header
        gxDisplay->setTextSize(1);
        gxDisplay->setCursor(2, 6);
        gxDisplay->print("CAMERA CAPTURE");

        // Horizontal line under header
        gxDisplay->drawLine(2, 18, 294, 18, GxEPD_BLACK);

        if (currentData.captureSuccess) {
            // Main success message - centered
            gxDisplay->setTextSize(2);
            int msgWidth = display->getTextWidth("Photo taken!", 2);
            gxDisplay->setCursor(148 - msgWidth / 2, 28);
            gxDisplay->print("Photo taken!");

            // Capture details
            gxDisplay->setTextSize(1);

            char sizeStr[32];
            if (currentData.imageSize > 1024) {
                sprintf(sizeStr, "Size: %.1f KB", currentData.imageSize / 1024.0);
            } else {
                sprintf(sizeStr, "Size: %u bytes", (unsigned int)currentData.imageSize);
            }
            gxDisplay->setCursor(10, 56);
            gxDisplay->print(sizeStr);

            char resStr[32];
            sprintf(resStr, "Resolution: %dx%d", currentData.frameWidth, currentData.frameHeight);
            gxDisplay->setCursor(10, 70);
            gxDisplay->print(resStr);

            char countStr[32];
            sprintf(countStr, "Total captures: %u", currentData.captureCount);
            gxDisplay->setCursor(10, 84);
            gxDisplay->print(countStr);

            // Show web URL for image viewing
            String urlStr = "View: http://" + WiFi.localIP().toString();
            gxDisplay->setCursor(10, 98);
            gxDisplay->print(urlStr);
        } else {
            // Error state
            gxDisplay->setTextSize(2);
            gxDisplay->setCursor(10, 35);
            gxDisplay->print("Capture failed!");

            gxDisplay->setTextSize(1);
            gxDisplay->setCursor(10, 65);
            gxDisplay->print("Check camera connection");
        }

        // Horizontal line above footer
        gxDisplay->drawLine(2, 108, 294, 108, GxEPD_BLACK);

        // Footer - timestamp
        String timeStr = currentData.lastCaptureTime.isEmpty()
            ? "No capture yet" : currentData.lastCaptureTime;
        String updateText = "Last captured: " + timeStr;
        gxDisplay->setTextSize(1);
        gxDisplay->setCursor(2, 114);
        gxDisplay->print(updateText);

    } while (gxDisplay->nextPage());

    gxDisplay->hibernate();
    Serial.println("E-paper display updated with capture info");
}

bool CameraCapture::isDataReady() const {
    return initialized && currentData.captureSuccess;
}

CaptureData CameraCapture::getCurrentData() const {
    return currentData;
}
