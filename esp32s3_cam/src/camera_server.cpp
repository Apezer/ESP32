#include "camera_server.h"

#include <Arduino.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include <img_converters.h>

namespace {

httpd_handle_t pageServer = nullptr;
httpd_handle_t streamServer = nullptr;

constexpr char kStreamType[] = "multipart/x-mixed-replace;boundary=frame";
constexpr char kStreamBoundary[] = "\r\n--frame\r\n";
constexpr char kStreamHeader[] =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

constexpr char kIndexHtml[] = R"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32-S3 OV5640</title>
  <style>
    :root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#101418;color:#eef2f5;font-family:system-ui,sans-serif}
    main{max-width:1100px;margin:auto;padding:18px}.card{background:#1b2229;border:1px solid #313c46;border-radius:14px;padding:14px}
    h1{font-size:1.35rem;margin:0 0 12px}.viewer{display:flex;justify-content:center;background:#050607;border-radius:10px;overflow:hidden}
    img{display:block;max-width:100%;height:auto}.controls{display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin-top:12px}
    button,select,input{font:inherit;color:inherit;background:#26313a;border:1px solid #44525e;border-radius:8px;padding:8px 10px}
    button{cursor:pointer;background:#136fca;border-color:#2387e8}.muted{color:#9eabb5;font-size:.9rem}label{display:flex;gap:7px;align-items:center}
  </style>
</head>
<body><main><section class="card">
  <h1>OV5640 实时画面</h1>
  <div class="viewer"><img id="stream" alt="Camera stream"></div>
  <div class="controls">
    <button id="toggle">开始画面</button>
    <button onclick="window.open('/capture','_blank')">拍照</button>
    <label>分辨率 <select id="size">
      <option value="8">VGA 640x480</option><option value="9">SVGA 800x600</option>
      <option value="10">XGA 1024x768</option><option value="11">HD 1280x720</option>
      <option value="12">SXGA 1280x1024</option><option value="13">UXGA 1600x1200</option>
      <option value="21">QSXGA 2560x1920</option>
    </select></label>
    <label>JPEG质量 <input id="quality" type="range" min="6" max="40" value="12"></label>
    <label><input id="vflip" type="checkbox">上下翻转</label>
    <label><input id="hmirror" type="checkbox">左右镜像</label>
  </div>
  <p id="status" class="muted">正在读取摄像头状态…</p>
</section></main>
<script>
const img=document.querySelector('#stream'),toggle=document.querySelector('#toggle');
let running=false;
toggle.onclick=()=>{running=!running;img.src=running?`http://${location.hostname}:81/stream`:'';toggle.textContent=running?'停止画面':'开始画面'};
async function control(variable,value){await fetch(`/control?var=${variable}&val=${value}`);}
document.querySelector('#size').onchange=e=>control('framesize',e.target.value);
document.querySelector('#quality').onchange=e=>control('quality',e.target.value);
document.querySelector('#vflip').onchange=e=>control('vflip',e.target.checked?1:0);
document.querySelector('#hmirror').onchange=e=>control('hmirror',e.target.checked?1:0);
fetch('/status').then(r=>r.json()).then(s=>{
 document.querySelector('#status').textContent=`传感器: ${s.sensor} | PSRAM: ${s.psram_mb} MB | 空闲堆: ${s.heap_kb} KB`;
 document.querySelector('#size').value=s.framesize;document.querySelector('#quality').value=s.quality;
}).catch(()=>document.querySelector('#status').textContent='状态读取失败');
toggle.click();
</script></body></html>
)HTML";

esp_err_t indexHandler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, kIndexHtml, HTTPD_RESP_USE_STRLEN);
}

esp_err_t statusHandler(httpd_req_t *req) {
  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor) {
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "camera unavailable");
  }

  const char *name = sensor->id.PID == OV5640_PID ? "OV5640" : "unknown";
  char json[192];
  snprintf(json, sizeof(json),
           "{\"sensor\":\"%s\",\"pid\":%u,\"framesize\":%d,\"quality\":%d,"
           "\"psram_mb\":%.1f,\"heap_kb\":%u}",
           name, sensor->id.PID, sensor->status.framesize, sensor->status.quality,
           ESP.getPsramSize() / 1048576.0, ESP.getFreeHeap() / 1024U);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_sendstr(req, json);
}

