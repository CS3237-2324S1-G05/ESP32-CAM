#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#define CAMERA_MODEL_AI_THINKER 
#include "camera_pins.h"

const char* ssid = "3237GRP5";//redian//Linksys14312
const char* password = "3237password";//venuspassword//genm56sadf
/// MQTT Broker settings
const char* mqtt_broker = "192.168.43.156";  // IP of the MQTT broker (the machine running the Python script)
// MQTT Topic
const char* mqtt_topic = "camera/take_photo";  // This should match the topic in the Python script
 // Topic name must match with what the Python script expects
const char* mqtt_username = ""; // if your MQTT broker has a username, replace this
const char* mqtt_password = ""; // if your MQTT broker has a password, replace this
const char* clientID = "ESP32_Camera_Client";

WiFiClient espClient;
PubSubClient client(espClient);


void startCameraServer();
void setupLedFlash(int pin);
void reconnect();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("START OF SETUP");

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
  config.jpeg_quality = 4;
  config.fb_count = 1;
  Serial.println("CONFIG DONE");
  
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

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
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

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif
Serial.println("SETTING UP WIFI");

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  client.setServer(mqtt_broker, 1883);
  client.setCallback(callback);

  startCameraServer();
  Serial.println("STARTED CAMERA SERVER");

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
} 
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }

  // Compare the payload to the command
  if (String(topic) == mqtt_topic) {
    if(messageTemp == "TAKE_PHOTO") {
      takePhoto();
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_topic, "ESP32 Camera is now connected");
      // ... and resubscribe
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void takePhoto() {
    camera_fb_t * fb = esp_camera_fb_get();
    if(!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    client.publish("camera/photo", "Photo captured successfully");

    // Print the size of the captured image
    Serial.printf("Captured image with size: %d bytes\n", fb->len);

    // Send the image to the server
    HTTPClient http;
    http.begin("http://192.168.43.156:3237/backend/carplate-recognition/entrance");
    http.addHeader("Content-Type", "image/jpeg");
    
    int httpResponseCode = http.POST(fb->buf, fb->len);
    
    if(httpResponseCode > 0){
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
    } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
    }

    http.end();

    // Return the frame buffer back to the camera driver
    esp_camera_fb_return(fb);

    delay(10000); // Wait for 10 seconds before capturing the next image
}

