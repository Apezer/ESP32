#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// AP 热点配置
const char *ap_ssid = "ESP8266_LED";
const char *ap_password = "12345678";  // 至少8位，改为你想要的密码

// LED 引脚（NodeMCU 板载 LED 在 GPIO2，低电平点亮）
#define LED_PIN LED_BUILTIN

ESP8266WebServer server(80);
bool ledState = false;

// ============ 网页 HTML ============
String getHTML() {
  String color = ledState ? "#4CAF50" : "#f44336";
  String stateText = ledState ? "ON" : "OFF";

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP8266 LED Control</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, sans-serif;
      display: flex; justify-content: center; align-items: center;
      min-height: 100vh;
      background: linear-gradient(135deg, #e0f0ff 0%, #b3d9ff 100%);
      color: #333;
    }
    .card {
      background: rgba(255,255,255,0.85);
      backdrop-filter: blur(10px);
      border-radius: 20px; padding: 40px;
      text-align: center; width: 300px;
      box-shadow: 0 8px 32px rgba(0,80,160,0.15);
      border: 1px solid rgba(255,255,255,0.6);
    }
    h1 { font-size: 1.5em; margin-bottom: 10px; color: #1a73e8; }
    .status { font-size: 3em; font-weight: bold; margin: 20px 0; color: #1a73e8; }
    .dot {
      width: 80px; height: 80px; margin: 20px auto;
      border-radius: 50%;
      background: )rawliteral" + color + R"rawliteral(;
      box-shadow: 0 0 )rawliteral" + String(ledState ? "30px" : "0px") + R"rawliteral( )rawliteral" + color + R"rawliteral(;
      transition: all 0.3s;
    }
    .btn {
      display: inline-block; margin-top: 20px;
      padding: 14px 50px; font-size: 1.2em;
      border: none; border-radius: 30px;
      background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
      color: #fff; cursor: pointer; font-weight: bold;
      box-shadow: 0 4px 15px rgba(79,172,254,0.4);
      transition: transform 0.1s;
    }
    .btn:active { transform: scale(0.95); }
  </style>
</head>
<body>
  <div class="card">
    <h1>&#128161; LED Control</h1>
    <div class="dot"></div>
    <div class="status">)rawliteral" + stateText + R"rawliteral(</div>
    <form action="/toggle" method="GET">
      <button class="btn" type="submit">Toggle</button>
    </form>
  </div>
</body>
</html>
)rawliteral";
  return html;
}

// ============ 路由处理 ============
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void handleToggle() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? LOW : HIGH);  // 板载LED低电平点亮
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(303);
}

// ============ 主程序 ============
void setup() {
  Serial.begin(115200);
  Serial.println();

  // 初始化 LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // 默认灭

  // 创建 WiFi 热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("================================");
  Serial.println("  WiFi Hotspot Started!");
  Serial.print("  SSID: "); Serial.println(ap_ssid);
  Serial.print("  Password: "); Serial.println(ap_password);
  Serial.print("  IP: "); Serial.println(WiFi.softAPIP());
  Serial.println("================================");
  Serial.println("  Connect and visit: http://192.168.4.1");
  Serial.println("================================");

  // 设置路由
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("  Web server started!");
}

void loop() {
  server.handleClient();
}
