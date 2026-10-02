// ============================================================
//  GOOUUU ESP32-S3-CAM V1.5 — прошивка
//  WiFi + MJPEG-веб-сервер
//
//  Arduino IDE: плата "ESP32S3 Dev Module", PSRAM = Enabled (Octal)
//  PlatformIO:  board = esp32-s3-devkitc-1, PSRAM = opi
// ============================================================

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// === НАСТРОЙКА WIFI ===
const char* ssid     = "";
const char* password = "";
const char* apSsid   = "ESP32S3-CAM";
const char* apPassword = "camviewer";

WebServer server(80);

// === Пины камеры (GOOUUU ESP32-S3-CAM V1.5) ===
#define PWDN_GPIO_NUM   -1
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   15
#define SIOD_GPIO_NUM    4
#define SIOC_GPIO_NUM    5
#define Y9_GPIO_NUM     16
#define Y8_GPIO_NUM     17
#define Y7_GPIO_NUM     18
#define Y6_GPIO_NUM     12
#define Y5_GPIO_NUM     10
#define Y4_GPIO_NUM      8
#define Y3_GPIO_NUM      9
#define Y2_GPIO_NUM     11
#define VSYNC_GPIO_NUM   6
#define HREF_GPIO_NUM    7
#define PCLK_GPIO_NUM   13

// === HTML-страница ===
#define BOUNDARY "ESP32S3CAMBOUNDARY"
static const char* CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" BOUNDARY;

const char INDEX_HTML[] PROGMEM = R"===(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>GOOUUU S3-CAM</title>
<style>
  body{font-family:sans-serif;text-align:center;background:#1a1a2e;color:#eee;margin:0;padding:20px}
  img{width:90%;max-width:800px;border:2px solid #444;border-radius:8px;margin-top:12px}
  a{color:#6cf}
</style>
</head>
<body>
  <h1>GOOUUU ESP32-S3-CAM V1.5</h1>
  <img src="/stream" alt="stream">
  <p><a href="/capture">Снимок</a></p>
</body>
</html>
)===";

// === Инициализация камеры ===
bool initCamera() {
  camera_config_t config;
  memset(&config, 0, sizeof(config));

  config.pin_pwdn      = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;
  config.pin_pclk  = PCLK_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format  = PIXFORMAT_JPEG;
  config.grab_mode     = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);
    s->set_saturation(s, 0);
    s->set_contrast(s, 0);
  }
  return true;
}

// === HTTP-обработчики ===
void handleIndex() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleCapture() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }
  server.setContentLength(fb->len);
  server.send(200, "image/jpeg", "");
  server.sendContent((const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleStream() {
  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  client.println(CONTENT_TYPE);
  client.println("Connection: close");
  client.println();

  camera_fb_t* warmup = esp_camera_fb_get();
  if (warmup) esp_camera_fb_return(warmup);

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) continue;

    client.print("--");
    client.println(BOUNDARY);
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println();

    size_t sent = client.write((const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);

    if (sent == 0) break;
    client.println();
    delay(60);
  }
}

// === setup / loop ===
void setup() {
  Serial.begin(115200);
  unsigned long serialWaitStartedAt = millis();
  while (!Serial && millis() - serialWaitStartedAt < 15000) delay(100);
  Serial.println("\n--- GOOUUU ESP32-S3-CAM V1.5 ---");

  if (!initCamera()) {
    Serial.println("Camera failed; check the pin mapping and camera connection.");
    while (true) delay(1000);
  }

  bool connectedToWifi = false;
  if (ssid[0] != '\0') {
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 15000) {
      delay(400);
      Serial.print(".");
    }
    Serial.println();
    connectedToWifi = WiFi.status() == WL_CONNECTED;
  }

  if (connectedToWifi) {
    Serial.print("WiFi IP: ");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(apSsid, apPassword)) {
      Serial.print("Access point: ");
      Serial.println(apSsid);
      Serial.print("Password: ");
      Serial.println(apPassword);
      Serial.print("Camera IP: ");
      Serial.println(WiFi.softAPIP());
    } else {
      Serial.println("Failed to start WiFi access point.");
    }
  }

  server.on("/",        HTTP_GET, handleIndex);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/stream",  HTTP_GET, handleStream);
  server.begin();
  Serial.println("Server started.");
}

void loop() {
  server.handleClient();
  delay(1);
}
