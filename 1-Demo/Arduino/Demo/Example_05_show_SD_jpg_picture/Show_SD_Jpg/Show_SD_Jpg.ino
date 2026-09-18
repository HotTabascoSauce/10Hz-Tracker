// Example for library:
// https://github.com/Bodmer/TJpg_Decoder

// This example if for an ESP8266 or ESP32, it renders a Jpeg file
// that is stored in a SD card file. The test image is in the sketch
// "data" folder (press Ctrl+K to see it). You must save the image
// to the SD card using you PC.

//            CS  DC/RS  RESET  SDI/MOSI  SCK  SDO/MISO  LED   SD_CS   VCC     GND    
//ESP32-S3:   10    2      15      11      12      13     21    7       5V     GND  

// Include the jpeg decoder library
#include <TJpg_Decoder.h>

#include "SD_MMC.h"
//#define FS_NO_GLOBALS
#include <FS.h>
//#ifdef ESP32S3
//  #include "SPIFFS.h" // ESP32 only
//#endif

#define SD_CLK   38
#define SD_CMD   40
#define SD_D0    39
#define SD_D1    41
#define SD_D2    48
#define SD_D3    47

// Include the TFT library https://github.com/Bodmer/TFT_eSPI

#include <TFT_eSPI.h>              // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library
//SPIClass MySPI(HSPI);

#define FILE_NUMBER 4
#define FILE_NAME_SIZE_MAX 20
char file_name[FILE_NUMBER][FILE_NAME_SIZE_MAX];

// This next function will be called during decoding of the jpeg file to
// render each block to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}


void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n Testing TJpg_Decoder library");

  strcpy(file_name[0],"/tulip.jpg");
  strcpy(file_name[1],"/game.jpg");
  strcpy(file_name[2],"/tree.jpg");
  strcpy(file_name[3],"/flower.jpg");
  
  if(! SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3))
  {
        Serial.println("Pin change failed!");
        return;
   }

  // Initialise SD before TFT
  if (!SD_MMC.begin()) 
  {
    Serial.println(F("SD.begin failed!"));
    while (1) delay(0);
  }
  Serial.println("\r\nInitialisation done.");

  // Initialise the TFT
  tft.begin();
  tft.setTextColor(0xFFFF, 0x0000);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true); // We need to swap the colour bytes (endianess)

  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);

  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);
}

void loop()
{
  int i = 0;
  //tft.fillScreen(TFT_RED);
  //delay(2000);

  // Time recorded for test purposes
  uint32_t t = millis();

  // Get the width and height in pixels of the jpeg if you wish
  uint16_t w = 0, h = 0;
  for(i=0;i<FILE_NUMBER;i++)
  {
    TJpgDec.getFsJpgSize(&w, &h, file_name[i], SD_MMC);
    Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);
  
    // Draw the image, top left at 0,0
    TJpgDec.drawFsJpg(0, 0, file_name[i],SD_MMC);
  
    // How much time did rendering take
    t = millis() - t;
    Serial.print(t); Serial.println(" ms");
  
    // Wait before drawing again
    delay(1500);
  }
}
