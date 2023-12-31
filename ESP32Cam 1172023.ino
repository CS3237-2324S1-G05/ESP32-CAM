#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#define CAMERA_MODEL_AI_THINKER 
#include "camera_pins.h"
#include <Arduino.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_camera.h>
#include <HTTPClient.h>

const char* ssid = "redian";//redian//Linksys14312//3237GRP5
const char* password = "venuspassword";//venuspassword//genm56sadf//3237password
const char* host = "192.168.86.80"; // Replace with your server IP
const int port = 7000; // Replace with your server port

// Replace with your MQTT broker's details
const char* mqtt_server = "192.168.86.80";
const int mqtt_port = 1883;

const char* CAM_CARPARK_TOPIC = "cam/carpark";
const char* CAM_LOT_TOPIC = "cam/lot/";
const char* CAM_GANTRY_ENTRANCE_TOPIC = "cam/gantry/entrance";
const char* CAM_GANTRY_EXIT_TOPIC = "cam/gantry/exit";

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* message, unsigned int length) {
  // Handle message arrived on subscribed topics
  String messageTemp;

  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  Serial.println(messageTemp);

  if (String(topic) == CAM_CARPARK_TOPIC) {
    Serial.println("Capturing and sending image for carpark...");
    captureAndSendImage();
  } else if (String(topic).startsWith(CAM_LOT_TOPIC)) {
    Serial.println("Capturing and sending image for lot...");
    captureAndSendImage();
  } else if (String(topic) == CAM_GANTRY_ENTRANCE_TOPIC) {
    Serial.println("Capturing and sending image for gantry entrance...");
    captureAndSendImage();
  } else if (String(topic) == CAM_GANTRY_EXIT_TOPIC) {
    Serial.println("Capturing and sending image for gantry exit...");
    captureAndSendImage();
  } else {
    Serial.println("Received message on unknown topic.");
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");

      // Subscribe to topics and report on each subscription
      if(client.subscribe(CAM_CARPARK_TOPIC)){
        Serial.println("Subscribed to CAM_CARPARK_TOPIC");
      } else {
        Serial.println("Subscription to CAM_CARPARK_TOPIC failed");
      }

      if(client.subscribe(CAM_LOT_TOPIC)){
        Serial.println("Subscribed to CAM_LOT_TOPIC");
      } else {
        Serial.println("Subscription to CAM_LOT_TOPIC failed");
      }

      if(client.subscribe(CAM_GANTRY_ENTRANCE_TOPIC)){
        Serial.println("Subscribed to CAM_GANTRY_ENTRANCE_TOPIC");
      } else {
        Serial.println("Subscription to CAM_GANTRY_ENTRANCE_TOPIC failed");
      }

      if(client.subscribe(CAM_GANTRY_EXIT_TOPIC)){
        Serial.println("Subscribed to CAM_GANTRY_EXIT_TOPIC");
      } else {
        Serial.println("Subscription to CAM_GANTRY_EXIT_TOPIC failed");
      }

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 10 seconds");
      // Wait 10 seconds before retrying
      delay(10000);
    }
  }
}

void startCameraServer();
void setupLedFlash(int pin);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Starting up...");;

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  } else {
    Serial.println("Camera initialized successfully.");
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif
// Print the camera settings after initialization
Serial.print("Camera settings after initialization: ");
Serial.print("Vflip: "); Serial.println(s->status.vflip);
Serial.print("Brightness: "); Serial.println(s->status.brightness);
Serial.print("Saturation: "); Serial.println(s->status.saturation);
// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid, password);
  Serial.println("Attempting to connect to WiFi...");
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());

  startCameraServer();

  Serial.println("Camera server started.");
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
}

void captureAndSendImage() {
  Serial.println("Function captureAndSendImage called...");

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.println("Image captured");

  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("Connection to server failed");
    return;
  }
  Serial.println("Connected to server");

  String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
  String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"image.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  client.println("POST /backend/human-recognition HTTP/1.1");
  client.println("Host: " + String(host) + ":" + String(port));
  client.println("Content-Length: " + String(fb->len + head.length() + tail.length()));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println();
  client.print(head);

  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  for (size_t n = 0; n < fbLen; n = n + 1024) {
    if (n + 1024 < fbLen) {
      client.write(fbBuf, 1024);
      fbBuf += 1024;
    } else if (fbLen % 1024 > 0) {
      size_t remainder = fbLen % 1024;
      client.write(fbBuf, remainder);
    }
  }

  client.print(tail);

  int timeout = 10000; // 10 seconds timeout for reading the data
  long int startTime = millis();

  while (client.connected() && millis() - startTime < timeout) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("HTTP/1.1")) {
    Serial.println(line);
  }
  
  // Close the connection
  client.stop();
  Serial.println("Connection closed");

  esp_camera_fb_return(fb);
  Serial.println("Frame buffer returned for reuse...");
}

void loop() {
  static unsigned long lastTime = 0; // will store last time an image was sent
  unsigned long currentTime = millis();

  // Check if 10 seconds have passed
  if (currentTime - lastTime >= 10000) { // 10000ms (10 seconds) has passed
    lastTime = currentTime; // save the last time an image was sent
    Serial.println("Ten seconds passed, sending image...");
    captureAndSendImage(); // capture and send the image
  }
    delay(10);
}
