from machine import SPI, Pin
from ILI9341 import LCD_28_ILI9341, SPI_W_SPEED, Delay_Ms

#pin define
LCD_RS = 46
LCD_CS = 10
#LCD_RST = 27  #connect to ESP32 reset pin
LCD_SCK = 12
LCD_SDA = 11
LCD_SDO = 13
LCD_BL = 45

spi = SPI(2,baudrate=SPI_W_SPEED,sck=Pin(LCD_SCK),mosi=Pin(LCD_SDA),miso=Pin(LCD_SDO))
mylcd = LCD_28_ILI9341(spi, LCD_CS, LCD_RS, LCD_BL)

if __name__=='__main__':
    while True:
        mylcd.Show_BMP_Pic('cloud.bmp', 0, 0)
        Delay_Ms(1000)
        mylcd.Show_BMP_Pic('fruit.bmp', 0, 0)
        Delay_Ms(1000)