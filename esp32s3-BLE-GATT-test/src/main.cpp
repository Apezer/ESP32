/**
 * ESP32-S3 BLE Vico Keyboard
 *
 * 硬件: DFRobot FireBeetle2 ESP32-S3 + 0.96寸 OLED (SSD1306, I2C)
 *
 * 接线:
 *   OLED SDA -> GPIO 10
 *   OLED SCL -> GPIO 9
 *   OLED VCC -> 3.3V
 *   OLED GND -> GND
 *
 *   K1 -> GPIO 4   (z, 同时切换 RGB 灯效)
 *   K2 -> GPIO 5   (x)
 *   K3 -> GPIO 6   (c)
 *   K4 -> GPIO 7   (v)
 *   K5 -> GPIO 8   (b)
 *   K6 -> GPIO 14  (n)
 *   K7 -> GPIO 15  (m)
 *   K8 -> GPIO 16  (a)
 *
 *   WS2812B DIN -> GPIO 17
 *
 * 功能:
 *   - BLE HID 键盘
 *   - 自定义 BLE GATT 状态通道
 *   - OLED 显示按键、连接、ClaudeCode 状态
 *   - RGB 灯带多灯效
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NimBleKeyboard.h>

#include "font.h"
#include "rbg_led.h"

// ===== 硬件配置 =====
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDR       0x3C

#define I2C_SDA         10
#define I2C_SCL         9

#define NUM_KEYS        8
#define DEBOUNCE_MS     10

// ===== BLE / GATT 配置 =====
static const char *DEVICE_NAME = "Vico Keyboard ESP32-S3";
static const char *STATUS_SERVICE_UUID = "7b6a0001-7c6e-4b3d-9f5f-7669636f0001";
static const char *STATUS_RX_UUID      = "7b6a0002-7c6e-4b3d-9f5f-7669636f0002";
static const char *STATUS_TX_UUID      = "7b6a0003-7c6e-4b3d-9f5f-7669636f0003";

// ===== 按键配置 =====
static const uint8_t KEY_PINS[NUM_KEYS] = {4, 5, 6, 7, 8, 14, 15, 16};
static const char KEY_CHARS[NUM_KEYS] = {'z', 'x', 'c', 'v', 'b', 'n', 'm', 'a'};
static const char *KEY_LABELS[NUM_KEYS] = {
    "Z", "X", "C", "V",
    "B", "N", "M", "A",
};

// ===== 创建对象 =====
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static NimBLECharacteristic *statusTxCharacteristic = nullptr;

// ===== OLED 状态 =====
static bool oledReady = false;
static bool oledDirty = true;
static uint8_t lastConnectionCount = 0;

// ===== 按键状态 =====
static bool keyStable[NUM_KEYS] = {false};
static bool keyRawLast[NUM_KEYS] = {false};
static bool keyReported[NUM_KEYS] = {false};
static unsigned long keyLastChange[NUM_KEYS] = {0};

// ===== ClaudeCode 状态 =====
struct ClaudeStatus {
    String state = "ready";
    String tool = "";
    String text = "Waiting for JSON";
    unsigned long updatedAt = 0;
};

static ClaudeStatus currentStatus;

// ===== 工具函数 =====
// OLED 一行空间有限，过长文本使用省略号截断，避免覆盖后续内容。
static String clippedText(const String &text, size_t maxChars) {
    if (text.length() <= maxChars) {
        return text;
    }
    if (maxChars <= 3) {
        return text.substring(0, maxChars);
    }
    return text.substring(0, maxChars - 3) + "...";
}

// 从简单 JSON 中提取字符串字段。当前桥接协议只需要 state/tool/text。
static String jsonStringValue(const String &json, const char *key) {
    const String pattern = String("\"") + key + "\"";
    const int keyIndex = json.indexOf(pattern);
    if (keyIndex < 0) {
        return "";
    }

    const int colon = json.indexOf(':', keyIndex + pattern.length());
    if (colon < 0) {
        return "";
    }

    const int firstQuote = json.indexOf('"', colon + 1);
    if (firstQuote < 0) {
        return "";
    }

    String value;
    bool escaping = false;
    for (int i = firstQuote + 1; i < json.length(); i++) {
        const char c = json[i];
        if (escaping) {
            value += c;
            escaping = false;
            continue;
        }
        if (c == '\\') {
            escaping = true;
            continue;
        }
        if (c == '"') {
            break;
        }
        value += c;
    }
    return value;
}

// 向电脑端返回 ACK。Python 端可以选择不订阅 ACK，以绕开 Windows 加密限制。
static void notifyStatusAck() {
    if (statusTxCharacteristic == nullptr) {
        return;
    }

    const String ack =
        String("{\"ok\":true,\"state\":\"") + currentStatus.state +
        "\",\"text\":\"" + currentStatus.text + "\"}";
    statusTxCharacteristic->setValue(ack.c_str());
    statusTxCharacteristic->notify();
}

// 更新 ClaudeCode 状态，并标记 OLED 需要刷新。
static void updateStatusFromJson(const String &json) {
    const String state = jsonStringValue(json, "state");
    const String tool = jsonStringValue(json, "tool");
    const String text = jsonStringValue(json, "text");

    if (state.length() > 0) {
        currentStatus.state = state;
    }
    currentStatus.tool = tool;
    if (text.length() > 0) {
        currentStatus.text = text;
    }
    currentStatus.updatedAt = millis();
    oledDirty = true;

    Serial.println();
    Serial.println("=== Claude Code Status ===");
    Serial.printf("state: %s\n", currentStatus.state.c_str());
    Serial.printf("tool : %s\n", currentStatus.tool.length() ? currentStatus.tool.c_str() : "-");
    Serial.printf("text : %s\n", currentStatus.text.c_str());
    Serial.printf("raw  : %s\n", json.c_str());
    Serial.println("==========================");
}

// ===== GATT 写入回调 =====
// 电脑端向 RX characteristic 写入 JSON 后，在这里解析并更新显示状态。
class StatusRxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &connInfo) override {
        const std::string value = characteristic->getValue();
        if (value.empty()) {
            return;
        }

        String json;
        json.reserve(value.length() + 1);
        for (char c : value) {
            json += c;
        }

        updateStatusFromJson(json);
        notifyStatusAck();
    }
};

// ===== BLE 键盘扩展类 =====
// 复用 ESP32-NIMBLE-Keyboard 的 HID 键盘能力，并在 onStarted() 中追加自定义 GATT 服务。
class VicoKeyboard : public BleKeyboard {
public:
    VicoKeyboard() : BleKeyboard(DEVICE_NAME, "Apezer", 100) {}

    uint8_t connectionCount() const {
        return activeConnections;
    }

    bool shouldRestartAdvertising() const {
        return restartAdvertisingRequested;
    }

    void clearRestartAdvertisingRequest() {
        restartAdvertisingRequested = false;
    }

protected:
    // HID 服务创建完成后，添加 ClaudeCode 状态通道。
    void onStarted(NimBLEServer *server) override {
        NimBLEService *service = server->createService(STATUS_SERVICE_UUID);

        NimBLECharacteristic *rxCharacteristic = service->createCharacteristic(
            STATUS_RX_UUID,
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
        );
        rxCharacteristic->setCallbacks(new StatusRxCallbacks());

        statusTxCharacteristic = service->createCharacteristic(
            STATUS_TX_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );
        statusTxCharacteristic->setValue("{\"ok\":true,\"state\":\"ready\"}");

        Serial.println("[GATT] Status service created");
    }

    // Windows HID 连接后继续允许第二个 GATT 客户端接入。
    void onConnect(NimBLEServer *server, NimBLEConnInfo &connInfo) override {
        if (activeConnections < CONFIG_BT_NIMBLE_MAX_CONNECTIONS) {
            activeConnections++;
        }

        BleKeyboard::onConnect(server, connInfo);
        restartAdvertisingRequested = activeConnections < CONFIG_BT_NIMBLE_MAX_CONNECTIONS;
        oledDirty = true;

        Serial.printf("[BLE] Connected peer: %s, active=%u/%u\n",
                      connInfo.getAddress().toString().c_str(),
                      activeConnections,
                      CONFIG_BT_NIMBLE_MAX_CONNECTIONS);
    }

    // 所有连接断开后，才把键盘库内部状态恢复为未连接。
    void onDisconnect(NimBLEServer *server, NimBLEConnInfo &connInfo, int reason) override {
        if (activeConnections > 0) {
            activeConnections--;
        }

        if (activeConnections == 0) {
            BleKeyboard::onDisconnect(server, connInfo, reason);
        }
        restartAdvertisingRequested = activeConnections < CONFIG_BT_NIMBLE_MAX_CONNECTIONS;
        oledDirty = true;

        Serial.printf("[BLE] Disconnected peer: %s, reason=%d, active=%u/%u\n",
                      connInfo.getAddress().toString().c_str(),
                      reason,
                      activeConnections,
                      CONFIG_BT_NIMBLE_MAX_CONNECTIONS);
    }

private:
    uint8_t activeConnections = 0;
    bool restartAdvertisingRequested = false;
};

static VicoKeyboard bleKeyboard;

// ===== 开机画面 =====
static void showSplash() {
    if (!oledReady) {
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.drawBitmap(
        (SCREEN_WIDTH - CLAUDE_LOGO_WIDTH) / 2,
        (SCREEN_HEIGHT - CLAUDE_LOGO_HEIGHT) / 2,
        CLAUDE_LOGO_64X32,
        CLAUDE_LOGO_WIDTH,
        CLAUDE_LOGO_HEIGHT,
        SSD1306_WHITE
    );
    display.display();
}

// ===== 绘制单个按键状态 =====
static void drawKeyBox(uint8_t index, bool pressed) {
    // 2 行 x 4 列网格布局，保持和参考键盘项目一致。
    const uint8_t boxW = 28;
    const uint8_t boxH = 13;
    const uint8_t gap = 2;
    const uint8_t startX = 4;
    const uint8_t startY = 16;

    const uint8_t col = index % 4;
    const uint8_t row = index / 4;
    const uint8_t x = startX + col * (boxW + gap);
    const uint8_t y = startY + row * (boxH + gap);

    if (pressed) {
        display.fillRect(x, y, boxW, boxH, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    } else {
        display.drawRect(x, y, boxW, boxH, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
    }

    display.setTextSize(1);
    display.setCursor(x + (boxW - 6) / 2, y + (boxH - 8) / 2);
    display.print(KEY_LABELS[index]);
    display.setTextColor(SSD1306_WHITE);
}

// ===== 渲染 OLED 主界面 =====
// 顶部显示连接与灯效，中间显示按键，底部显示 GATT 收到的状态文本。
static void renderDashboard() {
    if (!oledReady) {
        return;
    }

    const uint8_t connectionCount = bleKeyboard.connectionCount();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print("Vico");
    display.setCursor(32, 0);
    display.print(connectionCount > 0 ? "BLE" : "ADV");
    display.setCursor(58, 0);
    display.print("C:");
    display.print(connectionCount);
    display.setCursor(88, 0);
    display.print(clippedText(rbgLedGetEffectName(), 6));
    display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        drawKeyBox(i, keyStable[i]);
    }

    display.drawLine(0, 46, 127, 46, SSD1306_WHITE);
    display.setCursor(0, 49);
    display.print(clippedText(currentStatus.state, 8));
    if (currentStatus.tool.length() > 0) {
        display.print("/");
        display.print(clippedText(currentStatus.tool, 7));
    }

    display.setCursor(0, 57);
    display.print(clippedText(currentStatus.text, 21));

    display.display();
    oledDirty = false;
}

// ===== 初始化 OLED =====
static void setupOled() {
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] SSD1306 init failed");
        oledReady = false;
        return;
    }

    oledReady = true;
    Serial.println("[OLED] SSD1306 init OK");
    showSplash();
    delay(1200);
    oledDirty = true;
}

// ===== 保持广播，允许第二个 GATT 客户端连接 =====
// HID 连接建立后 BLE 广播会停止，这里在主循环中安全地重新开启广播。
static void keepAdvertisingForSecondClient() {
    if (!bleKeyboard.shouldRestartAdvertising()) {
        return;
    }

    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
    if (advertising != nullptr && !advertising->isAdvertising()) {
        const bool ok = NimBLEDevice::startAdvertising();
        Serial.printf("[BLE] Continue advertising for GATT client: %s\n", ok ? "OK" : "FAILED");
    }
    bleKeyboard.clearRestartAdvertisingRequest();
}

// ===== 按键事件处理 =====
static void handleKeyPress(uint8_t index) {
    if (index == 0) {
        rbgLedNextEffect();
        oledDirty = true;
    }

    if (bleKeyboard.isConnected()) {
        bleKeyboard.press(KEY_CHARS[index]);
    }
}

static void handleKeyRelease(uint8_t index) {
    if (bleKeyboard.isConnected()) {
        bleKeyboard.release(KEY_CHARS[index]);
    }
}

// ===== 扫描按键 + 去抖 + 发送 BLE 键值 =====
static void scanKeys() {
    const unsigned long now = millis();

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        const bool rawPressed = digitalRead(KEY_PINS[i]) == LOW;

        if (rawPressed != keyRawLast[i]) {
            keyRawLast[i] = rawPressed;
            keyLastChange[i] = now;
        }

        if ((now - keyLastChange[i]) >= DEBOUNCE_MS && rawPressed != keyStable[i]) {
            keyStable[i] = rawPressed;
            oledDirty = true;
        }
    }

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        if (keyStable[i] && !keyReported[i]) {
            handleKeyPress(i);
        } else if (!keyStable[i] && keyReported[i]) {
            handleKeyRelease(i);
        }
        keyReported[i] = keyStable[i];
    }
}

// ===== setup =====
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== Vico BLE Keyboard ESP32-S3 ===");

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        pinMode(KEY_PINS[i], INPUT_PULLUP);
        const bool pressed = digitalRead(KEY_PINS[i]) == LOW;
        keyStable[i] = pressed;
        keyRawLast[i] = pressed;
        keyReported[i] = pressed;
    }

    randomSeed(esp_random());
    rbgLedInit();

    bleKeyboard.begin();
    Serial.printf("[BLE] HID keyboard advertising as: %s\n", DEVICE_NAME);
    Serial.println("[BLE] Custom GATT status service is enabled");
    Serial.printf("[GATT] Service UUID: %s\n", STATUS_SERVICE_UUID);
    Serial.printf("[GATT] RX UUID     : %s\n", STATUS_RX_UUID);
    Serial.printf("[GATT] TX UUID     : %s\n", STATUS_TX_UUID);

    setupOled();
}

// ===== loop =====
void loop() {
    static unsigned long lastDraw = 0;
    const unsigned long now = millis();

    rbgLedUpdate();
    keepAdvertisingForSecondClient();
    scanKeys();

    const uint8_t connectionCount = bleKeyboard.connectionCount();
    if (connectionCount != lastConnectionCount) {
        lastConnectionCount = connectionCount;
        Serial.printf("[BLE] Active connections: %u/%u\n", connectionCount, CONFIG_BT_NIMBLE_MAX_CONNECTIONS);
        oledDirty = true;
    }

    if (oledDirty || now - lastDraw >= 1000) {
        lastDraw = now;
        renderDashboard();
    }

    delay(2);
}
