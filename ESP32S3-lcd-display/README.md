# ESP32S3-lcd-display — ST7735S LCD 基础测试

## 工程用途

测试 1.44 英寸 128×128 TFT 的信息页、纯色、图形、颜色条和渐变。SPI CLK=9、MOSI=10、RST=11、DC=12、CS=13，背光接 3.3V。TFT_eSPI 配置已写入 platformio.ini。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。
