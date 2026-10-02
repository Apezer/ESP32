#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>

#include "app_config.h"
#include "camera_pins.h"
#include "camera_server.h"

namespace {

bool initializeCamera() {
  camera_config_t config{};
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
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = CAMERA_INITIAL_FRAME_SIZE;
  config.jpeg_quality = CAMERA_JPEG_QUALITY;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 2;
  } else {
    Serial.println("[WARN] PSRAM not found; limiting stream to QVGA.");
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count = 1;
    config.frame_size = FRAMESIZE_QVGA;
  }

  const esp_err_t error = esp_camera_init(&config);
  if (error != ESP_OK) {
    Serial.printf("[CAM] Initialization failed: 0x%04x\n", error);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor) {
    Serial.println("[CAM] Sensor handle is unavailable.");
    return false;
  }

  Serial.printf("[CAM] Sensor PID: 0x%04x\n", sensor->id.PID);
  if (sensor->id.PID != OV5640_PID) {
    Serial.println("[WARN] Detected sensor is not OV5640; streaming will still be attempted.");
  } else {
    Serial.println("[CAM] OV5640 detected.");
  }

  sensor->set_framesize(sensor, config.frame_size);
  sensor->set_quality(sensor, CAMERA_JPEG_QUALITY);
  return true;
}

IPAddress startAccessPoint() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
    Serial.println("[WiFi] Failed to start access point.");
    return IPAddress();
  }
  const IPAddress ip = WiFi.softAPIP();
  Serial.printf("[WiFi] AP: %s\n", WIFI_AP_SSID);
  Serial.printf("[WiFi] Password: %s\n", WIFI_AP_PASSWORD);
  return ip;
}

IPAddress connectWiFi() {
  if (strlen(WIFI_STA_SSID) == 0) return startAccessPoint();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
  Serial.printf("[WiFi] Connecting to %s", WIFI_STA_SSID);
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(400);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP();

  Serial.println("[WiFi] Station connection timed out; falling back to access point.");
  WiFi.disconnect(true);
  delay(200);
  return startAccessPoint();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("\n[BOOT] ESP32-S3 + OV5640 MJPEG camera server");
  Serial.printf("[MEM] Flash: %.1f MB, PSRAM: %.1f MB\n",
                ESP.getFlashChipSize() / 1048576.0, ESP.getPsramSize() / 1048576.0);

  // Bring Wi-Fi up first so a camera fault never makes the AP disappear.
  const IPAddress ip = connectWiFi();
  if (ip == IPAddress()) return;

  if (!initializeCamera()) {
    Serial.println("[FATAL] Check camera ribbon orientation, power and pin mapping.");
    return;
  }

  if (!startCameraWebServer()) {
    Serial.println("[FATAL] Failed to start HTTP servers.");
    return;
  }

  Serial.printf("[READY] Open http://%s/\n", ip.toString().c_str());
  Serial.printf("[READY] Direct stream: http://%s:81/stream\n", ip.toString().c_str());
}

void loop() {
  delay(10000);
}
