# esp32s3-ec11 — EC11 旋转编码器测试

## 工程用途

OLED 显示旋转方向、累计计数、按键和引脚状态。EC11 A=11、B=12、SW=13；OLED SDA=10、SCL=9、地址 0x3C。编码器信号使用内部上拉。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。
