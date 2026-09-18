// IMPORTANT: LCDWIKI_SPI LIBRARY AND LCDWIKI_TOUCH LIBRARY MUST BE SPECIFICALLY
// CONFIGURED FOR EITHER THE TFT SHIELD OR THE BREAKOUT BOARD.

//This program is a demo of drawing

//pin usage as follow:
//            CS  DC/RS  RESET  SDI/MOSI  SCK  SDO/MISO  BL   CTP_INT  CTP_RST  CTP_SDA  CTP_SCL   VCC     GND    
//ESP32-S3:   10   46     -1      11      12      13     45     17       18       16       15       5V     GND  

//Remember to set the pins to suit your display module!

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
#include <TFT_eSPI.h> 
#include <SPI.h>
#include "touch.h"

TFT_eSPI my_lcd = TFT_eSPI();

#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

uint16_t color_mask[] = {0xF800,0x001F}; //color select


void LCD_Draw_Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t Size,uint16_t colour)
{
  uint16_t t; 
  int xerr=0,yerr=0,delta_x,delta_y,distance; 
  int incx,incy,uRow,uCol; 
  if(x1<Size||x2<Size||y1<Size||y2<Size)
  {
    return;  
  }
  delta_x=x2-x1; //计算坐标增量 
  delta_y=y2-y1; 
  uRow=x1; 
  uCol=y1; 
  if(delta_x>0)incx=1; //设置单步方向 
  else if(delta_x==0)incx=0;//垂直线 
  else {incx=-1;delta_x=-delta_x;} 
  if(delta_y>0)incy=1; 
  else if(delta_y==0)incy=0;//水平线 
  else{incy=-1;delta_y=-delta_y;} 
  if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
  else distance=delta_y; 
  for(t=0;t<=distance+1;t++ )//画线输出 
  {  
     my_lcd.fillCircle(uRow, uCol, Size,colour);
    //gui_circle(uRow, uCol,color, size, 1);
    //LCD_DrawPoint(uRow,uCol);//画点 
    xerr+=delta_x ; 
    yerr+=delta_y ; 
    if(xerr>distance) 
    { 
      xerr-=distance; 
      uRow+=incx; 
    } 
    if(yerr>distance) 
    { 
      yerr-=distance; 
      uCol+=incy; 
    } 
  }  
}

void setup(void) 
{
  my_lcd.init();
  my_lcd.setRotation(0);  
  touch_init(my_lcd.width(), my_lcd.height(),my_lcd.getRotation());
  my_lcd.fillScreen(WHITE);

  my_lcd.setTextColor(BLUE);
  my_lcd.drawString("RST",my_lcd.width()-36,0,2);
}

void loop()
{
  int16_t lastpos[2] = {0};
  while(1)
  { 
      if(touch_touched()) 
      {
        if(touch_last_x<my_lcd.width()&&touch_last_y<my_lcd.height()) 
        {
          if(lastpos[0]==0xFFFF) 
          {
             lastpos[0]=touch_last_x;
             lastpos[1]=touch_last_y;
           }
           if(touch_last_x>(my_lcd.width()-36)&&touch_last_y<16)
           {
              touch_last_x = 0xFFFF;
              touch_last_y = 0xFFFF;
              my_lcd.fillScreen(WHITE);
              my_lcd.setTextColor(BLUE);
              my_lcd.drawString("RST",my_lcd.width()-36,0,2);
            } 
            else
            {
                LCD_Draw_Line(lastpos[0],lastpos[1],touch_last_x,touch_last_y,2,color_mask[0]);
            }          
           lastpos[0]=touch_last_x;
           lastpos[1]=touch_last_y;
          }
       }
       else
       {
          lastpos[0]=0XFFFF;
        }
    // }
  }
}
