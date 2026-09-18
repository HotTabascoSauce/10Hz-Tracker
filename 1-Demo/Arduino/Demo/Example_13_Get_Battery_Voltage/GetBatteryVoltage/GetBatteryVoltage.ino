//This program is used to display the battery level

//pin usage as follow:
//                   CS  DC/RS  RESET    SDI/MOSI  SCK   SDO/MISO  BL   BAT_VOLT_ADC   VCC    GND    
//ESP32-S3:          15    2   ESP32-EN     13      14      12     27        34        5V     GND  

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

#include "Arduino.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_err.h"
#include "TFT_eSPI.h" /* Please use the TFT library provided in the library. */

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_8
#define ADC_ATTEN  ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

TFT_eSPI tft = TFT_eSPI();
unsigned long targetTime = 0;
adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t adc_cali_handle = NULL;
unsigned int bv = 0;

void setup()
{ 
    esp_err_t ret;
    Serial.begin(115200);

    // 初始化TFT屏幕
    tft.begin();
    tft.setRotation(0);
    tft.setTextSize(3);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawRoundRect(tft.width()/2-51, tft.height()/2, 102, 22, 3, TFT_WHITE);
    tft.fillRect(tft.width()/2+51, tft.height()/2+6, 5, 10, TFT_WHITE);

    //init adc
    adc_oneshot_unit_init_cfg_t init_config = 
    {
        .unit_id = ADC_UNIT,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);
    //configure adc channel
    adc_oneshot_chan_cfg_t channel_config = 
    {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };   
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &channel_config);
    //init adc cal
    adc_cali_curve_fitting_config_t cali_config = 
    {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
    if(ret == ESP_OK)
    {
      Serial.println("adc calibration scheme create successfully!!\r\n");
    }
    else
    {
      Serial.println("adc calibration scheme create failed!!\r\n");
    }
}

void loop()
{
    int adc_raw_value = 0, vol = 0;
    if (millis() > targetTime) 
    {
       adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw_value);
       if(adc_cali_raw_to_voltage(adc_cali_handle, adc_raw_value, &vol) == ESP_OK)
       {
          Serial.println(adc_raw_value);
          int v1 = vol * 2; // 考虑分压
          Serial.print(v1);
          Serial.println("mV");

          tft.setCursor(tft.width()/2-50, tft.height()/4);
          tft.print(v1);
          tft.print("mV");

          if(v1 <= 2500)
          {
            bv = 0;  
          }
          else if((v1 > 2500) && (v1 <= 4200))
          {
            bv = (v1 - 2500)/17;  
          }
          else
          {
            bv = 100;  
          }
          tft.fillRoundRect(tft.width()/2-50, tft.height()/2+1,100, 20, 3, TFT_BLACK);
          tft.fillRoundRect(tft.width()/2-50, tft.height()/2+1, bv, 20, 3, TFT_GREEN);
       }
       else
       {
          Serial.println("convert raw value to voltage failed!!");
       }

        targetTime = millis() + 1000;
    }
    delay(20);
}