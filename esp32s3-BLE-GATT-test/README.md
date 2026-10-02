# esp32s3-BLE-GATT-test — BLE 键盘与 GATT 状态通道

## 工程用途

测试 BLE HID 键盘同时暴露自定义 GATT 服务，PC Python 工具发送 JSON 状态并在 SSD1306 OLED 显示，可持续监控 status.json。OLED SDA=10、SCL=9；详细协议和启动方式见原说明。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。

---

## 原有详细说明

# ESP32-S3 BLE HID + GATT Status Test

This project tests whether a BLE HID keyboard can expose an additional custom GATT status channel.

The ESP32-S3 advertises as a BLE keyboard and also exposes a custom BLE service:

- Device name: `Vico Keyboard ESP32-S3` (may appear as `Vico Keyboard E`)
- HID service UUID: `1812`
- Service UUID: `7b6a0001-7c6e-4b3d-9f5f-7669636f0001`
- RX/write UUID: `7b6a0002-7c6e-4b3d-9f5f-7669636f0002`
- TX/notify UUID: `7b6a0003-7c6e-4b3d-9f5f-7669636f0003`

OLED display:

- SSD1306 128x64 I2C, address `0x3C`
- SDA: GPIO 10
- SCL: GPIO 9
- Shows BLE state (`ADV`/`LINK`), Claude Code `state`, `tool`, `text`, and last update time.

Test flow:

1. Flash the ESP32-S3.
2. Open serial monitor at `115200`.
3. Open Windows Bluetooth settings and pair `Vico Key`.
4. Edit `status.json`.
5. Send it from the PC:

```powershell
python -m pip install bleak
python tools/send_status_ble.py status.json
```

To observe the BLE connection state on the OLED, keep the connection open:

```powershell
python tools/send_status_ble.py status.json --hold 30
```

If the keyboard is already connected and no longer advertising, connect by address/device id:

```powershell
python tools/send_status_ble.py status.json --address XX:XX:XX:XX:XX:XX --hold 30
```

The ESP32 prints the received status JSON and sends an ACK notification.

Quick start the persistent JSON-to-GATT watcher:

```powershell
.\start_gatt_watch.bat
```
