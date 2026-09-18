from machine import SPI, Pin
from ILI9341 import LCD_28_ILI9341, SPI_W_SPEED, SPI_R_SPEED
from Font_8x16_EN import Font_8x16_EN


#pin define
LCD_RS = 46
LCD_CS = 10
#LCD_RST = 27  #connect to ESP32 reset pin
LCD_SCK = 12
LCD_SDA = 11
LCD_SDO = 13
LCD_BL = 45

#color define
BLACK = 0x0000
WHITE = 0xFFFF
RED = 0xF800
GREEN = 0x07E0
BLUE = 0x001F
MAGENAT = 0xF81F
DARKBLUE = 0x01CF
LIGHTGREEN = 0x841F

color_list = [RED, MAGENAT, GREEN, DARKBLUE, BLUE, BLACK, LIGHTGREEN]

spi = SPI(2,baudrate=SPI_W_SPEED,sck=Pin(LCD_SCK),mosi=Pin(LCD_SDA),miso=Pin(LCD_SDO))
mylcd = LCD_28_ILI9341(spi, LCD_CS, LCD_RS, LCD_BL)

if __name__=='__main__':
    mylcd.LCD_Clear(WHITE)
    lcd_id = mylcd.LCD_Read_ID()
    string = "LCD ID: 0x{:04x}".format(lcd_id)
    mylcd.Show_String(10, 20, string, Font_8x16_EN, RED)
    for i in range(7):
        mylcd.Fill_Rect(10, 65 + i * 35 - 10, 20, 20, color_list[i])
        color = mylcd.Read_Point(20, 65 + i * 35)
        string = "read color: 0x{:04X}".format(color)
        mylcd.Show_String(40, 65 + i * 35 - 8, string, Font_8x16_EN, color_list[i])
        if color == color_list[i]:
            mylcd.Show_String(192, 65 + i * 35 - 8, "OK", Font_8x16_EN, color_list[i])
        else:
            mylcd.Show_String(192, 65 + i * 35 - 8, "ERROR", Font_8x16_EN, color_list[i])
        