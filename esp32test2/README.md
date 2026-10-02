# esp32test2 — 单颗 RGB LED 七彩渐变

## 工程用途

使用 FastLED_min 驱动 GPIO48 上的一颗 RGB LED，亮度 50，循环在红橙黄绿青蓝紫之间渐变。已将本地绝对路径依赖改为随工程提供的 lib/FastLED_min。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。
