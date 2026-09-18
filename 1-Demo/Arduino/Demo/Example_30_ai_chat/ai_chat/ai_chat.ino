#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"
#include <mbedtls/md.h>
#include <base64.h>
#include "Base64_Arturo.h"
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "FS.h"
#include "SD_MMC.h"
#include "SPI.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_check.h"
#include "es8311.h"
#include "Audio.h"
#include "demo_music.h"
#include "ESP_Panel_Board_Custom.h"
#include <UrlEncode.h>
#include <TFT_eSPI.h> 


TFT_eSPI my_lcd = TFT_eSPI(); 

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define EXAMPLE_MCLK_MULTIPLE I2S_MCLK_MULTIPLE_384
#define BUTTON_PIN 0  // 假设按键连接到GPIO 0
#define RECORD_TIME_MS 3000  // 录制时间为1.5毫秒

const char *ssid = "xxx";         //输入你的网络名称
const char *password = "xxx";     //输入你的网络密码

//豆包 OpenAI API key
const char* doubao_apiKey = "xxx";//输入你获取的API_KEY

// Send request to OpenAI API
String inputText = "你好,语音小助手!";//唤醒词
String apiUrl = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";

// 百度Token
String Token = "xxx";             //输入你获取的access_token

static const char *TAG = "i2s_es8311";
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
Audio audio;
HTTPClient http_client;
bool isRecording = false;
bool isPlaying = false;
bool has_recorded = false;
uint8_t *audio_buffer;
size_t buffer_size;
size_t bytes_recorded = 0;

/*
void audioTask(void *parameter)
{
  while (true) 
  {
    audio.loop();
    vTaskDelay(1);
  }
}

void audioInit() 
{
  xTaskCreatePinnedToCore(
    //audioTask,
    "audioplay",
    5000,
    NULL,
    2 | portPRIVILEGE_BIT,
    NULL,
    0
  );
}
*/
esp_err_t I2C_init(void)
{
  const i2c_config_t es_i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = { 
          .clk_speed = I2C_SPEED,
        },
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM, &es_i2c_cfg), TAG, "config i2c failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER,  0, 0, 0), TAG, "install i2c driver failed");
    return ESP_OK;
}

esp_err_t i2s_driver_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "I2S channel created successfully");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = GPIO_NUM_4 ,
            .bclk = GPIO_NUM_5 ,
            .ws = GPIO_NUM_7 ,
            .dout = GPIO_NUM_8 ,
            .din = GPIO_NUM_6 ,
            .invert_flags = {
             .mclk_inv = false,
             .bclk_inv = false,
             .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;

    err = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init TX channel in std mode: %s", esp_err_to_name(err));
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        return err;
    }
    err = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init RX channel in std mode: %s", esp_err_to_name(err));
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        return err;
    }
    err = i2s_channel_enable(tx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable TX channel: %s", esp_err_to_name(err));
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        return err;
    }
    err = i2s_channel_enable(rx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RX channel: %s", esp_err_to_name(err));
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        return err;
    }
    return ESP_OK;
}

void record_audio() {
    isRecording = true;
    bytes_recorded = 0;
    buffer_size = EXAMPLE_SAMPLE_RATE * 2 * (RECORD_TIME_MS / 1000); // 2 bytes per sample (16-bit)
    audio_buffer = (uint8_t *)malloc(buffer_size);
    if (!audio_buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for audio buffer");
        isRecording = false;
        return;
    }
    Serial.println("开始录音......");
    uint32_t start_time = millis();
    while (millis() - start_time < RECORD_TIME_MS/2) {
        size_t bytes_read = 0;
        esp_err_t ret = i2s_channel_read(rx_handle, audio_buffer + bytes_recorded, buffer_size - bytes_recorded, &bytes_read, 2000);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "i2s read failed, %s", esp_err_to_name(ret));
            break;
        }
        bytes_recorded += bytes_read;
    }
    Serial.println("结束录音");
    isRecording = false;
    has_recorded = true;
    ESP_LOGI(TAG, "Recording finished, %d bytes recorded", bytes_recorded);
}

void play_audio() {
    Serial.print("音频字节长度：");
    Serial.println(bytes_recorded);
    if (!has_recorded) {
        ESP_LOGW(TAG, "No audio recorded yet");
        return;
    }
    isPlaying = true;
    size_t bytes_written = 0;
    esp_err_t ret = i2s_channel_write(tx_handle, audio_buffer, bytes_recorded, &bytes_written, 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s write failed, %s", esp_err_to_name(ret));
    }
    isPlaying = false;
    ESP_LOGI(TAG, "Playback finished, %d bytes written", bytes_written);
}

String STT()
{
      String base64audioString = base64::encode(audio_buffer,bytes_recorded);
      String data_json = "{";
      data_json += "\"format\":\"pcm\",";
      data_json += "\"rate\":16000,";
      data_json += "\"dev_pid\":1537,";
      data_json += "\"channel\":1,";
      data_json += "\"token\":\"" + Token + "\",";
      data_json += "\"cuid\":\"66666\",";
      data_json += "\"len\":48000,";
      data_json += "\"speech\":\"";
      data_json += base64audioString;  // 直接拼接String对象
      data_json += "\"}";
      int httpCode;
      http_client.setTimeout(5000);
      http_client.begin("http://vop.baidu.com/server_api");  //https://vop.baidu.com/pro_api
      http_client.addHeader("Content-Type", "application/json");
      httpCode = http_client.POST(data_json);

      if (httpCode == 200) {
        if (httpCode == HTTP_CODE_OK) {
          String response = http_client.getString();
          http_client.end();
          Serial.println(response);
          // Parse JSON response
          DynamicJsonDocument jsonDoc(512);
          deserializeJson(jsonDoc, response);
          String outputText = jsonDoc["result"][0];
          // 访问"result"数组，并获取其第一个元
          // 输出结果
          Serial.println(outputText);
          return outputText;
        } else {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http_client.errorToString(httpCode).c_str());
        }
      }
}


