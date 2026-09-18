//The function of this program is to use PWM to control the backlight brightness of the LCD screen
// with capacitive touch screen and a specific display interface

//pin usage as follow:
//                   CS  DC/RS  RESET    SDI/MOSI  SCK   SDO/MISO  BL   CTP_INT  CTP_RST  CTP_SDA  CTP_SCL   VCC    GND    
//ESP32-S3:          10   46     -1      11      12      13     45        17       18       16       15      5V     GND  

/***********************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE 
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**********************************************************************************/

#include <TFT_eSPI.h> 
#include <SPI.h>
#include "touch.h"

#define BACKLIGHT_PIN 45

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// PWM related parameter settings
int freq = 2000;
int channel = 0;
int resolution = 8;

TFT_eSPI my_lcd = TFT_eSPI(); 

void setup()
{
    Serial.begin(115200);
    my_lcd.begin();
    my_lcd.setRotation(1);
    touch_init(my_lcd.width(), my_lcd.height(), my_lcd.getRotation());
    my_lcd.fillScreen(TFT_WHITE);
    my_lcd.setTextColor(TFT_BLACK, TFT_WHITE);
    my_lcd.setTextSize(2);
    my_lcd.setCursor(my_lcd.width()/2 - 150, 160);
    my_lcd.println("0");
    my_lcd.setCursor(my_lcd.width()/2 + 110, 160);
    my_lcd.println("255");
    my_lcd.fillRoundRect(my_lcd.width()/2 - 138, 130, 276, 21, 10, 0x07FF);
    my_lcd.fillCircle(my_lcd.width()/2 + 128, 140, 10, TFT_MAGENTA);
    my_lcd.setTextColor(TFT_RED);
    my_lcd.drawNumber(255, my_lcd.width()/2 - 15, 65, 2);

    ledcAttachChannel(BACKLIGHT_PIN, freq, resolution, channel);
    ledcWrite(BACKLIGHT_PIN, 255);
}

void loop()
{
    if (touch_touched())
    {
        int16_t t_x = touch_last_x;
        int16_t t_y = touch_last_y;
        if ((t_x >= my_lcd.width()/2 - 128) && (t_x < my_lcd.width()/2 + 128) && t_y > 129 && t_y < 149)
        {
            my_lcd.setTextColor(TFT_RED, TFT_WHITE);
            my_lcd.fillRoundRect(t_x - 10, 130, my_lcd.width()/2 + 138 - t_x + 10, 21, 10, 0x8430);
            my_lcd.fillRoundRect(my_lcd.width()/2 - 138, 130, t_x - my_lcd.width()/2 + 148, 21, 10, 0x07FF);
            my_lcd.fillCircle(t_x, 140, 10, TFT_MAGENTA);
            my_lcd.setTextPadding(50);
            int brightness = t_x - (my_lcd.width()/2 - 128);
            my_lcd.drawNumber(brightness, my_lcd.width()/2 - 15, 65, 2);
            ledcWrite(BACKLIGHT_PIN, brightness);
        }
    }
}