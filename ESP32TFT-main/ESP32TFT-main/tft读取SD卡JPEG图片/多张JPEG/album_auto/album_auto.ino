// 【学习ESP32请关注】
// - B站/视频号：伊娃老师来了
// - 公众号：伊娃来了

//1.引用相关库
#include <SD.h>   //读取SD卡
#include <SPI.h>  //SPI通信

#include <TFT_eSPI.h>  //TFT显示

#include <TJpg_Decoder.h>  //JPEG解码，JPEG文件必须是24位格式（不支持8位）

//2.定义常量与变量
#define SD_CS 5            //使用 #define 定义引脚没有额外的内存使用
const int maxFiles = 100;  //定义常量maxFiles为100，表示最大读取100个jpg文件

TFT_eSPI tft = TFT_eSPI();  //调用TFT_eSPI库使用
String jpgFiles[maxFiles];  //定义字符串数组jpgFiles存放jpg文件名称

int fileCount = 0;    //定义遍历用的变量
String folder = "/test";  //定义文件夹的路径，/为根目录

//3.自定义函数tft_output：渲染图片
//如果JPEG图像在屏幕上的顶部坐标 y 超过了屏幕的高度，
//就停止继续解码。这是因为如果图像的顶部已经超出屏幕底部，
//那么在继续解码下一部分图像可能就不再需要了，因为它已经不在屏幕可见的范围内。
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

//4.自定义函数listJpgFiles：列出指定目录下的所有JPEG文件
void listJpgFiles(String dir) {
  File root = SD.open(dir);  //定义dir为开启指定目录的文件

  if (!root) {
    Serial.println("开启文件夹失败");
    return;
  }

  Serial.println("列出文件夹中的JPG文件：");

  //遍历所有文件直到没有更多文件
  while (true) {
    File file = root.openNextFile();  //定义file为开启下一个文件
    if (!file) {                      //使用 if (!file) 来检查是否还有更多文件，如果没有，跳出循环。
      break;
    }
    //使用!file.isDirectory()判断当前文件不是文件夹
    //使用isJpgFile(file.name())判断是jpg
    if (!file.isDirectory() && isJpgFile(file.name())) {
      Serial.print("文件: ");
      Serial.println(file.name());


      if (fileCount < maxFiles) {
        jpgFiles[fileCount] = file.name();
        fileCount++;
      } else {
        Serial.println("达到文件最大上限");
        break;
      }
    }

    file.close();  //使用 file.close() 关闭文件，释放资源
  }

  root.close();  //使用 root.close() 关闭根目录文件，释放资源
}
//5.自定义函数scalePic：在显示屏上绘制指定文件名的JPEG图片
void scalePic(String fileName) {
  tft.fillScreen(TFT_BLACK);  //设置底色为黑色

  //获取图片的宽跟高，并根据高度进行缩放
  uint16_t w = 0, h = 0;
  TJpgDec.getSdJpgSize(&w, &h, fileName);
  Serial.print("Width = ");
  Serial.print(w);
  Serial.print(", height = ");
  Serial.println(h);
  float a = static_cast<float>(h) / tft.height();
  if (a > 4) {
    TJpgDec.setJpgScale(8);
  } else if (a > 2) {
    TJpgDec.setJpgScale(4);
  } else if (a > 1) {
    TJpgDec.setJpgScale(2);
  } else {
    TJpgDec.setJpgScale(1);
  }
  Serial.println(a);

  //绘制图片
  TJpgDec.drawSdJpg(0, 0, fileName);
}

//6.自定义函数isJpgFile，判断文件的名称：
//（1）不是.开始：避免隐藏文件
//（2）是.jpg或.JPG或.jpeg或.JPEG结尾
bool isJpgFile(String name) {
  return !name.startsWith(".") && (name.endsWith(".jpg") || name.endsWith(".JPG") || name.endsWith(".jpeg") || name.endsWith(".JPEG"));
}
//初始化函数
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

  //列出指定目录下的所有文件
  listJpgFiles(folder);
}

//7.主循环函数
void loop() {
  //遍历所有文件，并用自定义函数drawPic显示出来，每五秒切换一次
  for (int i = 0; i < fileCount; i++) {
    if (folder == "/") {
      scalePic("/" + jpgFiles[i]);
    } else {
      scalePic(folder + "/" + jpgFiles[i]);
    }


    delay(5000);
  }
}
