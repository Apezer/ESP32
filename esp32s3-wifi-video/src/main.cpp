/**
 * ESP32-S3 WiFi Video - OLED WiFi Video Streamer
 *
 * OLED 接线 (SSD1306 128x64 I2C):
 *   SDA -> GPIO 10
 *   SCL -> GPIO 9
 *   VCC -> 3.3V
 *   GND -> GND
 *
 * 功能: ESP32 创建 WiFi AP，通过 TCP 接收视频帧，在 OLED 上实时播放
 * 协议: 每帧 = [0xAA][0x55] + 1024字节位图数据
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "font.h"

// ===== OLED 配置 =====
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDR       0x3C
#define I2C_SDA         10
#define I2C_SCL         9

// ===== WiFi AP 配置 =====
#define AP_SSID         "ESP32S3-VIDEO"
#define AP_PASS         "12345678"
#define TCP_PORT        8080

// ===== 帧协议 =====
#define SYNC_BYTE_0     0xAA
#define SYNC_BYTE_1     0x55
#define FRAME_SIZE      1024  // 128 * 64 / 8

// ===== OLED 对象 =====
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static bool oledReady = false;

// ===== WiFi / TCP =====
WiFiServer tcpServer(TCP_PORT);
WiFiClient client;

// ===== 帧统计 =====
static uint32_t frameCount = 0;
static uint32_t fpsTimer = 0;
static float currentFps = 0;

// ===== 开机 Splash =====
static void showSplash() {
    display.clearDisplay();
    display.drawBitmap((SCREEN_WIDTH - 32) / 2, 0, wifi_icon, 32, 32, SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 36);
    display.println("WiFi Video");
    display.setCursor(28, 48);
    display.println("ESP32-S3");
    display.drawLine(0, 58, SCREEN_WIDTH - 1, 58, SSD1306_WHITE);
    display.setCursor(8, 60);
    display.print("AP: ");
    display.print(AP_SSID);
    display.display();
}

// ===== 等待连接界面 =====
static void showWaiting() {
    display.clearDisplay();
    display.drawBitmap(48, 0, wifi_icon, 32, 32, SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 36);
    display.print("Waiting for");
    display.setCursor(20, 46);
    display.print("connection...");
    display.setCursor(0, 58);
    display.print("TCP ");
    display.print(WiFi.softAPIP());
    display.print(":");
    display.print(TCP_PORT);
    display.display();
}

// ===== OLED 初始化 =====
static void setupOled() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(800000); // 800kHz I2C，加速显示
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] SSD1306 init failed");
        oledReady = false;
        return;
    }
    oledReady = true;
    display.contrast(0xFF); // 最大对比度
    Serial.println("[OLED] SSD1306 init OK (800kHz I2C)");
    showSplash();
    delay(1500);
}

// ===== WiFi AP 初始化 =====
static void setupWifi() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(100);
    Serial.printf("[WiFi] AP '%s' started\n", AP_SSID);
    Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());

    tcpServer.begin();
    tcpServer.setNoDelay(true);
    Serial.printf("[TCP] Listening on port %d\n", TCP_PORT);
}

// ===== 读取指定字节数 =====
static bool readExact(uint8_t *buf, size_t len) {
    size_t received = 0;
    uint32_t start = millis();
    while (received < len) {
        if (!client.connected()) return false;
        int available = client.available();
        if (available > 0) {
            size_t toRead = min((size_t)available, len - received);
            client.read(buf + received, toRead);
            received += toRead;
            start = millis(); // 重置超时
        } else {
            if (millis() - start > 3000) {
                Serial.println("[TCP] Read timeout");
                return false;
            }
            delayMicroseconds(100);
        }
    }
    return true;
}

// ===== 接收并显示一帧 =====
static bool receiveFrame() {
    // 1. 寻找同步头
    uint8_t sync[2];
    uint32_t syncStart = millis();
    int syncState = 0;

    while (syncState < 2) {
        if (!client.connected()) return false;
        if (client.available()) {
            uint8_t b = client.read();
            if (syncState == 0 && b == SYNC_BYTE_0) {
                syncState = 1;
            } else if (syncState == 1 && b == SYNC_BYTE_1) {
                syncState = 2;
            } else {
                syncState = 0;
            }
        }
        if (millis() - syncStart > 3000) {
            Serial.println("[TCP] Sync timeout");
            return false;
        }
    }

    // 2. 读取帧数据，直接写入显示缓冲区
    uint8_t *buf = display.getBuffer();
    if (!readExact(buf, FRAME_SIZE)) {
        return false;
    }

    // 3. 刷新显示
    display.display();
    frameCount++;
    return true;
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n[BOOT] ESP32-S3 WiFi Video Streamer");

    setupOled();
    setupWifi();

    if (oledReady) {
        showWaiting();
    }
}

void loop() {
    // 检查新连接
    if (!client || !client.connected()) {
        client = tcpServer.available();
        if (client) {
            Serial.println("[TCP] Client connected");
            client.setNoDelay(true);
            frameCount = 0;
            fpsTimer = millis();
        }
        delay(10);
        return;
    }

    // 接收并显示帧
    if (oledReady) {
        if (!receiveFrame()) {
            Serial.println("[TCP] Connection lost");
            client.stop();
            showWaiting();
            return;
        }

        // 计算帧率 (每秒更新)
        uint32_t now = millis();
        if (now - fpsTimer >= 1000) {
            currentFps = frameCount * 1000.0f / (now - fpsTimer);
            Serial.printf("[STAT] Frames: %d, FPS: %.1f, Heap: %d\n",
                          frameCount, currentFps, ESP.getFreeHeap());
            frameCount = 0;
            fpsTimer = now;
        }
    }
}
