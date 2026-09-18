//This program controls NeoPixel lights through touch screen buttons using capacitive touch

//pin usage as follow:
//            CS  DC/RS  RESET  SDI/MOSI  SCK  SDO/MISO  BL   CTP_INT  CTP_RST  CTP_SDA  CTP_SCL   VCC     GND    
//ESP32-S3:   10   46     -1      11      12      13     45     17       18       16       15       5V     GND  

/*********************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE 
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**********************************************************************************/
#include <Arduino.h>
#include "TFT_eSPI.h" 
#include "touch.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN    42 
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  60 
// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50 

// 当前颜色索引，这里使用数组来分别记录每个颜色的状态
int currentColorIndex[3] = {0, 0, 0};

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 定义颜色数组，按红绿蓝顺序
const uint32_t colors[] = {
  strip.Color(255, 0, 0), // 红色
  strip.Color(0, 255, 0), // 绿色
  strip.Color(0, 0, 255)  // 蓝色
};

//key numbers
#define NUM_KEYS 3

// Keypad start position, key sizes and spacing
#define KEY_X 75 // Centre of key
#define KEY_Y 65
#define KEY_W 65 // Width and height
#define KEY_H 35
#define KEY_SPACING_X 20 // X and Y gap
#define KEY_SPACING_Y 1
#define KEY_TEXTSIZE 1   // Font size multiplier

uint16_t k_color[NUM_KEYS] = {TFT_RED, TFT_GREEN, TFT_BLUE};

TFT_eSPI tft = TFT_eSPI();
TFT_eSPI_Button key_b[NUM_KEYS];

// 初始化 NeoPixel 灯珠
void initNeoPixel() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);
}

// 设置灯颜色
void setNeoPixelColor(uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

// 闪烁
void neoPixelFlick(uint8_t colorIndex) {
  for (int i = 0; i < 6; i++) {
    setNeoPixelColor(0);
    delay(500);
    setNeoPixelColor(colors[colorIndex]);
    delay(500);
  }
}

void setup()
{
    tft.begin();
    tft.setRotation(1);
    touch_init(tft.width(), tft.height(), tft.getRotation()); // 初始化电容触摸
    tft.fillScreen(TFT_BLACK);

    initNeoPixel();

    tft.setCursor(5, 25);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    //tft.println("Press the button to turn the NeoPixel light on or off");
    tft.setTextSize(2);
    tft.setCursor(tft.width()/2 - 150, 195);
    tft.println("0");
    tft.setCursor(tft.width()/2 + 110, 195);
    tft.println("255");
    tft.fillRoundRect(tft.width()/2 - 138, 165, 276, 21, 10, 0x8430);
    tft.fillCircle(tft.width()/2 - 128, 175, 10, TFT_MAGENTA);

    tft.setTextColor(0xFFE0);
    tft.drawNumber(0, tft.width()/2 - 15, 120, 1);
    tft.setFreeFont(&FreeSansBold9pt7b);
    //draw button
    for (int i = 0; i < NUM_KEYS; i++)
    {
        key_b[i].initButton(&tft,
                            KEY_X + i * (KEY_W + KEY_SPACING_X),
                            KEY_Y + 0 * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
                            KEY_W,
                            KEY_H,
                            TFT_WHITE, // Outline
                            k_color[i], // Fill
                            TFT_WHITE, // Text
                            "", // 10 Byte Label
                            KEY_TEXTSIZE);

        // Adjust button label X delta according to array position
        // setLabelDatum(uint16_t x_delta, uint16_t y_delta, uint8_t datum)
        key_b[i].setLabelDatum(13 - (KEY_W/2), 2, ML_DATUM);

        // Draw button and specify label string
        // Specifying label string here will allow more than the default 10 byte label
        key_b[i].drawButton(false, "OFF");
    }
}

void loop()
{
    uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
    uint8_t i = 0;

    if (touch_touched()) // 使用电容触摸检测函数
    {
        t_x = touch_last_x;
        t_y = touch_last_y;

        for (i = 0; i < NUM_KEYS; i++)
        {
            if (key_b[i].contains(t_x, t_y))
            {
                key_b[i].press(true);  // tell the button it is pressed
            }
            else
            {
                key_b[i].press(false);  // tell the button it is NOT pressed
            }
        }
    }
    else
    {
        for (i = 0; i < NUM_KEYS; i++)
        {
            key_b[i].press(false);  // tell the button it is NOT pressed
        }
    }

    // Check if any key has changed state
    for (i = 0; i < NUM_KEYS; i++)
    {
        // If button was just pressed, redraw inverted button
        if (key_b[i].justPressed())
        {
            if (currentColorIndex[i] == 0)
            {
                key_b[i].drawButton(true, "OFF");
            }
            else if (currentColorIndex[i] == 1)
            {
                key_b[i].drawButton(true, "ON");
            }
            else
            {
                key_b[i].drawButton(true, "FLI");
            }
        }
        // If button was just released, redraw normal color button
        if (key_b[i].justReleased())
        {
            if (currentColorIndex[i] == 0)
            {
                key_b[i].drawButton(false, "ON");
                setNeoPixelColor(colors[i]);
                currentColorIndex[i] = 1;
            }
            else if (currentColorIndex[i] == 1)
            {
                key_b[i].drawButton(false, "FLI");
                neoPixelFlick(i);
                currentColorIndex[i] = 2;
            }
            else
            {
                key_b[i].drawButton(false, "OFF");
                setNeoPixelColor(0);
                currentColorIndex[i] = 0;
            }
        }
    }

    if (touch_touched()) // 使用电容触摸检测函数
    {
        t_x = touch_last_x;
        t_y = touch_last_y;
        if ((t_x >= tft.width()/2 - 128) && (t_x < tft.width()/2 + 128) && t_y > 164 && t_y < 184)
        {
            // 亮度调节
            int brightness = t_x - (tft.width()/2 - 128);
            if (brightness < 0) brightness = 0;
            if (brightness > 255) brightness = 255;
            strip.setBrightness(brightness);
            strip.show();

            tft.setTextColor(0xFFE0, TFT_BLACK);
            tft.fillRoundRect(t_x - 10, 165, tft.width()/2 + 138 - t_x + 10, 21, 10, 0x8430);
            tft.fillRoundRect(tft.width()/2 - 138, 165, t_x - tft.width()/2 + 148, 21, 10, 0x07FF);
            tft.fillCircle(t_x, 175, 10, TFT_MAGENTA);
            tft.setTextPadding(50);
            tft.drawNumber(brightness, tft.width()/2 - 15, 120, 1);
        }
    }
}