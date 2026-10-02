/**
 * font.h - 字模数据声明
 *
 * 包含 OLED 显示用的位图图标数据
 * 所有图标均为单色 (1-bit) 位图，使用 PROGMEM 存储到 Flash
 */

#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 16x16 表情图标 ========== */
extern const uint8_t smiley_bmp[];      // 笑脸
extern const uint8_t sad_bmp[];         // 悲伤脸
extern const uint8_t surprised_bmp[];   // 惊讶脸
extern const uint8_t heart_icon[];      // 心形

/* ========== 32x32 图标 ========== */
extern const uint8_t wifi_icon[];       // WiFi 图标

#ifdef __cplusplus
}
#endif

#endif /* FONT_H */
