# esp32s3-lcdtest — TFT 多页面图形演示

## 工程用途

在 128×128 ST7735 上循环展示颜色、文字、图片、字符雨和旋转立方体。接线同 ESP32S3-lcd-display；tools/image_to_rgb565.py 用于生成图片数组。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。

---

## 原有详细说明

# ESP32-S3 LCD Display Demo

这是一个基于 PlatformIO + Arduino + TFT_eSPI 的 ESP32-S3 128x128 TFT 屏幕测试工程。程序会循环显示颜色、图形、文字、图片、字符雨、旋转立方体和状态信息，用来快速确认屏幕接线、颜色顺序、字体和刷新效果是否正常。

## 硬件

- 开发板：DFRobot FireBeetle 2 ESP32-S3
- 屏幕：128x128 ST7735 TFT LCD
- 显示库：TFT_eSPI

## 接线

| TFT 引脚 | ESP32-S3 引脚 | 说明 |
| --- | --- | --- |
| SCL / CLK | GPIO9 | SPI 时钟 |
| SDA / MOSI | GPIO10 | SPI 数据 |
| RES / RST | GPIO11 | 屏幕复位 |
| DC / A0 | GPIO12 | 数据/命令选择 |
| CS | GPIO13 | 片选 |
| BL / LED | 3.3V | 背光常亮 |
| VCC | 3.3V | 电源 |
| GND | GND | 地 |

对应配置在 `platformio.ini` 的 `build_flags` 中：

```ini
-D TFT_MOSI=10
-D TFT_SCLK=9
-D TFT_CS=13
-D TFT_DC=12
-D TFT_RST=11
```

## 编译和烧录

在工程根目录执行：

```powershell
pio run
pio run -t upload
```

串口监视器波特率为 `115200`：

```powershell
pio device monitor
```

## 代码结构

- `src/main.cpp`：主程序，负责初始化屏幕并循环显示各个测试页面。
- `src/img.h`：由 JPG/PNG 图片转换得到的 RGB565 图片数组。
- `tools/image_to_rgb565.py`：图片转 RGB565 头文件脚本。
- `platformio.ini`：PlatformIO 工程配置和 TFT_eSPI 引脚/字体配置。

## 页面说明

主循环位于 `src/main.cpp` 的 `loop()`：

```cpp
colorBars();
graphicsPage();
textPage();
imagePage();
matrixPage();
cubePage();
statusPage();
```

各页面用途：

- `bootScreen()`：开机欢迎页，确认基础图形和文字显示正常。
- `colorBars()`：颜色条和渐变测试，用于检查颜色顺序、亮度和 RGB565 渐变。
- `graphicsPage()`：基础图形测试，包括矩形、圆形、三角形、圆角矩形等。
- `textPage()`：文字测试，确认内置字体和文字颜色可用。
- `imagePage()`：使用 `tft.pushImage()` 显示 `src/img.h` 中的 128x128 RGB565 图片。
- `matrixPage()`：字符雨动画，测试局部刷新和动态文字。
- `cubePage()`：旋转线框立方体，测试连续绘图刷新。
- `statusPage()`：显示工程名、屏幕型号、分辨率和主要接线信息。

## 图片转换

本工程的图片显示方式参考 `TFT_1pic` 示例：把图片预先转换为 RGB565 数组，再通过 `tft.pushImage()` 直接显示。

转换脚本：

```powershell
python tools\image_to_rgb565.py "C:\path\to\photo.jpg" -o src\img.h
```

默认行为：

- 居中裁剪图片，让画面铺满 128x128 屏幕。
- 缩放为 `128x128`。
- 转换为 RGB565。
- 生成数组名为 `img` 的 `src/img.h`。

如果希望完整保留原图，不裁剪，可以使用黑边模式：

```powershell
python tools\image_to_rgb565.py "C:\path\to\photo.jpg" -o src\img.h --fit contain
```

`main.cpp` 中显示图片的关键代码：

```cpp
tft.setSwapBytes(true);
tft.pushImage(0, 0, SCREEN_W, SCREEN_H, img);
```

`setSwapBytes(true)` 用于匹配当前图片数组的字节顺序。如果去掉它，图片颜色可能会异常。

## TFT_eSPI 配置说明

因为工程在 `platformio.ini` 中使用了：

```ini
-D USER_SETUP_LOADED
```

所以需要显式声明屏幕驱动、尺寸、引脚和字体：

```ini
-D ST7735_DRIVER
-D TFT_WIDTH=128
-D TFT_HEIGHT=128
-D ST7735_GREENTAB3
-D TFT_RGB_ORDER=TFT_BGR
-D LOAD_GLCD
-D LOAD_FONT2
-D LOAD_FONT4
-D SMOOTH_FONT
```

其中 `LOAD_GLCD` 是基础文字显示所需的内置字体开关。
