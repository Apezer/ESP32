/*
 * ESP32-S3 + 1.44" ST7735S TFT LCD 测试程序
 *
 * 接线:
 *   LCD SCL/CLK  -> ESP32-S3 GPIO9
 *   LCD SDA/MOSI -> ESP32-S3 GPIO10
 *   LCD RES/RST  -> ESP32-S3 GPIO11
 *   LCD DC/A0    -> ESP32-S3 GPIO12
 *   LCD CS       -> ESP32-S3 GPIO13
 *   LCD BL/LED   -> 3.3V (常亮)
 *   LCD VCC      -> 3.3V
 *   LCD GND      -> GND
 *
 * 功能:
 *   1. 屏幕初始化 + 显示基本信息
 *   2. 红/绿/蓝三色填充测试
 *   3. 矩形/圆形/斜线图形测试
 *   4. 颜色条渐变测试
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// 创建 TFT 显示对象
TFT_eSPI tft = TFT_eSPI();

// 使用 TFT_eSPI 内置颜色宏 (自动适配 BGR 颜色顺序)

/*
 * 显示文字信息
 * 清屏为黑色，显示设备信息和 GPIO 引脚
 */
void showInfo() {
    tft.fillScreen(TFT_BLACK);

    // 标题
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("LCD OK");

    // 设备型号
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 40);
    tft.println("ESP32-S3");

    // 驱动芯片
    tft.setCursor(10, 55);
    tft.println("ST7735S 128x128");

    // GPIO 引脚信息
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 75);
    tft.println("SCK:9  MOSI:10");
    tft.setCursor(10, 90);
    tft.println("RST:11 DC:12");
    tft.setCursor(10, 105);
    tft.println("CS:13  BL:3.3V");

    delay(3000);
}

/*
 * 三色填充测试
 * 依次用红、绿、蓝填满整个屏幕
 */
void colorFillTest() {
    // 红色填充
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(25, 55);
    tft.println("RED");
    delay(1000);

    // 绿色填充
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.setTextSize(2);
    tft.setCursor(25, 55);
    tft.println("GREEN");
    delay(1000);

    // 蓝色填充
    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setTextSize(2);
    tft.setCursor(25, 55);
    tft.println("BLUE");
    delay(1000);
}

/*
 * 图形测试
 * 绘制矩形、圆形、斜线
 */
void graphicsTest() {
    tft.fillScreen(TFT_BLACK);

    // 标题
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 2);
    tft.println("Graphics Test");

    // 矩形 (左上)
    tft.drawRect(5, 15, 50, 40, TFT_RED);
    tft.fillRect(10, 20, 40, 30, TFT_RED);

    // 圆角矩形 (右上)
    tft.drawRoundRect(70, 15, 50, 40, 8, TFT_GREEN);
    tft.fillRoundRect(75, 20, 40, 30, 6, TFT_GREEN);

    // 圆形 (左下)
    tft.drawCircle(30, 85, 20, TFT_CYAN);
    tft.fillCircle(30, 85, 15, TFT_CYAN);

    // 三角形 (右下)
    tft.drawTriangle(85, 65, 115, 105, 70, 105, TFT_MAGENTA);
    tft.fillTriangle(90, 70, 110, 100, 75, 100, TFT_MAGENTA);

    // 斜线
    tft.drawLine(5, 110, 122, 110, TFT_YELLOW);
    for (int i = 0; i < 120; i += 10) {
        tft.drawLine(5, 115 + (i / 10) * 2, 5 + i, 127, TFT_ORANGE);
    }

    delay(3000);
}

/*
 * 颜色条测试
 * 显示横向颜色渐变条
 */
void colorBarTest() {
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 2);
    tft.println("Color Bars");

    // 各色颜色条
    int barHeight = 12;
    int y = 16;
    const uint16_t colors[] = {
        TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREEN,
        TFT_MAGENTA, TFT_RED, TFT_BLUE, TFT_BLACK
    };
    const char* names[] = {
        "WHT", "YEL", "CYN", "GRN",
        "MAG", "RED", "BLU", "BLK"
    };

    for (int i = 0; i < 8; i++) {
        tft.fillRect(0, y, 100, barHeight, colors[i]);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(105, y + 2);
        tft.println(names[i]);
        y += barHeight + 2;
    }

    // 底部灰度渐变条
    y += 4;
    tft.setCursor(10, y);
    tft.println("Grayscale:");
    y += 12;
    for (int i = 0; i < 128; i++) {
        // 灰度: 从黑到白
        uint8_t gray = (i * 255) / 127;
        uint16_t grayColor = tft.color565(gray, gray, gray);
        tft.drawFastVLine(i, y, 15, grayColor);
    }

    delay(3000);
}

void setup() {
    // 初始化串口
    Serial.begin(115200);
    Serial.println("ESP32-S3 ST7735S LCD Test Starting...");

    // 初始化 TFT
    tft.init();
    tft.setRotation(0);  // 竖屏模式 (0=竖屏, 1=横屏)
    tft.fillScreen(TFT_BLACK);

    Serial.println("LCD initialized, running tests...");
    Serial.printf("Screen: %dx%d\n", tft.width(), tft.height());

    // 测试 1: 显示基本信息
    Serial.println("[1/4] Showing info...");
    showInfo();

    // 测试 2: 红绿蓝三色填充
    Serial.println("[2/4] Color fill test...");
    colorFillTest();

    // 测试 3: 图形绘制
    Serial.println("[3/4] Graphics test...");
    graphicsTest();

    // 测试 4: 颜色条
    Serial.println("[4/4] Color bar test...");
    colorBarTest();

    // 测试完成，回到信息页
    Serial.println("All tests done!");
    showInfo();
}

void loop() {
    // 主循环：切换显示测试内容
    static unsigned long lastSwitch = 0;
    static int page = 0;

    if (millis() - lastSwitch > 5000) {
        lastSwitch = millis();
        page = (page + 1) % 3;

        switch (page) {
            case 0:
                showInfo();
                break;
            case 1:
                graphicsTest();
                break;
            case 2:
                colorBarTest();
                break;
        }
    }
}
