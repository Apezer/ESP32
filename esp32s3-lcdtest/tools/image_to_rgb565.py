#!/usr/bin/env python3
"""
将图片文件转换为 TFT_eSPI 可直接使用的 RGB565 头文件。

示例：
  python tools/image_to_rgb565.py "C:/path/to/photo.jpg" -o src/img.h

生成的头文件会包含：
  const uint16_t img[0x4000] PROGMEM = { ... };

本工程的 main.cpp 使用下面的方式显示：
  tft.setSwapBytes(true);
  tft.pushImage(0, 0, 128, 128, img);
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="将图片转换为 TFT_eSPI 使用的 16 位 RGB565 头文件。"
    )
    parser.add_argument("input", type=Path, help="输入图片路径，例如 jpg/png。")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("src/img.h"),
        help="输出头文件路径。默认：src/img.h",
    )
    parser.add_argument(
        "--name",
        default="img",
        help="头文件里的 C 数组名。默认：img",
    )
    parser.add_argument(
        "--width",
        type=int,
        default=128,
        help="输出图片宽度。默认：128",
    )
    parser.add_argument(
        "--height",
        type=int,
        default=128,
        help="输出图片高度。默认：128",
    )
    parser.add_argument(
        "--fit",
        choices=("crop", "contain"),
        default="crop",
        help="crop 为铺满屏幕并裁剪；contain 为完整保留图片并补黑边。默认：crop",
    )
    return parser.parse_args()


def resize_image(image: Image.Image, width: int, height: int, fit: str) -> Image.Image:
    image = image.convert("RGB")

    if fit == "crop":
        src_w, src_h = image.size
        target_ratio = width / height
        src_ratio = src_w / src_h

        if src_ratio > target_ratio:
            crop_w = int(src_h * target_ratio)
            crop_h = src_h
        else:
            crop_w = src_w
            crop_h = int(src_w / target_ratio)

        left = (src_w - crop_w) // 2
        top = (src_h - crop_h) // 2
        image = image.crop((left, top, left + crop_w, top + crop_h))
        return image.resize((width, height), Image.Resampling.LANCZOS)

    # contain：完整保留图片，比例不匹配时用黑边补齐。
    canvas = Image.new("RGB", (width, height), (0, 0, 0))
    image.thumbnail((width, height), Image.Resampling.LANCZOS)
    left = (width - image.width) // 2
    top = (height - image.height) // 2
    canvas.paste(image, (left, top))
    return canvas


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def build_header(image: Image.Image, array_name: str, source: Path) -> str:
    if hasattr(image, "get_flattened_data"):
        source_pixels = image.get_flattened_data()
    else:
        source_pixels = image.getdata()

    pixels = [rgb888_to_rgb565(r, g, b) for r, g, b in source_pixels]
    guard = f"{array_name.upper()}_RGB565_H"
    pixel_count = len(pixels)
    pixel_count_hex = f"0x{pixel_count:X}"

    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#include <Arduino.h>",
        "",
        f"// 由图片转换生成：{source.name}",
        f"// 尺寸：{image.width}x{image.height}，格式：RGB565。",
        "// main.cpp 中通过 tft.setSwapBytes(true) 和 tft.pushImage() 显示。",
        f"const uint16_t {array_name}[{pixel_count_hex}] PROGMEM = {{",
    ]

    for i in range(0, pixel_count, 12):
        chunk = pixels[i : i + 12]
        lines.append("  " + ", ".join(f"0x{value:04X}" for value in chunk) + ",")

    lines.extend(
        [
            "};",
            "",
            f"#endif  // {guard}",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> None:
    args = parse_args()
    if not args.input.exists():
        raise FileNotFoundError(f"Input image not found: {args.input}")

    image = Image.open(args.input)
    image = resize_image(image, args.width, args.height, args.fit)
    header = build_header(image, args.name, args.input)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(header, encoding="utf-8")

    print(f"Wrote {args.output}")
    print(f"Array: {args.name}, pixels: {args.width}x{args.height}, fit: {args.fit}")


if __name__ == "__main__":
    main()
