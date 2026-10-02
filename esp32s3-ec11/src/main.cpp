/**
 * ESP32-S3 EC11 编码器测试
 *
 * 硬件: DFRobot FireBeetle2 ESP32-S3 + 0.96寸 OLED (SSD1306, I2C)
 *
 * 接线:
 *   OLED SDA -> GPIO 10
 *   OLED SCL -> GPIO 9
 *   OLED VCC -> 3.3V
 *   OLED GND -> GND
 *
 *   编码器 A  -> GPIO 11
 *   编码器 B  -> GPIO 12
 *   编码器 SW -> GPIO 13
 *
 * 功能:
 *   - OLED 显示编码器旋转方向、计数值、按键状态
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== 硬件配置 =====
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

#define I2C_SDA 10
#define I2C_SCL 9

// 编码器引脚
#define ENC_A   11
#define ENC_B   12
#define ENC_SW  13

// ===== 创建对象 =====
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== 编码器状态 =====
static volatile int32_t encoderCount = 0;
static volatile uint8_t lastEncoded = 0;
static volatile bool encoderChanged = false;

// 按键状态
static bool buttonPressed = false;
static bool lastButtonRaw = HIGH;
static unsigned long lastDebounceTime = 0;
static const unsigned long DEBOUNCE_MS = 50;

// ===== 编码器中断处理 =====
static void IRAM_ATTR encoderISR() {
    uint8_t a = digitalRead(ENC_A);
    uint8_t b = digitalRead(ENC_B);
    uint8_t encoded = (a << 1) | b;
    uint8_t sum = (lastEncoded << 2) | encoded;

    // 状态转换表: 只在有效转换时更新计数
    if (sum == 0b0001 || sum == 0b0111 || sum == 0b1110 || sum == 0b1000) {
        encoderCount++;
        encoderChanged = true;
    } else if (sum == 0b0010 || sum == 0b0100 || sum == 0b1101 || sum == 0b1011) {
        encoderCount--;
        encoderChanged = true;
    }

    lastEncoded = encoded;
}

// ===== 初始化 OLED =====
static bool oledReady = false;

static void setupOled() {
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] SSD1306 init failed");
        oledReady = false;
        return;
    }
    oledReady = true;
    Serial.println("[OLED] SSD1306 init OK");

    // 开机画面
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 24);
    display.print("EC11 Encoder");
    display.setCursor(32, 40);
    display.print("Test v1.0");
    display.display();
    delay(1000);
}

// ===== 初始化编码器 =====
static void setupEncoder() {
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
    pinMode(ENC_SW, INPUT_PULLUP);

    // 读取初始状态
    uint8_t a = digitalRead(ENC_A);
    uint8_t b = digitalRead(ENC_B);
    lastEncoded = (a << 1) | b;

    // 在 A 和 B 相上都挂中断，提高精度
    attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);

    Serial.println("[ENC] Encoder init OK");
}

// ===== 渲染 OLED =====
static int32_t lastDisplayCount = 0;
static bool lastDisplayButton = false;
static bool displayDirty = true;

static void renderDisplay() {
    if (!oledReady) return;

    // 检测按键 (带消抖)
    bool raw = digitalRead(ENC_SW);
    if (raw != lastButtonRaw) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
        if (raw != buttonPressed) {
            buttonPressed = raw;
            displayDirty = true;
        }
    }
    lastButtonRaw = raw;

    // 检测编码器变化
    if (encoderChanged) {
        encoderChanged = false;
        displayDirty = true;
    }

    if (!displayDirty) return;
    displayDirty = false;

    int32_t count = encoderCount;
    bool pressed = (buttonPressed == LOW); // LOW = 按下

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // ---- 标题栏 (y=0~10) ----
    display.setCursor(0, 0);
    display.print("EC11 Encoder");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // ---- 旋转方向 + 按键状态 (y=12~20) ----
    display.setCursor(0, 12);
    if (count > lastDisplayCount) {
        display.print("CW >>");
    } else if (count < lastDisplayCount) {
        display.print("<< CCW");
    } else {
        display.print("--");
    }
    display.setCursor(72, 12);
    display.print(pressed ? "[PUSH]" : "open");

    // ---- 计数值 大字体 (y=22~37) ----
    display.setCursor(0, 24);
    display.setTextSize(2);
    display.print(count);
    display.setTextSize(1);

    // ---- 引脚状态 (y=40~48) ----
    display.setCursor(0, 42);
    display.print("A:");
    display.print(digitalRead(ENC_A));
    display.setCursor(36, 42);
    display.print("B:");
    display.print(digitalRead(ENC_B));
    display.setCursor(72, 42);
    display.print("SW:");
    display.print(digitalRead(ENC_SW));

    // ---- 分隔线 + 底部提示 (y=54~63) ----
    display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
    display.setCursor(0, 56);
    display.print("Turn or press btn");

    display.display();

    lastDisplayCount = count;
    lastDisplayButton = pressed;
}

// ===== 主程序 =====
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ESP32-S3 EC11 Encoder Test ===");

    setupOled();
    setupEncoder();

    Serial.println("[OK] Ready. Turn the encoder or press the button.");
}

void loop() {
    renderDisplay();
}
