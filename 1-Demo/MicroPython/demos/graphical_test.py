from machine import SPI, Pin
from ILI9341 import LCD_28_ILI9341, SPI_W_SPEED, Delay_Ms
from Font_8x16_EN import Font_8x16_EN
import random

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
YELLOW = 0xFFE0

color_list = [RED, MAGENAT, GREEN, YELLOW, BLUE]

spi = SPI(2,baudrate=SPI_W_SPEED,sck=Pin(LCD_SCK),mosi=Pin(LCD_SDA),miso=Pin(LCD_SDO))
mylcd = LCD_28_ILI9341(spi, LCD_CS, LCD_RS, LCD_BL)

def Title_Show(string):
    mylcd.LCD_Clear(BLACK)
    mylcd.Fill_Rect(0, 0, mylcd.lcd_width, 16, BLUE)
    mylcd.Show_String((mylcd.lcd_width - len(string)*8) // 2, 0, string, Font_8x16_EN, WHITE)
    mylcd.Fill_Rect(0, mylcd.lcd_height - 1 - 15 , mylcd.lcd_width, 16, BLUE)
    mylcd.Show_String((mylcd.lcd_width - 120) // 2, mylcd.lcd_height - 1 - 15, "www.lcdwiki.com", Font_8x16_EN, WHITE)
def Show_Points():
    Title_Show("Point test")
    for i in range(2000):
      mylcd.Draw_Point(random.randint(0,mylcd.lcd_width-1),random.randint(18,mylcd.lcd_height - 18), random.randint(0,0xFFFF))
    Delay_Ms(1000)
def Show_lines():
    Title_Show("line test")
    for i in range(50):
      mylcd.Draw_line(random.randint(0,mylcd.lcd_width-1),random.randint(18,mylcd.lcd_height - 18), random.randint(0,mylcd.lcd_width-1),random.randint(18,mylcd.lcd_height - 18), random.randint(0,0xFFFF))
    Delay_Ms(1000)
def Show_Rect():
    Title_Show("rectangle test")
    for i in range(5):
        mylcd.Draw_Rect(mylcd.lcd_width//2-90+(i*20),mylcd.lcd_height//2-90+(i*20),80, 80, color_list[i])
    Delay_Ms(1000)
    for i in range(5):
        mylcd.Fill_Rect(mylcd.lcd_width//2-90+(i*20),mylcd.lcd_height//2-90+(i*20),80, 80, color_list[i])
    Delay_Ms(1000)
def Show_Round_Rect():
    Title_Show("round rectangle test")
    for i in range(5):
        mylcd.Draw_Round_Rect(mylcd.lcd_width//2-90+(i*20),mylcd.lcd_height//2-90+(i*20),80, 80, 15, color_list[i])
    Delay_Ms(1000)
    for i in range(5):
        mylcd.Fill_Round_Rect(mylcd.lcd_width//2-90+(i*20),mylcd.lcd_height//2-90+(i*20),80, 80, 15, color_list[i])
    Delay_Ms(1000)
def Show_Triangle():
    Title_Show("triangle test")
    for i in range(5):
        mylcd.Draw_Triangle(mylcd.lcd_width//2-80+(i*30),mylcd.lcd_height//2-20+(i*25),mylcd.lcd_width//2-50-1+(i*30),mylcd.lcd_height//2-10-72-1+(i*25),mylcd.lcd_width//2-1-20+(i*30),mylcd.lcd_height//2-20+(i*25), color_list[i])
    Delay_Ms(1000)
    for i in range(5):
        mylcd.Fill_Triangle(mylcd.lcd_width//2-80+(i*30),mylcd.lcd_height//2-20+(i*25),mylcd.lcd_width//2-50-1+(i*30),mylcd.lcd_height//2-10-72-1+(i*25),mylcd.lcd_width//2-1-20+(i*30),mylcd.lcd_height//2-20+(i*25), color_list[i])
    Delay_Ms(1000)
def Show_Circle():
    Title_Show("circle test")
    for i in range(5):
        mylcd.Draw_Circle(mylcd.lcd_width//2-50+(i*25),mylcd.lcd_height//2-60+(i*25), 40, color_list[i])
    Delay_Ms(1000)
    for i in range(5):
        mylcd.Fill_Circle(mylcd.lcd_width//2-50+(i*25),mylcd.lcd_height//2-60+(i*25), 40, color_list[i])
    Delay_Ms(1000)    
def Show_Ellipse():
    Title_Show("ellipse test")
    for i in range(5):
        mylcd.Draw_Ellipse(mylcd.lcd_width//2-50+(i*25),mylcd.lcd_height//2-60+(i*25), 40, 20, color_list[i])
    Delay_Ms(1000)
    for i in range(5):
        mylcd.Fill_Ellipse(mylcd.lcd_width//2-50+(i*25),mylcd.lcd_height//2-60+(i*25), 40, 20, color_list[i])
    Delay_Ms(1000) 
if __name__=='__main__':
    while True:
        for i in range(4):
            mylcd.LCD_Set_Rotate(i)
            Show_Points()
            Show_lines()
            Show_Rect()
            Show_Round_Rect()
            Show_Triangle()
            Show_Circle()
            Show_Ellipse()