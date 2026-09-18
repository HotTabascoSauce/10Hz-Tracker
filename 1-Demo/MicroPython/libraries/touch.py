from machine import Pin
import ustruct
import time

CTP_MAX_TOUCH = 2

FT_ADDRESS = 0x38
FT_CMD_WR = (FT_ADDRESS<<2)
FT_CMD_RD = (FT_ADDRESS<<2 + 1)

FT_DEVIDE_MODE = 0x00
FT_REG_NUM_FINGER = 0x02
FT_TP1_REG = 0x03
FT_TP2_REG = 0x09
FT_ID_G_CIPHER_MID = 0x9F
FT_ID_G_CIPHER_LOW = 0xA0
FT_ID_G_LIB_VERSION = 0xA1
FT_ID_G_CIPHER_HIGH = 0xA3
FT_ID_G_MODE = 0xA4
FT_ID_G_FOCALTECH_ID = 0xA8
FT_ID_G_THGROUP = 0x80
FT_ID_G_PERIODACTIVE = 0x88

class FT6336(object):
    def __init__(self, i2c, rst, irq = None, width = 240, height = 320):
        self.i2c = i2c
        self.tp_rotate = 0
        self.tp_width = width
        self.tp_height = height
        self.tp_x = [0xFFFF, 0xFFFF]
        self.tp_y = [0xFFFF, 0xFFFF]
        self.tp_rst = Pin(rst, Pin.OUT)
        if irq is not None:
            self.tp_irq = Pin(irq, Pin.IN)
        self.TP_Reset()
    def TP_Reset(self):
        self.tp_rst(1)
        time.sleep_ms(20)
        self.tp_rst(0)
        time.sleep_ms(20)
        self.tp_rst(1)
        time.sleep_ms(500)
    def TP_Read_ID(self):
        buf = bytearray(2)
        self.i2c.readfrom_mem_into(FT_ADDRESS, FT_ID_G_FOCALTECH_ID, buf, addrsize=8)
        if buf[0] != 0x11:
            return True
        self.i2c.readfrom_mem_into(FT_ADDRESS, FT_ID_G_CIPHER_MID, buf, addrsize=8)
        if buf[0] != 0x26:
            return True
        if buf[1] != 0x00 and buf[1] != 0x01 and buf[1] != 0x02:
            return True
        self.i2c.readfrom_mem_into(FT_ADDRESS, FT_ID_G_CIPHER_HIGH, buf, addrsize=8)
        if buf[0] != 0x64:
            return True
        return False
    def TP_Set_Rotation(self, rotation):
        self.tp_rotate = rotation % 4
    def TP_Get_Touch(self):
        tp_buf = bytearray(4)
        num = self.i2c.readfrom_mem(FT_ADDRESS, FT_REG_NUM_FINGER, 1, addrsize=8)
        touch_num = int.from_bytes(num, 'big')
        if touch_num > 0 and touch_num < 3:
            for i in range(touch_num):
                self.i2c.readfrom_mem_into(FT_ADDRESS, FT_TP1_REG + i * 6, tp_buf, addrsize=8)
                x = (tp_buf[0]&0x0F) << 8
                x |= tp_buf[1]
                y = (tp_buf[2]&0x0F) << 8
                y |= tp_buf[3]
                if self.tp_rotate == 0:
                    x = x
                    y = y
                elif self.tp_rotate == 1:
                    temp = x
                    x = y
                    y = self.tp_width - temp
                elif self.tp_rotate == 2:
                    x = self.tp_width - x
                    y = self.tp_height - y
                elif self.tp_rotate == 3:
                    temp = x
                    x = self.tp_height - y
                    y = temp
                self.tp_x[i] = x
                self.tp_y[i] = y
            return True
        else:
            return False