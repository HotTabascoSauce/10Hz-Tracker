from machine import SPI, Pin, I2C
from ILI9341 import LCD_28_ILI9341, SPI_W_SPEED, SPI_R_SPEED, Delay_Ms
from touch import FT6336
from Font_8x16_EN import Font_8x16_EN
import time

#pin define
LCD_RS = 46
LCD_CS = 10
#LCD_RST = 27  #connect to ESP32 reset pin
LCD_SCK = 12
LCD_SDA = 11
LCD_SDO = 13
LCD_BL = 45

TOUCH_RST = 18
I2C_SCL = 15
I2C_SDA = 16

spi = SPI(2,baudrate=SPI_W_SPEED,sck=Pin(LCD_SCK),mosi=Pin(LCD_SDA),miso=Pin(LCD_SDO))
i2c = I2C(0,scl=Pin(I2C_SCL),sda=Pin(I2C_SDA),freq=400000)
mylcd = LCD_28_ILI9341(spi, LCD_CS, LCD_RS, LCD_BL)
mytouch = FT6336(i2c, TOUCH_RST, width = 240, height = 320)

if __name__=='__main__':
    mylcd.LCD_Set_Rotate(0)
    mytouch.TP_Set_Rotation(mylcd.lcd_rotate)
    mylcd.LCD_Clear(0xFFFF)
    if mytouch.TP_Read_ID() != False:
        mylcd.Show_String(5, 100, "touch ID error!", Font_8x16_EN, 0x001F)
    else:
        mylcd.Show_String(mylcd.lcd_width - 36, 0, "RST", Font_8x16_EN, 0x001F)
        while True:
            if mytouch.TP_Get_Touch():
                if mytouch.tp_x[0] < mylcd.lcd_width and mytouch.tp_y[0] < mylcd.lcd_height:
                    if mytouch.tp_x[0] > mylcd.lcd_width - 36 and mytouch.tp_y[0] < 16:
                        mylcd.LCD_Clear(0xFFFF)
                        mylcd.Show_String(mylcd.lcd_width - 36, 0, "RST", Font_8x16_EN, 0x001F)
                    else:
                        mylcd.Fill_Circle(mytouch.tp_x[0], mytouch.tp_y[0], 2, 0xF800)
            else:
                mytouch.tp_x[0] = 0xFFFF
                mytouch.tp_y[0] = 0xFFFF
        