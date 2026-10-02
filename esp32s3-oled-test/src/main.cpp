/**
 * ESP32-S3 + SSD1306 OLED (128x64) Demo
 *
 * 硬件: DFRobot FireBeetle2 ESP32-S3 + 0.96寸 OLED (SSD1306, I2C)
 *
 * 接线:
 *   OLED SDA -> GPIO 1  (默认 I2C SDA)
 *   OLED SCL -> GPIO 2  (默认 I2C SCL)
 *   OLED VCC -> 3.3V
 *   OLED GND -> GND
 *
 * 依赖库 (platformio.ini 已配置):
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_system.h>
#include "font.h"

/* ========== 硬件配置 - 根据你的实际接线修改 ========== */
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1      // 无复位引脚填 -1
#define OLED_ADDR       0x3C    // 7位地址 (0x78 >> 1 = 0x3C)

// I2C 引脚 (DFRobot FireBeetle2 ESP32-S3 默认)
#define I2C_SDA         1
#define I2C_SCL         2

/* ========== 创建 OLED 对象 ========== */ 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* ========== 函数声明 ========== */
void show_splash();         // 开机画面
void show_text_demo();      // 文字显示演示
void show_graphics_demo();  // 图形绘制演示
void show_bitmap_demo();    // 位图表情演示
void show_custom_image();   // 自定义图片显示
void show_chinese_text();   // 中文字符显示
void show_animation();      // 简单动画演示

/* ========== setup ========== */
void setup()
{
    Serial.begin(115200);
    Serial.println("=== ESP32-S3 OLED Demo ===");

    // 初始化 I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    // 初始化 OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 init failed!");
        while (true) delay(1000);
    }
    Serial.println("SSD1306 init OK");

    // 开机画面
    show_splash();
    delay(2000);
}

/* ========== loop ========== */
void loop()
{
    // 轮流演示各种功能
    show_text_demo();
    delay(3000);

    show_graphics_demo();
    delay(3000);

    show_bitmap_demo();
    delay(3000);

    show_custom_image();
    delay(3000);

    show_chinese_text();
    delay(3000);

    show_animation();
    delay(1000);
}

/* ========== 开机画面 ========== */
void show_splash()
{
    display.clearDisplay();

    // 标题
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println("ESP32-S3");

    // 副标题
    display.setTextSize(1);
    display.setCursor(20, 24);
    display.println("OLED Demo");

    // 分隔线
    display.drawLine(0, 38, 127, 38, SSD1306_WHITE);

    // 底部信息
    display.setCursor(8, 46);
    display.println("128x64 SSD1306");
    display.setCursor(20, 56);
    display.println("I2C @ 0x3C");

    display.display();
}

/* ========== 文字显示演示 ========== */
void show_text_demo()
{
    display.clearDisplay();

    // 不同大小的文字
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);  // 6x8 像素
    display.setCursor(0, 0);
    display.println("Size 1: Hello!");

    display.setTextSize(2);  // 12x16 像素
    display.setCursor(0, 16);
    display.println("Size 2");

    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("Counter:");

    // 动态数字
    static int counter = 0;
    display.setTextSize(2);
    display.setCursor(64, 38);
    display.println(counter++);

    // 底部提示
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print("Uptime: ");
    display.print(millis() / 1000);
    display.print("s");

    display.display();
}

/* ========== 图形绘制演示 ========== */
void show_graphics_demo()
{
    display.clearDisplay();

    // 画线
    display.drawLine(0, 0, 127, 63, SSD1306_WHITE);
    display.drawLine(127, 0, 0, 63, SSD1306_WHITE);

    // 画矩形
    display.drawRect(2, 2, 40, 25, SSD1306_WHITE);
    display.fillRect(4, 4, 36, 21, SSD1306_WHITE);

    // 画圆
    display.drawCircle(90, 30, 20, SSD1306_WHITE);
    display.fillCircle(90, 30, 15, SSD1306_WHITE);

    // 画圆角矩形
    display.drawRoundRect(2, 35, 50, 26, 6, SSD1306_WHITE);

    // 画三角形
    display.drawTriangle(60, 40, 75, 60, 90, 40, SSD1306_WHITE);

    display.display();
}

/* ========== 位图表情演示 ========== */
void show_bitmap_demo()
{
    display.clearDisplay();

    // 标题
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Emotion Demo");

    // 笑脸
    display.drawBitmap(8, 20, smiley_bmp, 16, 16, SSD1306_WHITE);
    display.setCursor(6, 40);
    display.print("Happy");

    // 悲伤脸
    display.drawBitmap(52, 20, sad_bmp, 16, 16, SSD1306_WHITE);
    display.setCursor(50, 40);
    display.print("Sad");

    // 惊讶脸
    display.drawBitmap(96, 20, surprised_bmp, 16, 16, SSD1306_WHITE);
    display.setCursor(90, 40);
    display.print("Wow");

    // 底部分隔线
    display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
    display.setCursor(10, 56);
    display.print("16x16 bitmaps");

    display.display();
}

/* ========== 简单动画演示 ========== */
void show_animation()
{
    // 弹跳球动画
    int ball_x = 10, ball_y = 10;
    int dx = 2, dy = 1;
    const int radius = 4;

    for (int frame = 0; frame < 60; frame++) {
        display.clearDisplay();

        // 标题
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print("Bounce! Frame:");
        display.print(frame);

        // 边界
        display.drawRect(0, 10, 128, 54, SSD1306_WHITE);

        // 更新球位置
        ball_x += dx;
        ball_y += dy;

        // 碰撞检测
        if (ball_x - radius <= 1 || ball_x + radius >= 126) dx = -dx;
        if (ball_y - radius <= 11 || ball_y + radius >= 62) dy = -dy;

        // 画球
        display.fillCircle(ball_x, ball_y, radius, SSD1306_WHITE);

        display.display();
        delay(30);
    }
}

/* ========== 自定义图片显示演示 ========== */
void show_custom_image()
{
    display.clearDisplay();

    // 标题
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Custom Images");

    // 显示 WiFi 图标 (32x32)
    display.drawBitmap(10, 16, wifi_icon, 32, 32, SSD1306_WHITE);
    display.setCursor(10, 52);
    display.print("WiFi");

    // 显示心形图标 (16x16)
    display.drawBitmap(80, 20, heart_icon, 16, 16, SSD1306_WHITE);
    display.setCursor(78, 40);
    display.print("Heart");

    // 显示不同大小的文字
    display.setCursor(80, 52);
    display.print("Font");

    display.display();
}

/* ========== 中文字符显示演示 ========== */
void show_chinese_text()
{
    display.clearDisplay();

    // 标题
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Chinese Text Demo");

    // 注意: Adafruit GFX 默认不支持中文
    // 需要使用自定义字体或位图方式显示中文

    // 方法1: 使用拼音或英文替代
    display.setCursor(0, 16);
    display.println("Ni Hao World!");

    // 方法2: 显示 ASCII 艺术字
    display.setCursor(0, 28);
    display.println("ESP32 + OLED");

    // 方法3: 使用 drawBitmap 显示自定义中文位图
    // 需要先将中文转换为位图数据

    // 显示系统信息
    display.setCursor(0, 42);
    display.print("RAM: ");
    display.print(ESP.getFreeHeap() / 1024);
    display.print("KB");

    display.setCursor(0, 54);
    display.print("Time: ");
    display.print(millis() / 1000);
    display.print("s");

    display.display();
}