String getGPTAnswer(String inputText) {
  HTTPClient http;
  http.setTimeout(20000);
  http.begin(apiUrl);
  http.addHeader("Content-Type", "application/json");
  String token_key = String("Bearer ") + doubao_apiKey;
  http.addHeader("Authorization", token_key);
  String payload = "{\"model\":\"ep-20250210093736-fpmjk\",\"messages\":[{\"role\":\"system\",\"content\":\"你是我的AI助手,叫语音小助手,你必须用中文回答且字数不超过60个\"},{\"role\":\"user\",\"content\":\"" + inputText + "\"}],\"temperature\": 0.3}";

  int httpResponseCode = http.POST(payload);
  if (httpResponseCode == 200) {
    String response = http.getString();
    http.end();
    Serial.println(response);

    // Parse JSON response
    DynamicJsonDocument jsonDoc(1024);
    deserializeJson(jsonDoc, response);
    String outputText = jsonDoc["choices"][0]["message"]["content"];
    return outputText;
    // Serial.println(outputText);
  } else {
    http.end();
    Serial.printf("Error %i \n", httpResponseCode);
    return "<error>";
  }
}

const int PER = 4;
const int SPD = 5;
const int PIT = 5;
const int VOL = 4;
const int AUE = 3;

String TTS_URL = "http://tsn.baidu.com/text2audio";
String url;


void tts_get(String answer) {
  const char *headerKeys[] = { "Content-Type", "Content-Length" };
  url = TTS_URL;
  // 修改百度语音助手的token
  url += "?tok="+Token;
  url += "&tex=" + answer;     //播放文本
  url += "&per=" + String(PER);
  url += "&spd=" + String(SPD);
  url += "&pit=" + String(PIT);
  url += "&vol=" + String(VOL);
  url += "&aue=" + String(AUE);
  url += "&cuid=nzizgzdNcNsc9oET12irmlGK1P";
  url += "&lan=zh";
  url += "&ctp=1";

  HTTPClient http;

  Serial.print("URL: ");
  Serial.println(url);

  http.begin(url);
  http.collectHeaders(headerKeys, 2);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    if (httpResponseCode == HTTP_CODE_OK) {
      Serial.print("Content-Type = ");
      Serial.println(http.header("Content-Type"));
      String contentType = http.header("Content-Type");
      if (contentType.startsWith("audio")) {
        Serial.println("合成成功，返回的是音频文件");
      } else if (contentType.equals("application/json")) {
        Serial.println("合成出现错误，返回的是JSON文本");
      } else {
        Serial.println("未知的Content-Type");
      }
    } else {
      Serial.println("Failed to receive audio file");
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();

}

void setup()
{
    Serial.begin(115200);
    I2C_init();
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
    pinMode(AP_ENABLE, OUTPUT);
    digitalWrite(AP_ENABLE, LOW);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
      uint8_t count = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    count++;
    if (count >= 75) {
      Serial.printf("\r\n-- wifi connect fail! --");
      break;
    }
    vTaskDelay(200);
  }
  Serial.printf("\r\n-- wifi connect success! --\r\n");

    //i2s init
    if (i2s_driver_init() != ESP_OK) 
    {
        Serial.println("i2s init failed!");
        return;
    }
   
    //es8311 init
    if (es8311_codec_init() != ESP_OK) 
    {
        Serial.println("ES8311 init failed!");
        return;
    }
    String answer;
    String outputtext=STT();
    Serial.print("语音识别：");
    Serial.println(outputtext);
    answer = getGPTAnswer(inputText);
    Serial.println("Answer: " + answer);
    Serial.println("Enter a prompt:");
    answer = urlEncode(urlEncode(answer));
    tts_get(answer);
    player();
    vTaskDelay(1000);
    //audioInit();
    Serial.println("按住背后的按键开始对话");
}
bool flag = LOW;
void loop()
{    
    static bool last_button_state = HIGH;
    bool current_button_state = digitalRead(BUTTON_PIN);
    if (last_button_state == HIGH && current_button_state == LOW) {
            record_audio();
            String answer;
            String outputtext=STT();
            Serial.print("语音识别：");
            Serial.println(outputtext);
            answer = getGPTAnswer(outputtext);
            Serial.println("Answer: " + answer);
            Serial.println("Enter a prompt:");
            answer = urlEncode(urlEncode(answer));
            tts_get(answer);
            player();
            vTaskDelay(1000);
            flag = HIGH;
    }
    last_button_state = current_button_state;

    if(current_button_state==last_button_state && flag == HIGH)
    {
      vTaskDelay(200);
      audio.loop();
    }

    vTaskDelay(1);
}
void player() {
  const char *host = url.c_str();
  audio.setPinout(I2S_BCK, I2S_WS, I2S_DOUT,I2S_MCK);
  audio.setVolume(12);        // 0...21
  audio.connecttohost(host);  //  128k mp3
  //Serial.println(host);
}