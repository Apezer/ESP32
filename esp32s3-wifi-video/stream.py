"""
stream.py - PC端视频流发送脚本

将视频文件解码为 128x64 单色位图，通过 WiFi TCP 发送到 ESP32-S3 OLED 显示。

用法:
    python stream.py video.mp4
    python stream.py video.mp4 --ip 192.168.4.1 --port 8080

依赖:
    pip install opencv-python numpy
"""

import sys
import socket
import struct
import time
import argparse

import cv2
import numpy as np

# ===== 配置 =====
DEFAULT_IP = "192.168.4.1"   # ESP32 AP 默认 IP
DEFAULT_PORT = 8080
SYNC_BYTES = bytes([0xAA, 0x55])
FRAME_WIDTH = 128
FRAME_HEIGHT = 64
FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT // 8  # 1024 bytes
THRESHOLD = 170  # 二值化阈值 (0~255, 越低越亮)


def frame_to_ssd1306(frame: np.ndarray) -> bytes:
    """
    将 OpenCV 帧转换为 SSD1306 OLED 的 page 格式缓冲区 (1024 bytes)。

    SSD1306 buffer 格式:
        buffer[page * 128 + x] 的 bit(y % 8) = pixel(x, y)
        page = y // 8, bit = y % 8 (bit 0 = 最上方像素)
    """
    # 缩放到 128x64
    resized = cv2.resize(frame, (FRAME_WIDTH, FRAME_HEIGHT), interpolation=cv2.INTER_AREA)

    # 转灰度
    if len(resized.shape) == 3:
        gray = cv2.cvtColor(resized, cv2.COLOR_BGR2GRAY)
    else:
        gray = resized

    # 二值化 (阈值 128)
    _, binary = cv2.threshold(gray, 180, 255, cv2.THRESH_BINARY)

    # 转为布尔数组 (True = 白色像素)
    pixels = binary > 0  # shape: (64, 128)

    # 打包为 SSD1306 page 格式
    # 每 page 8 行，共 8 个 page
    buf = bytearray(FRAME_SIZE)
    for page in range(8):
        for col in range(FRAME_WIDTH):
            byte = 0
            for bit in range(8):
                y = page * 8 + bit
                if pixels[y, col]:
                    byte |= (1 << bit)
            buf[page * FRAME_WIDTH + col] = byte

    return bytes(buf)


def frame_to_ssd1306_fast(frame: np.ndarray) -> bytes:
    """
    使用 numpy 向量化操作的快速版本。
    比逐像素循环快约 10-20 倍。
    """
    # 缩放 + 灰度
    resized = cv2.resize(frame, (FRAME_WIDTH, FRAME_HEIGHT), interpolation=cv2.INTER_AREA)
    if len(resized.shape) == 3:
        gray = cv2.cvtColor(resized, cv2.COLOR_BGR2GRAY)
    else:
        gray = resized

    # 二值化
    _, binary = cv2.threshold(gray, THRESHOLD, 255, cv2.THRESH_BINARY)

    # numpy 向量化打包
    # pixels shape: (64, 128), dtype: uint8 (0 or 255)
    pixels = binary > 0  # bool (64, 128)

    # reshape 为 (8, 8, 128) - 8个page, 每page 8行, 128列
    pages = pixels.reshape(8, 8, FRAME_WIDTH)

    # 每行的 bit 权重: [1, 2, 4, 8, 16, 32, 64, 128]
    bit_weights = np.array([1 << i for i in range(8)], dtype=np.uint8).reshape(8, 1)

    # 对每个 page 的 8 行做加权求和
    # pages: (8, 8, 128) * bit_weights: (8, 1) -> sum over axis=1 -> (8, 128)
    buf = np.tensordot(pages.astype(np.uint32), bit_weights, axes=([1], [0]))
    buf = buf.astype(np.uint8).reshape(FRAME_SIZE)

    return buf.tobytes()


def main():
    parser = argparse.ArgumentParser(description="Stream video to ESP32 OLED")
    parser.add_argument("video", help="Path to video file")
    parser.add_argument("--ip", default=DEFAULT_IP, help=f"ESP32 IP (default: {DEFAULT_IP})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"TCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--fps", type=int, default=0, help="Target FPS (0 = no limit, as fast as possible)")
    parser.add_argument("--show", action="store_true", help="Show preview window on PC")
    args = parser.parse_args()

    # 打开视频
    cap = cv2.VideoCapture(args.video)
    if not cap.isOpened():
        print(f"Error: Cannot open video '{args.video}'")
        sys.exit(1)

    video_fps = cap.get(cv2.CAP_PROP_FPS)
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    print(f"Video: {args.video}")
    print(f"Resolution: {int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))}x{int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}")
    print(f"FPS: {video_fps}, Total frames: {total_frames}")

    # 连接 ESP32
    print(f"Connecting to {args.ip}:{args.port}...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    sock.settimeout(10)

    try:
        sock.connect((args.ip, args.port))
    except (socket.timeout, ConnectionRefusedError, OSError) as e:
        print(f"Connection failed: {e}")
        print("Make sure ESP32 AP is running and you're connected to it.")
        sys.exit(1)

    print("Connected! Streaming...")

    # 帧率控制
    frame_interval = 1.0 / args.fps if args.fps > 0 else 0
    sent_count = 0
    start_time = time.time()
    fps_timer = start_time

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                # 视频结束，循环播放
                cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                ret, frame = cap.read()
                if not ret:
                    break

            frame_start = time.time()

            # 转换帧格式
            ssd1306_data = frame_to_ssd1306_fast(frame)

            # 发送: 同步头 + 帧数据
            try:
                sock.sendall(SYNC_BYTES + ssd1306_data)
            except (BrokenPipeError, ConnectionResetError, OSError) as e:
                print(f"\nConnection lost: {e}")
                break

            sent_count += 1

            # 帧率统计 (每秒打印)
            now = time.time()
            if now - fps_timer >= 1.0:
                elapsed = now - start_time
                avg_fps = sent_count / elapsed
                instant_fps = sent_count / (now - fps_timer + (now - fps_timer == 0))
                print(f"\rFrames: {sent_count}, Avg FPS: {avg_fps:.1f}", end="", flush=True)
                fps_timer = now

            # PC 端预览
            if args.show:
                # 原始视频 (缩放到与预览一致)
                cv2.imshow("Original", cv2.resize(frame, (256, 128)))
                # OLED 效果预览 (128x64 二值化)
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY) if len(frame.shape) == 3 else frame
                resized = cv2.resize(gray, (FRAME_WIDTH, FRAME_HEIGHT), interpolation=cv2.INTER_AREA)
                _, bw = cv2.threshold(resized, THRESHOLD, 255, cv2.THRESH_BINARY)
                preview = cv2.resize(bw, (256, 128), interpolation=cv2.INTER_NEAREST)
                cv2.imshow("OLED Preview", preview)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    print("\nStopped by user")
                    break

            # 帧率限制
            if frame_interval > 0:
                elapsed = time.time() - frame_start
                if elapsed < frame_interval:
                    time.sleep(frame_interval - elapsed)

    except KeyboardInterrupt:
        print("\nStopped by user")
    finally:
        cap.release()
        sock.close()
        if args.show:
            cv2.destroyAllWindows()
        elapsed = time.time() - start_time
        print(f"\nDone. Sent {sent_count} frames in {elapsed:.1f}s ({sent_count/elapsed:.1f} fps avg)")


if __name__ == "__main__":
    main()