esp_err_t controlHandler(httpd_req_t *req) {
  char query[128];
  char variable[24];
  char valueText[16];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "var", variable, sizeof(variable)) != ESP_OK ||
      httpd_query_key_value(query, "val", valueText, sizeof(valueText)) != ESP_OK) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing var or val");
  }

  sensor_t *sensor = esp_camera_sensor_get();
  const int value = atoi(valueText);
  int result = -1;
  if (!strcmp(variable, "framesize")) {
    result = sensor->set_framesize(sensor, static_cast<framesize_t>(value));
  } else if (!strcmp(variable, "quality")) {
    result = sensor->set_quality(sensor, value);
  } else if (!strcmp(variable, "vflip")) {
    result = sensor->set_vflip(sensor, value);
  } else if (!strcmp(variable, "hmirror")) {
    result = sensor->set_hmirror(sensor, value);
  }

  if (result != 0) {
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "control failed");
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, nullptr, 0);
}

esp_err_t captureHandler(httpd_req_t *req) {
  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) {
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "capture failed");
  }

  uint8_t *jpeg = frame->buf;
  size_t jpegLength = frame->len;
  bool converted = false;
  if (frame->format != PIXFORMAT_JPEG) {
    converted = frame2jpg(frame, 80, &jpeg, &jpegLength);
  }

  esp_err_t result = ESP_FAIL;
  if (frame->format == PIXFORMAT_JPEG || converted) {
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=ov5640.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    result = httpd_resp_send(req, reinterpret_cast<const char *>(jpeg), jpegLength);
  }

  if (converted) free(jpeg);
  esp_camera_fb_return(frame);
  return result;
}

esp_err_t streamHandler(httpd_req_t *req) {
  esp_err_t result = httpd_resp_set_type(req, kStreamType);
  if (result != ESP_OK) return result;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  char header[96];
  while (true) {
    camera_fb_t *frame = esp_camera_fb_get();
    if (!frame) {
      result = ESP_FAIL;
      break;
    }

    uint8_t *jpeg = frame->buf;
    size_t jpegLength = frame->len;
    bool converted = false;
    if (frame->format != PIXFORMAT_JPEG) {
      converted = frame2jpg(frame, 80, &jpeg, &jpegLength);
      if (!converted) {
        esp_camera_fb_return(frame);
        result = ESP_FAIL;
        break;
      }
    }

    const size_t headerLength = snprintf(header, sizeof(header), kStreamHeader,
                                         static_cast<unsigned>(jpegLength));
    result = httpd_resp_send_chunk(req, kStreamBoundary, strlen(kStreamBoundary));
    if (result == ESP_OK) result = httpd_resp_send_chunk(req, header, headerLength);
    if (result == ESP_OK) {
      result = httpd_resp_send_chunk(req, reinterpret_cast<const char *>(jpeg), jpegLength);
    }

    if (converted) free(jpeg);
    esp_camera_fb_return(frame);
    if (result != ESP_OK) break;
  }
  return result;
}

}  // namespace

bool startCameraWebServer() {
  httpd_config_t pageConfig = HTTPD_DEFAULT_CONFIG();
  pageConfig.server_port = 80;
  pageConfig.max_uri_handlers = 8;
  pageConfig.lru_purge_enable = true;
  if (httpd_start(&pageServer, &pageConfig) != ESP_OK) return false;

  httpd_uri_t indexUri = {.uri = "/", .method = HTTP_GET, .handler = indexHandler, .user_ctx = nullptr};
  httpd_uri_t statusUri = {.uri = "/status", .method = HTTP_GET, .handler = statusHandler, .user_ctx = nullptr};
  httpd_uri_t controlUri = {.uri = "/control", .method = HTTP_GET, .handler = controlHandler, .user_ctx = nullptr};
  httpd_uri_t captureUri = {.uri = "/capture", .method = HTTP_GET, .handler = captureHandler, .user_ctx = nullptr};
  httpd_register_uri_handler(pageServer, &indexUri);
  httpd_register_uri_handler(pageServer, &statusUri);
  httpd_register_uri_handler(pageServer, &controlUri);
  httpd_register_uri_handler(pageServer, &captureUri);

  httpd_config_t streamConfig = HTTPD_DEFAULT_CONFIG();
  streamConfig.server_port = 81;
  streamConfig.ctrl_port = pageConfig.ctrl_port + 1;
  streamConfig.lru_purge_enable = true;
  if (httpd_start(&streamServer, &streamConfig) != ESP_OK) {
    httpd_stop(pageServer);
    pageServer = nullptr;
    return false;
  }

  httpd_uri_t streamUri = {.uri = "/stream", .method = HTTP_GET, .handler = streamHandler, .user_ctx = nullptr};
  return httpd_register_uri_handler(streamServer, &streamUri) == ESP_OK;
}
