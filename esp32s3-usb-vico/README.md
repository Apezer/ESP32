# esp32s3-usb-vico — 八键 USB HID 键盘

## 工程用途

八个按键输出 z/x/c/v/b/n/m/a，OLED 显示按键状态，8 颗 WS2812B 显示灯效，第一键切换灯效。按键 GPIO：4/5/6/7/8/14/15/16，另一端接 GND；OLED SDA=10、SCL=9，灯带数据=17。代码使用 USBHIDKeyboard；需使用支持原生 USB 的接口，当前 USB 宏配置与枚举行为尚未实机验证。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。
