# esp32s3-wifi-video — PC 视频无线播放到 OLED

## 工程用途

stream.py 将电脑视频转为 128×64 单色帧，经 Wi-Fi TCP 发送到 SSD1306。OLED SDA=10、SCL=9；AP：ESP32S3-VIDEO，密码 12345678，TCP 端口 8080。PC 依赖 opencv-python 和 numpy，具体参数见原说明。

## 开发环境与使用

- 板型：dfrobot_firebeetle2_esp32s3
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。

---

## 原有详细说明

# ESP32-S3 WiFi Video OLED Streamer

通过 WiFi 将 PC 上的视频实时传输到 ESP32-S3，在 128x64 SSD1306 OLED 单色屏幕上播放。

## Architecture

```
[PC: Python + OpenCV] --WiFi TCP--> [ESP32-S3 AP] --I2C--> [SSD1306 OLED 128x64]
```

- **PC 端**: Python 脚本读取视频，逐帧转为 128x64 单色位图，通过 TCP 发送
- **ESP32 端**: 创建 WiFi AP，监听 TCP 连接，接收帧数据直接写入 OLED 显示缓冲区
- **协议**: 每帧 = `[0xAA][0x55]` 同步头 + 1024 字节位图数据

## Hardware

### Components

| Component | Model |
|-----------|-------|
| MCU | DFRobot FireBeetle 2 ESP32-S3 |
| Display | SSD1306 128x64 OLED (I2C) |

### Wiring

| OLED Pin | ESP32-S3 GPIO |
|----------|---------------|
| SDA | GPIO 10 |
| SCL | GPIO 9 |
| VCC | 3.3V |
| GND | GND |

## Software Setup

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- Python 3.7+
- `opencv-python` and `numpy`

### ESP32 Firmware

```bash
# Clone
git clone git@github.com:Apezer/ESP32.git
cd ESP32/esp32s3-wifi-video

# Build & Upload
pio run -t upload

# Monitor serial output
pio device monitor
```

### Python Dependencies

```bash
pip install opencv-python numpy
```

## Usage

### Step 1: Upload firmware to ESP32

```bash
pio run -t upload
```

### Step 2: Connect PC to ESP32 WiFi

| Setting | Value |
|---------|-------|
| SSID | `ESP32S3-VIDEO` |
| Password | `12345678` |
| ESP32 IP | `192.168.4.1` |
| TCP Port | `8080` |

### Step 3: Run the stream script

```bash
# Basic usage
python stream.py video.mp4

# With preview windows (original + OLED effect)
python stream.py video.mp4 --show

# Limit FPS to 15
python stream.py video.mp4 --fps 15

# Custom ESP32 IP
python stream.py video.mp4 --ip 192.168.4.1 --port 8080
```

### Command Line Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `video` | (required) | Path to video file |
| `--ip` | `192.168.4.1` | ESP32 IP address |
| `--port` | `8080` | TCP port |
| `--fps` | `0` (unlimited) | Target FPS (0 = as fast as possible) |
| `--show` | off | Show preview windows on PC |

Press `q` to stop when preview windows are open.

## Configuration

### Adjusting Threshold (brightness)

In `stream.py`, change the threshold value at the top of the file:

```python
THRESHOLD = 170  # 0~255, lower = brighter (more white pixels)
```

### Adjusting OLED Contrast

In `src/main.cpp`, modify the contrast value:

```cpp
display.contrast(0xFF);  // 0x00~0xFF, 0xFF = maximum
```

### Changing WiFi AP Name/Password

In `src/main.cpp`:

```cpp
#define AP_SSID  "ESP32S3-VIDEO"
#define AP_PASS  "12345678"
```

### I2C Pins

In `src/main.cpp`:

```cpp
#define I2C_SDA  10   // GPIO 10
#define I2C_SCL  9    // GPIO 9
```

## Performance

| Metric | Value |
|--------|-------|
| I2C Clock | 800 kHz |
| Frame Size | 1024 bytes (128x64 / 8) |
| Display Refresh | ~20ms per frame |
| Theoretical Max FPS | ~40-50 fps |
| WiFi Protocol | TCP with sync header |

## Project Structure

```
esp32s3-wifi-video/
├── include/
│   └── font.h              # Bitmap icon declarations
├── src/
│   ├── main.cpp            # ESP32 firmware (WiFi AP + TCP + OLED)
│   └── font.c              # Bitmap icon data (smiley, heart, wifi, etc.)
├── stream.py               # PC-side Python video streamer
├── platformio.ini          # PlatformIO build config
└── README.md
```

## How It Works

1. **ESP32 boots** -> Initializes OLED, shows splash screen, starts WiFi AP and TCP server
2. **PC connects** to ESP32's WiFi hotspot
3. **Python script** reads video with OpenCV, for each frame:
   - Resizes to 128x64
   - Converts to grayscale
   - Applies binary threshold
   - Packs into SSD1306 page format (column-major, 8 vertical pixels per byte)
   - Sends `[0xAA 0x55]` sync header + 1024 bytes via TCP
4. **ESP32 receives** frame, writes directly to display buffer via `getBuffer()`, calls `display()`

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `SSD1306 init failed` | Check I2C wiring (SDA/SCL), verify OLED address (0x3C) |
| `Connection failed` | Ensure PC is connected to ESP32 AP, check IP/port |
| Low FPS | Reduce video resolution, try `--fps` to limit, check WiFi signal |
| No display | Verify OLED is powered (3.3V + GND), check I2C pins |
| Video looks too dark | Lower `THRESHOLD` value in `stream.py` |
| Video looks too bright | Raise `THRESHOLD` value in `stream.py` |
