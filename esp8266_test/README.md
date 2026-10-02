# esp8266_test — ESP8266 网页控制板载 LED

## 工程用途

NodeMCU 创建 ESP8266_LED 热点（密码 12345678），通过 http://192.168.4.1/ 网页切换板载 LED，GPIO2 低电平点亮。使用 ESP8266WebServer 的 HTTP 服务。

## 开发环境与使用

- 板型：nodemcuv2
- 在本目录执行 `pio run` 编译、`pio run -t upload` 烧录、`pio device monitor` 查看串口。依赖与板型以 platformio.ini 为准。
- 本次整理依据源码与配置，未执行固件编译或硬件测试。不同工程的引脚配置不通用，连接前核对各自源码。
