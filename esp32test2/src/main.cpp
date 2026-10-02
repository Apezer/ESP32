#include <Arduino.h>
#include <FastLED_min.h>

#define LED_PIN 48       // GPIO pin for data
#define NUM_LEDS 1       // Number of LEDs in your strip
#define BRIGHTNESS 50    // 0-255 (50 = ~20% brightness)
#define FADE_DELAY 10    // 渐变速度 (ms)，值越小越快

// Create LED array
CRGB leds[NUM_LEDS];

// 七彩颜色定义 (红、橙、黄、绿、青、蓝、紫)
const CRGB rainbowColors[] = {
  CRGB(255, 0, 0),      // 红
  CRGB(255, 127, 0),    // 橙
  CRGB(255, 255, 0),    // 黄
  CRGB(0, 255, 0),      // 绿
  CRGB(0, 255, 255),    // 青
  CRGB(0, 0, 255),      // 蓝
  CRGB(127, 0, 255)     // 紫
};
const int NUM_COLORS = 7;

// Function declarations
void sevenColorFade();
CRGB lerpColor(CRGB c1, CRGB c2, int step, int totalSteps);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Seven Color Fade Effect ===");
  Serial.printf("Pin: %d | LEDs: %d | Brightness: %d\n", LED_PIN, NUM_LEDS, BRIGHTNESS);

  // Initialize FastLED_min
  FASTLED_MIN_SETUP(LED_PIN, leds, NUM_LEDS);
  FastLED_min<LED_PIN>.setBrightness(BRIGHTNESS);

  Serial.println("Ready!");
}

void loop() {
  // 七彩轮流渐变效果
  sevenColorFade();
}

// 七彩轮流渐变效果
void sevenColorFade() {
  for (int colorIndex = 0; colorIndex < NUM_COLORS; colorIndex++) {
    int nextIndex = (colorIndex + 1) % NUM_COLORS;

    // 从当前颜色渐变到下一个颜色
    for (int step = 0; step <= 255; step++) {
      leds[0] = lerpColor(rainbowColors[colorIndex], rainbowColors[nextIndex], step, 255);
      FastLED_min<LED_PIN>.show();
      delay(FADE_DELAY);
    }
  }
}

// 线性插值：在两个颜色之间平滑过渡
CRGB lerpColor(CRGB c1, CRGB c2, int step, int totalSteps) {
  uint8_t r = c1.r + ((int)(c2.r - c1.r) * step / totalSteps);
  uint8_t g = c1.g + ((int)(c2.g - c1.g) * step / totalSteps);
  uint8_t b = c1.b + ((int)(c2.b - c1.b) * step / totalSteps);
  return CRGB(r, g, b);
}
