# esp32s3-oled-test — SSD1306 OLED 综合演示

## 工程用途

测试 128×64 OLED 的文字、几何图形、位图、自定义图片、中文和动画。I2C SDA=1、SCL=2、地址 0x3C；字体资源在 src/font.c 和 include/font.h。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。
