# ESP32 / ESP8266 工程合集

收录 D:/esp 中的 15 个独立工程，涵盖 BLE/USB 键盘、摄像头、音频、OLED/TFT 显示、编码器和 LED 控制。每个目录独立使用；点击工程名查看用途、接线和运行说明。

## 工程目录

| 工程 | 用途 | PlatformIO 板型 / 环境 |
| --- | --- | --- |
| [esp32_ok](./esp32_ok/README.md) | ESP32 基础工程模板 | esp32dev |
| [ESP32-C3 BLE Keybrick](./ESP32-C3%20BLE%20Keybrick/README.md) | 五键 BLE 宏键盘 | esp32-c3-devkitm-1 |
| [esp32s3_cam](./esp32s3_cam/README.md) | OV5640 Wi-Fi 摄像头 | dfrobot_firebeetle2_esp32s3 |
| [esp32s3-BLE-GATT-test](./esp32s3-BLE-GATT-test/README.md) | BLE 键盘与 GATT 状态通道 | dfrobot_firebeetle2_esp32s3 |
| [esp32s3-ec11](./esp32s3-ec11/README.md) | EC11 旋转编码器测试 | dfrobot_firebeetle2_esp32s3 |
| [ESP32S3-lcd-display](./ESP32S3-lcd-display/README.md) | ST7735S LCD 基础测试 | dfrobot_firebeetle2_esp32s3 |
| [esp32s3-lcdtest](./esp32s3-lcdtest/README.md) | TFT 多页面图形演示 | dfrobot_firebeetle2_esp32s3 |
| [ESP32S3-microphone-test](./ESP32S3-microphone-test/README.md) | INMP441 无线录音 | dfrobot_firebeetle2_esp32s3 |
| [esp32s3-oled-test](./esp32s3-oled-test/README.md) | SSD1306 OLED 综合演示 | dfrobot_firebeetle2_esp32s3 |
| [esp32s3-usb-vico](./esp32s3-usb-vico/README.md) | 八键 USB HID 键盘 | dfrobot_firebeetle2_esp32s3 |
| [esp32s3-wifi-video](./esp32s3-wifi-video/README.md) | PC 视频无线播放到 OLED | dfrobot_firebeetle2_esp32s3 |
| [esp32test2](./esp32test2/README.md) | 单颗 RGB LED 七彩渐变 | dfrobot_firebeetle2_esp32s3 |
| [ESP32TFT-main](./ESP32TFT-main/README.md) | Arduino TFT 学习示例合集 | ESP32 / Arduino IDE |
| [esp8266_led_web](./esp8266_led_web/README.md) | ESP8266 网页灯控工程模板 | nodemcu |
| [esp8266_test](./esp8266_test/README.md) | ESP8266 网页控制板载 LED | nodemcuv2 |

## 快速开始

安装 PlatformIO，克隆仓库后进入要使用的工程目录：

```powershell
git clone git@github.com:Apezer/ESP32.git
cd ESP32/esp32s3-ec11
pio run
pio run -t upload
pio device monitor
```

TFT Arduino 示例合集需按其原教程单独打开。esp32_ok 和 esp8266_led_web 为缺少应用源码的模板。大部分 S3 项目配置为 FireBeetle 2，但实际板卡、摄像头连接器和 GPIO 必须以各工程接线为准。

## 整理与验证范围

- 保留源码、配置、自带库、工具、示例资源和原说明；不包含 .git、.pio、build、编辑器配置、日志及编译产物。
- 摄像头的真实路由器凭据已清空；TFT 时钟的网络配置改为占位符，自建 AP 的示例密码仍为 12345678。
- esp32test2 的 FastLED_min 库已随工程提供，移除机器绝对路径。
- 尚未执行固件编译、烧录或实机验证；test/README 是 PlatformIO 默认说明，不表示已有自动化测试。
- 保留已有作者署名与库许可证；第三方 TFT 示例沿用原资料，使用与分发前查看相应授权。
