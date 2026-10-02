#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>

// img.h 来自 ESP32TFT-main 里的 TFT_1pic 示例。
// 文件中保存了一张 128x128 的 RGB565 图片数组，数组名为 `img`。
#include "img.h"

TFT_eSPI tft;

// 当前演示程序按 128x128 的 ST7735 屏幕布局。
constexpr int16_t SCREEN_W = 128;
constexpr int16_t SCREEN_H = 128;

// 将 8 位 RGB 颜色转换成 TFT_eSPI 使用的 16 位 RGB565 颜色。
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return tft.color565(r, g, b);
}

// 绘制每个测试页面顶部复用的标题栏。
void header(const char *title, uint16_t color) {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, SCREEN_W, 16, color);

  // 将标题居中显示在 16 像素高的标题栏里。
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, color);
  tft.drawString(title, SCREEN_W / 2, 8, 1);
  tft.setTextDatum(TL_DATUM);
}

// 开机欢迎页，同时检查文字和简单图形是否能正常显示。
void bootScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRoundRect(8, 8, 112, 112, 8, TFT_CYAN);
  tft.fillRoundRect(14, 14, 100, 100, 6, rgb(10, 18, 28));

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, rgb(10, 18, 28));
  tft.drawString("ESP32-S3", 64, 34, 2);
  tft.setTextColor(TFT_WHITE, rgb(10, 18, 28));
  tft.drawString("TFT Demo", 64, 58, 2);
  tft.setTextColor(TFT_YELLOW, rgb(10, 18, 28));
  tft.drawString("128 x 128", 64, 82, 1);

  // 底部小圆点加载动画。
  for (int x = 22; x <= 106; x += 7) {
    tft.fillCircle(x, 102, 2, rgb(20 + x, 220 - x, 120));
    delay(35);
  }
  delay(700);
}

// 颜色测试页：检查 RGB/BGR 顺序、亮度和渐变显示效果。
void colorBars() {
  header("COLOR TEST", TFT_YELLOW);

  const uint16_t colors[] = {
      TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN,
      TFT_CYAN, TFT_BLUE, TFT_MAGENTA, TFT_WHITE,
  };

  for (int i = 0; i < 8; ++i) {
    tft.fillRect(i * 16, 22, 16, 48, colors[i]);
  }

  // 在色条下方绘制一段 RGB565 渐变。
  for (int y = 76; y < 112; ++y) {
    uint8_t v = map(y, 76, 111, 20, 255);
    tft.drawFastHLine(8, y, 112, rgb(v, v / 2, 255 - v));
  }

  tft.drawRect(7, 75, 114, 38, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("RGB order / brightness", 8, 116, 1);
  delay(1800);
}

// 基础图形页：测试 TFT_eSPI 常用的基础绘图函数。
void graphicsPage() {
  header("GRAPHICS", TFT_CYAN);

  tft.drawRect(8, 24, 28, 20, TFT_BLUE);
  tft.fillRect(13, 29, 18, 10, TFT_YELLOW);

  tft.drawCircle(64, 34, 13, TFT_MAGENTA);
  tft.fillCircle(96, 34, 11, TFT_CYAN);

  tft.drawTriangle(16, 86, 36, 60, 56, 86, TFT_ORANGE);
  tft.fillTriangle(72, 86, 92, 60, 112, 86, TFT_PURPLE);

  tft.drawRoundRect(11, 96, 44, 22, 6, TFT_GREEN);
  tft.fillRoundRect(74, 96, 44, 22, 6, TFT_RED);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("pixel line rect circle", 8, 48, 1);
  delay(1800);
}

// 文字测试页：确认内置字体和文字颜色可以正常显示。
void textPage() {
  header("TEXT TEST", TFT_GREEN);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Font 1: Hello,world!", 8, 24, 1);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Font 2", 8, 42, 2);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("123", 72, 35, 4);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("TFT_eSPI works", 8, 82, 1);

  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("ASCII text OK", 8, 100, 1);
  delay(2000);
}

// 图片显示页：显示 img.h 中的整屏 RGB565 图片。
void imagePage() {
  tft.fillScreen(TFT_BLACK);

  // pushImage 会把 RGB565 像素数组直接画到屏幕上。
  // 这张图片的数据需要在 setup() 中调用 setSwapBytes(true)，否则颜色可能不对。
  tft.pushImage(0, 0, SCREEN_W, SCREEN_H, img);
  delay(5000);
}

