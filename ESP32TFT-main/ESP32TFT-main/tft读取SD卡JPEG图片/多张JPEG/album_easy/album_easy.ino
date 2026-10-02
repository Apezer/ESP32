// 【学习ESP32请关注】
// - B站/视频号：伊娃老师来了
// - 公众号：伊娃来了

//1.加载需要的库
#include <TJpg_Decoder.h>  //JPEG解码，JPEG文件必须是24位格式（不支持8位）
#include <SD.h>            //读取sd卡
#include <SPI.h>           //SPI通信
#include <TFT_eSPI.h>      // TFT显示

//2.定义常量和变量
#define SD_CS 5  //定义SD卡的CS引脚为5
//String name = "/1.jpg";     //在这里可以修改图片名称
String name[100] = { "/test/1.jpeg", "/test/2.jpeg", "/test/3.jpeg", "/test/4.jpeg", "/test/5.jpeg", "/test/6.jpeg" };
TFT_eSPI tft = TFT_eSPI();  // 调用自定义库

//3.渲染函数
//如果JPEG图像在屏幕上的顶部坐标 y 超过了屏幕的高度，
//就停止继续解码。这是因为如果图像的顶部已经超出屏幕底部，
//那么在继续解码下一部分图像可能就不再需要了，因为它已经不在屏幕可见的范围内。
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

//4.初始化函数
void setup() {
  Serial.begin(115200);  //串口监视器使用115200频率

  // 初始化SD卡
  SPI.begin();  //使用SPI通信
  if (!SD.begin(SD_CS)) {
    Serial.println("SD初始化失败");
    return;
  }
  Serial.println("SD初始化成功");

  // 初始化TFT
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  // 设置当JPEG解码时，调用tft_output函数
  TJpgDec.setCallback(tft_output);
}

void loop() {

  //自定义函数，根据图片高度缩放图片比例
  for (int i = 0; i < 6; i++) {
    scalePic(name[i]);
  }
}

void scalePic(String name) {
  // 获取jpeg图像的宽度和高度（以像素为单位）
  uint16_t w = 0, h = 0;  //uint16_t 表示无符号16位整数，可以节省一位的存储空间
  TJpgDec.getSdJpgSize(&w, &h, name);
  Serial.print("宽度 = ");
  Serial.print(w);
  Serial.print(", 高度 = ");
  Serial.println(h);
  // 根据图像和屏幕的高度比例设置图像缩放级别（1：1倍，2：1/2倍，4：1/4倍，8：1/8倍）
  float scale = float(h) / float(tft.height());
  if (scale > 4) {
    TJpgDec.setJpgScale(8);
  } else if (scale > 2) {
    TJpgDec.setJpgScale(4);
  } else if (scale > 1) {
    TJpgDec.setJpgScale(2);
  } else {
    TJpgDec.setJpgScale(1);
  }
  // 在屏幕上绘制图像，左上角坐标为0,0
  TJpgDec.drawSdJpg(0, 0, name);

  // 等待5秒
  delay(5000);
}
