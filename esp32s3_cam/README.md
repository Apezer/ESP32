# esp32s3_cam — OV5640 Wi-Fi 摄像头

## 工程用途

摄像头采集 JPEG，通过网页预览、拍照和 MJPEG 视频流输出，支持分辨率、质量、镜像和翻转控制。AP：ESP32S3-OV5640，密码 12345678，访问 http://192.168.4.1/；GPIO 见 include/camera_pins.h。路由器凭据已清空，按需修改 include/app_config.h。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。

---

## 原有详细说明

# ESP32-S3 OV5640 Wi-Fi camera

Pipeline: `OV5640 -> ESP32-S3 -> Wi-Fi -> browser MJPEG stream`.

## First run

1. Connect the OV5640 ribbon cable with pin 1 aligned to the camera connector marker.
2. Build and upload with PlatformIO.
3. Open the serial monitor at 115200 baud.
4. Connect a phone or computer to Wi-Fi `ESP32S3-OV5640`, password `12345678`.
5. Open `http://192.168.4.1/`.

To use an existing 2.4 GHz router instead, edit `include/app_config.h` and fill in
`WIFI_STA_SSID` and `WIFI_STA_PASSWORD`. If the connection fails after 15 seconds,
the firmware automatically falls back to access-point mode.

The web page provides a live MJPEG image, still capture, resolution, JPEG quality,
vertical flip and horizontal mirror controls. Port 80 serves the page and port 81
serves the MJPEG stream.

The PlatformIO board profile matches the user's existing projects. Camera GPIOs
follow the physical Freenove ESP32-S3 WROOM camera connector and are recorded in
`include/camera_pins.h`.