// 字符雨动画页：按列绘制下落字符。
void matrixPage() {
  header("MATRIX", rgb(0, 180, 80));

  const char symbols[] = "01ESP32TFT";
  for (int frame = 0; frame < 42; ++frame) {
    tft.fillRect(0, 17, SCREEN_W, 111, TFT_BLACK);
    for (int x = 0; x < SCREEN_W; x += 8) {
      // 每一列都有一个移动的高亮字符头和一小段渐暗尾巴。
      int head = (frame * 5 + x * 3) % 112 + 17;
      for (int j = 0; j < 6; ++j) {
        int y = head - j * 12;
        if (y < 18 || y > 122) {
          continue;
        }

        // 头部使用亮色，尾部使用逐渐变暗的绿色。
        uint16_t c = j == 0 ? TFT_WHITE : rgb(0, 220 - j * 28, 60);
        tft.setTextColor(c, TFT_BLACK);
        char s[2] = {symbols[(x / 8 + frame + j) % (sizeof(symbols) - 1)], '\0'};
        tft.drawString(s, x, y, 1);
      }
    }
    delay(45);
  }
}

// 线框立方体页：旋转 8 个三维顶点，并投影到二维屏幕上。
void cubePage() {
  header("CUBE", TFT_MAGENTA);

  // 立方体的 8 个顶点，本地三维坐标。
  const int8_t points[8][3] = {
      {-20, -20, -20}, {20, -20, -20}, {20, 20, -20}, {-20, 20, -20},
      {-20, -20, 20},  {20, -20, 20},  {20, 20, 20},  {-20, 20, 20},
  };

  // 每两个顶点组成一条边，共 12 条边。
  const uint8_t edges[12][2] = {
      {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
      {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
  };

  for (int frame = 0; frame < 90; ++frame) {
    int16_t sx[8];
    int16_t sy[8];
    float a = frame * 0.08f;
    float b = frame * 0.05f;

    for (int i = 0; i < 8; ++i) {
      // 让每个顶点绕两个轴旋转，再做一个简单的透视缩放。
      float x = points[i][0];
      float y = points[i][1];
      float z = points[i][2];

      float x1 = x * cosf(a) - z * sinf(a);
      float z1 = x * sinf(a) + z * cosf(a);
      float y1 = y * cosf(b) - z1 * sinf(b);
      float z2 = y * sinf(b) + z1 * cosf(b);
      float scale = 78.0f / (90.0f + z2);

      sx[i] = 64 + int16_t(x1 * scale);
      sy[i] = 72 + int16_t(y1 * scale);
    }

    // 只清除标题栏下方的绘图区，保留顶部标题栏。
    tft.fillRect(0, 17, SCREEN_W, 111, TFT_BLACK);
    for (const auto &edge : edges) {
      tft.drawLine(sx[edge[0]], sy[edge[0]], sx[edge[1]], sy[edge[1]], TFT_CYAN);
    }
    tft.fillCircle(64, 72, 2, TFT_YELLOW);
    delay(35);
  }
}

// 状态信息页：显示当前工程名和重要接线信息。
void statusPage() {
  header("STATUS", TFT_ORANGE);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Project: lcdtest", 8, 22, 1);
  tft.drawString("Board  : ESP32-S3", 8, 38, 1);
  tft.drawString("LCD    : ST7735", 8, 54, 1);
  tft.drawString("Size   : 128x128", 8, 70, 1);
  tft.drawString("MOSI   : GPIO10", 8, 86, 1);
  tft.drawString("SCLK   : GPIO9", 8, 102, 1);
  tft.drawString("CS/DC  : 13 / 12", 8, 116, 1);
  delay(2200);
}

void setup() {
  tft.init();
  tft.setRotation(0);

  // 参考图片数据的字节顺序需要 TFT_eSPI 做交换。
  // 如果没有这句，图片可能会出现颜色异常。
  tft.setSwapBytes(true);

  // 设置演示页面默认使用的文字样式。
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  bootScreen();
}

void loop() {
  // 持续循环播放所有测试页面。
  colorBars();
  graphicsPage();
  textPage();
  imagePage();
  matrixPage();
  cubePage();
  statusPage();
}
