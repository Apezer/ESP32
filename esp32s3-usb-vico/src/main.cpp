/**
 * ESP32-S3 USB Vico Keyboard
 *
 * Hardware:
 *   DFRobot FireBeetle2 ESP32-S3
 *   SSD1306 OLED 128x64 on I2C
 *   8 buttons wired to GND, using INPUT_PULLUP
 *   8 WS2812B LEDs on GPIO 17
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "font.h"
#include "rgb_led.h"

#if !ARDUINO_USB_MODE
#error "This sketch requires native USB mode. Set ARDUINO_USB_MODE=1."
#endif

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDR       0x3C

#define I2C_SDA         10
#define I2C_SCL         9

#define NUM_KEYS        8
#define DEBOUNCE_MS     10

const uint8_t KEY_PINS[NUM_KEYS] = {4, 5, 6, 7, 8, 14, 15, 16};
const char KEY_CHARS[NUM_KEYS] = {'z', 'x', 'c', 'v', 'b', 'n', 'm', 'a'};
const char *KEY_LABELS[NUM_KEYS] = {
    "Z", "X", "C", "V",
    "B", "N", "M", "A",
};

static bool key_state[NUM_KEYS] = {false};
static bool last_reported[NUM_KEYS] = {false};
static unsigned long last_change[NUM_KEYS] = {0};
static bool display_dirty = true;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
USBHIDKeyboard usbKeyboard;

void handleKeyPress(uint8_t index) {
    if (index == 0) {
        rgbLedNextEffect();
    }

    usbKeyboard.press(KEY_CHARS[index]);
}

void handleKeyRelease(uint8_t index) {
    usbKeyboard.release(KEY_CHARS[index]);
}

void showSplash() {
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

void drawKeyBox(uint8_t index, bool pressed) {
    const uint8_t box_w = 28;
    const uint8_t box_h = 14;
    const uint8_t gap = 2;
    const uint8_t start_x = 4;
    const uint8_t start_y = 18;

    const uint8_t col = index % 4;
    const uint8_t row = index / 4;
    const uint8_t x = start_x + col * (box_w + gap);
    const uint8_t y = start_y + row * (box_h + gap);

    if (pressed) {
        display.fillRect(x, y, box_w, box_h, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    } else {
        display.drawRect(x, y, box_w, box_h, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
    }

    display.setTextSize(1);
    display.setCursor(x + (box_w - 6) / 2, y + (box_h - 8) / 2);
    display.print(KEY_LABELS[index]);
}

void renderKeyStatus() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.print("Vico USB-S3");

    display.setCursor(104, 0);
    display.print("USB");

    display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        drawKeyBox(i, key_state[i]);
    }

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 54);
    display.print(rgbLedGetEffectName());
    display.setCursor(84, 54);
    display.print("GPIO17");

    display.display();
}

void scanKeys() {
    bool changed = false;
    const unsigned long now = millis();

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        const bool raw = (digitalRead(KEY_PINS[i]) == LOW);

        if (raw != key_state[i] && (now - last_change[i]) >= DEBOUNCE_MS) {
            key_state[i] = raw;
            last_change[i] = now;
            changed = true;
        }
    }

    if (!changed) {
        return;
    }

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        if (key_state[i] && !last_reported[i]) {
            handleKeyPress(i);
        } else if (!key_state[i] && last_reported[i]) {
            handleKeyRelease(i);
        }
        last_reported[i] = key_state[i];
    }

    display_dirty = true;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("=== Vico USB Keyboard ESP32-S3 ===");

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        pinMode(KEY_PINS[i], INPUT_PULLUP);
    }

    Wire.begin(I2C_SDA, I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[FAIL] SSD1306 init failed!");
        while (true) delay(1000);
    }

    rgbLedInit();

    usbKeyboard.begin();
    USB.begin();

    showSplash();
    delay(1200);
    display_dirty = true;
}

void loop() {
    rgbLedUpdate();
    scanKeys();

    static unsigned long last_draw = 0;
    const unsigned long now = millis();
    if (display_dirty || now - last_draw > 500) {
        last_draw = now;
        display_dirty = false;
        renderKeyStatus();
    }
}
