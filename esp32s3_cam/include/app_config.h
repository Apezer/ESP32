#pragma once

// Leave WIFI_STA_SSID empty to use the built-in access point only.
// To join an existing 2.4 GHz Wi-Fi network, fill in both values below.
#define WIFI_STA_SSID ""
#define WIFI_STA_PASSWORD ""

#define WIFI_AP_SSID "ESP32S3-OV5640"
#define WIFI_AP_PASSWORD "12345678"

// VGA is a good initial balance of latency and detail.
#define CAMERA_INITIAL_FRAME_SIZE FRAMESIZE_VGA
#define CAMERA_JPEG_QUALITY 12
