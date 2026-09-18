from machine import Pin
import ustruct
import time

XCMD = 0x2A
YCMD = 0x2B
WCMD = 0x2C
RCMD = 0x2E
MADCTL = 0x36

LCD_W = 240
LCD_H = 320

SPI_W_SPEED = 40000000
SPI_R_SPEED = 20000000

#mem size
MEM_SIZE = 5120  #5x1024

DEFAULT_ROTATE = 0  # 0-0, 1-90, 2-180, 3-270

def Delay_Ms(time_ms):
    time.sleep_ms(time_ms)

def Color_To_RGB565(r, g, b):
    return (r & 0xF8) << 8 | (g & 0xFC) << 3 |  b >> 3

class LCD_28_ILI9341(object):
    def __init__(self, spi, cs, rs, blk, rst = None, width = LCD_W, height = LCD_H):
        self.spi = spi
        self.lcd_width = width
        self.lcd_height = height
        self.lcd_id = 0
        self.lcd_rotate = 0
        self.lcd_cs = Pin(cs, Pin.OUT)
        self.lcd_rs = Pin(rs, Pin.OUT)
        self.lcd_blk = Pin(blk, Pin.OUT)
        self.lcd_cs(1)
        self.lcd_rs(1)
        self.lcd_blk(0)
        if rst is not None:
            self.lcd_rst = Pin(rst, Pin.OUT)
            self.lcd_rst(1)
            self.LCD_Reset()
        self.LCD_Init()
        self.lcd_blk(1)
        self.LCD_Set_Rotate(DEFAULT_ROTATE)
    def LCD_Reset(self):
        Delay_Ms(50)
        self.lcd_rst(0)
        Delay_Ms(100)
        self.lcd_rst(1)
        Delay_Ms(50)
    def LCD_Write_Reg(self, cmd, data = None):
        self.lcd_cs(0)
        self.lcd_rs(0)
        self.spi.write(bytearray([cmd]))
        if data is not None:
            self.lcd_rs(1)
            self.spi.write(data)
        self.lcd_cs(1)
    def LCD_Write_Data(self, data):
        self.lcd_cs(0)
        self.lcd_rs(1)
        self.spi.write(data)
        self.lcd_cs(1)
    def LCD_Read_Color(self):
        self.lcd_cs(0)
        self.lcd_rs(0)
        self.spi.write(bytearray([RCMD]))
        self.lcd_rs(1)
        buf = self.spi.read(4)
        self.lcd_cs(1)
        return Color_To_RGB565(buf[1], buf[2], buf[3])
    def LCD_Init(self):     
        self.LCD_Write_Reg(0xCF, b"\x00\xC1\x30") 
        self.LCD_Write_Reg(0xED, b"\x64\x03\x12\x81")
        self.LCD_Write_Reg(0xE8, b"\x85\x00\x78")
        self.LCD_Write_Reg(0xCB, b"\x39\x2C\x00\x34\x02")
        self.LCD_Write_Reg(0xF7, b"\x20")
        self.LCD_Write_Reg(0xEA, b"\x00\x00")
        self.LCD_Write_Reg(0xC0, b"\x13")
        self.LCD_Write_Reg(0xC1, b"\x13")  
        self.LCD_Write_Reg(0xC5, b"\x22\x35")
        self.LCD_Write_Reg(0xC7, b"\xBD")        
        self.LCD_Write_Reg(0x36, b"\x08")
        self.LCD_Write_Reg(0xB6, b"\x0A\xA2")
        self.LCD_Write_Reg(0x3A, b"\x55")
        self.LCD_Write_Reg(0xF6, b"\x01\x30")
        self.LCD_Write_Reg(0xB1, b"\x00\x1B")
        self.LCD_Write_Reg(0xF2, b"\x00")
        self.LCD_Write_Reg(0x26, b"\x01")
        self.LCD_Write_Reg(0xE0, b"\x0F\x35\x31\x0B\x0E\x06\x49\xA7\x33\x07\x0F\x03\x0C\x0A\x00")        
        self.LCD_Write_Reg(0XE1, b"\x00\x0A\x0F\x04\x11\x08\x36\x58\x4D\x07\x10\x0C\x32\x34\x0F")        
        self.LCD_Write_Reg(0X21)
        self.LCD_Write_Reg(0X11)
        Delay_Ms(120)
        self.LCD_Write_Reg(0X29)
    def LCD_Set_Windows(self,x1,y1,x2,y2):
        self.LCD_Write_Reg(XCMD, ustruct.pack(">HH", x1, x2))
        self.LCD_Write_Reg(YCMD, ustruct.pack(">HH", y1, y2))
        self.LCD_Write_Reg(WCMD)
    def LCD_Clear(self, color): 
        self.LCD_Set_Windows(0,0,self.lcd_width-1,self.lcd_height-1)
        self.lcd_cs(0)
        self.lcd_rs(1)
        quo, rest = divmod(self.lcd_width*self.lcd_height,MEM_SIZE)
        if quo:
            buf = ustruct.pack(">H",color)*MEM_SIZE
            for i in range(quo):
                self.spi.write(buf)
        if rest:
            buf = ustruct.pack(">H",color)*rest
            self.spi.write(buf)
        self.lcd_cs(1)
    def LCD_Read_ID(self):
        val = bytearray(3)
        self.spi.init(baudrate=SPI_R_SPEED)
        self.lcd_cs(0)
        for i in range(1, 4):
            self.lcd_rs(0)
            self.spi.write(bytearray([0xD9]))
            self.lcd_rs(1)
            self.spi.write(bytearray([0x10 + i]))
            self.lcd_rs(0)
            self.spi.write(bytearray([0xD3]))
            self.lcd_rs(1)
            val[i - 1] = int.from_bytes(self.spi.read(1), 'big')
        self.spi.init(baudrate=SPI_W_SPEED)
        self.lcd_cs(1)
        self.lcd_id = val[1]
        self.lcd_id <<= 8
        self.lcd_id |= val[2]
        return self.lcd_id
    def Rotate_0_deg(self):
        self.lcd_width = LCD_W
        self.lcd_height = LCD_H
        self.LCD_Write_Reg(MADCTL, b"\x08")
    def Rotate_90_deg(self):
        self.lcd_width = LCD_H
        self.lcd_height = LCD_W
        self.LCD_Write_Reg(MADCTL, b"\x68")    
    def Rotate_180_deg(self):
        self.lcd_width = LCD_W
        self.lcd_height = LCD_H
        self.LCD_Write_Reg(MADCTL, b"\xC8")    
    def Rotate_270_deg(self):
        self.lcd_width = LCD_H
        self.lcd_height = LCD_W
        self.LCD_Write_Reg(MADCTL, b"\xA8")
    def LCD_Set_Rotate(self, ro):
        self.lcd_rotate = ro % 4
        rotate = {
            0: self.Rotate_0_deg,
            1: self.Rotate_90_deg,
            2: self.Rotate_180_deg,
            3: self.Rotate_270_deg,
        }
        rotate[self.lcd_rotate]()
    def Draw_Point(self, x, y, color):
        self.LCD_Set_Windows(x,y,x,y)
        self.LCD_Write_Data(ustruct.pack(">H",color))
    def Read_Point(self, x, y):
        self.spi.init(baudrate=SPI_R_SPEED)
        self.LCD_Set_Windows(x,y,x,y)
        color = self.LCD_Read_Color()
        self.spi.init(baudrate=SPI_W_SPEED)
        return color
    def Read_Region(self, x, y, w, h):
        total = w * h
        buf = bytearray(2 * total)
        self.spi.init(baudrate=SPI_R_SPEED)
        self.LCD_Set_Windows(x, y, x + w - 1, y + h - 1)
        self.lcd_cs(0)
        self.lcd_rs(0)
        self.spi.write(bytearray([RCMD]))
        self.lcd_rs(1)
        self.spi.read(1)
        for j in range(h):
            tbuf = self.spi.read(3 * w)
            for i in range(w):
                color = ustruct.pack(">H",Color_To_RGB565(tbuf[i*3], tbuf[i*3+1], tbuf[i*3+2]))
                buf[(j*w+i)*2] = color[0]
                buf[(j*w+i)*2+1] = color[1]
        self.lcd_cs(1)
        self.spi.init(baudrate=SPI_W_SPEED)
        return buf
    def Fill_Region(self, x, y, buf, w, h):
        self.LCD_Set_Windows(x, y, x + w - 1, y + h - 1)
        self.lcd_cs(0)
        self.lcd_rs(1)
        self.spi.write(buf)
        self.lcd_cs(1)
    def Fill_Rect(self, x, y, w, h, color):
        if x >= self.lcd_width or y >= self.lcd_height:
            return
        if (w < 1 or w > self.lcd_width) or (h < 1 or h > self.lcd_height):
            return
        if (x + w) > self.lcd_width:
            w = self.lcd_width - x
        if (y + h) > self.lcd_height:
            h = self.lcd_height - y
        self.LCD_Set_Windows(x, y, x + w - 1, y + h - 1)
        self.lcd_cs(0)
        self.lcd_rs(1)
        quo, rest = divmod(w * h, MEM_SIZE)
        if quo:
            buf = ustruct.pack(">H", color) * MEM_SIZE
            for i in range(quo):
                self.spi.write(buf)
        if rest != 0:
            rbuf = ustruct.pack(">H", color) * rest
            self.spi.write(rbuf)
        self.lcd_cs(1)
    def Draw_Hline(self, x, y, w, color):
        self.LCD_Set_Windows(x, y, x + w - 1, y)
        self.lcd_cs(0)
        self.lcd_rs(1)
        quo, rest = divmod(w, 10)
        if quo:
            buf = ustruct.pack(">H", color) * 10
            for i in range(quo):
                self.spi.write(buf)
        if rest != 0:
            rbuf = ustruct.pack(">H", color) * rest
            self.spi.write(rbuf)
        self.lcd_cs(1)
    def Draw_Vline(self, x, y, h, color):
        self.LCD_Set_Windows(x, y, x, y + h - 1)
        self.lcd_cs(0)
        self.lcd_rs(1)
        quo, rest = divmod(h, 10)
        if quo:
            buf = ustruct.pack(">H", color) * 10
            for i in range(quo):
                self.spi.write(buf)
        if rest != 0:
            rbuf = ustruct.pack(">H", color) * rest
            self.spi.write(rbuf)
        self.lcd_cs(1)
    def Draw_line(self, x1, y1, x2, y2, color):
        ste = abs(y2 - y1) > abs(x2 - x1)
        if ste:
            x1, y1 = y1, x1
            x2, y2 = y2, x2
        if x1 > x2:           
            x1, x2 = x2, x1
            y1, y2 = y2, y1
        dx = x2 - x1
        dy = abs(y2 - y1)
        err = dx >> 1
        ys = -1
        xs = x1
        dlen = 0
        if y1 < y2:
            ys = 1;
        if ste:
            while x1 <= x2:
                dlen += 1
                err -= dy
                if err < 0:
                    if dlen == 1:
                        self.Draw_Point(y1, xs, color)
                    else:
                        self.Draw_Vline(y1, xs, dlen, color)
                    dlen = 0
                    y1 += ys
                    xs = x1 + 1
                    err += dx
                x1 += 1
            if dlen:
                self.Draw_Vline(y1, xs, dlen, color)
        else:
            while x1 <= x2:
                dlen += 1
                err -= dy
                if err < 0:
                    if dlen == 1:
                        self.Draw_Point(xs, y1, color)
                    else:
                        self.Draw_Hline(xs, y1, dlen, color)
                    dlen = 0
                    y1 += ys
                    xs = x1 + 1
                    err += dx
                x1 += 1
            if dlen:
                self.Draw_Hline(xs, y1, dlen, color)                            
    def Draw_Rect(self, x, y, w, h, color):
        self.Draw_Hline(x, y, w, color)
        self.Draw_Hline(x, y + h - 1, w, color)
        self.Draw_Vline(x, y, h, color)
        self.Draw_Vline(x + w - 1, y, h, color)
    def Draw_Circle_Corner(self, x, y, r, n, color):
        if r <= 0:
            return
        f = 1 - r
        dfx = 1
        dfy = -2 * r
        xe = 0
        xs = 0
        dlen = 0
        while True:
            while f < 0:
                xe += 1
                dfx += 2
                f += dfx
            dfy += 2
            f += dfy
            if (xe - xs) == 1:
                if (n & 0x1):
                    self.Draw_Point(x - xe, y - r, color)
                    self.Draw_Point(x - r, y - xe, color)
                if (n & 0x2):
                    self.Draw_Point(x + r, y - xe, color)
                    self.Draw_Point(x + xs + 1, y - r, color)
                if (n & 0x4):
                    self.Draw_Point(x + xs + 1, y + r, color)
                    self.Draw_Point(x + r, y + xs + 1, color)
                if (n & 0x8):
                    self.Draw_Point(x - r, y + xs + 1, color)
                    self.Draw_Point(x - xe, y + r, color)
            else:
                dlen = xe - xs
                xs += 1
                if (n & 0x1):
                    self.Draw_Hline(x - xe, y - r, dlen, color)
                    self.Draw_Vline(x - r, y - xe, dlen, color)
                if (n & 0x2):
                    self.Draw_Vline(x + r, y - xe, dlen, color)
                    self.Draw_Hline(x + xs, y - r, dlen, color)
                if (n & 0x4):
                    self.Draw_Hline(x + xs, y + r, dlen, color)
                    self.Draw_Vline(x + r, y + xs, dlen, color)
                if (n & 0x8):
                    self.Draw_Vline(x - r, y + xs, dlen, color)
                    self.Draw_Hline(x - xe, y + r, dlen, color)
            xs = xe
            if xe >= r:
                break
            r -= 1
    def Fill_Circle_Corner(self, x, y, r, n, d, color):
        f = 1 - r
        dfx = 1
        dfy = -2*r
        by = 0
        d += 1
        while by < r:
            if f >= 0:
                if (n & 0x1):
                    self.Draw_Hline(x - by, y + r, 2*by + d, color)
                if (n & 0x2):
                    self.Draw_Hline(x - by, y - r, 2*by + d, color)
                r -= 1
                dfy += 2
                f += dfy
            by += 1
            dfx += 2
            f += dfx
            if (n & 0x1):
                self.Draw_Hline(x - r, y + by, 2*r + d, color)
            if (n & 0x2):
                self.Draw_Hline(x - r, y - by, 2*r + d, color)           
    def Draw_Round_Rect(self, x, y, w, h, r, color):
        self.Draw_Hline(x + r, y, w - 2*r, color)
        self.Draw_Hline(x + r, y + h - 1, w - 2*r, color)
        self.Draw_Vline(x, y + r, h - 2*r, color)
        self.Draw_Vline(x + w - 1, y + r, h - 2*r, color)
        self.Draw_Circle_Corner(x + r, y + r, r, 1, color)
        self.Draw_Circle_Corner(x + w - r - 1, y + r, r, 2, color)
        self.Draw_Circle_Corner(x + w - r - 1, y + h - r - 1, r, 4, color)
        self.Draw_Circle_Corner(x + r, y + h - r - 1, r, 8, color)
    def Fill_Round_Rect(self, x, y, w, h, r, color):
        self.Fill_Rect(x, y + r, w, h - 2*r, color)
        self.Fill_Circle_Corner(x + r, y + h - r - 1, r, 1, w - 2*r - 1, color)
        self.Fill_Circle_Corner(x + r, y + r, r, 2, w - 2*r - 1, color)
    def Draw_Triangle(self, x1, y1, x2, y2, x3, y3, color):
        self.Draw_line(x1, y1, x2, y2, color)
        self.Draw_line(x2, y2, x3, y3, color)
        self.Draw_line(x3, y3, x1, y1, color)
    def Fill_Triangle(self, x1, y1, x2, y2, x3, y3, color):
        if y1 > y2:
            y1, y2 = y2, y1
            x1, x2 = x2, x1
        if y2 > y3:
            y3, y2 = y2, y3
            x3, x2 = x2, x3
        if y1 > y2:
            y1, y2 = y2, y1
            x1, x2 = x2, x1
        if y1 == y3:
            a, b = x1, x1
            if x2 < a:
                a = x2
            elif x2 > b:
                b = x2
            if x3 < a:
                a = x3
            elif x3 > b:
                b = x3
            self.Draw_Hline(a, y1, b - a + 1, color)
            return
        dx1 = x2 - x1
        dy1 = y2 - y1
        dx2 = x3 - x1
        dy2 = y3 - y1
        dx3 = x3 - x2
        dy3 = y3 - y2
        aa = 0
        bb = 0
        if y2 == y3:
            last = y2
        else:
            last = y2 - 1
        i = y1
        while i <= last:
            a = x1 + aa // dy1
            b = x1 + bb // dy2
            aa += dx1
            bb += dx2
            if a > b:
                a, b = b, a
            self.Draw_Hline(a, i, b - a + 1, color)
            i += 1
        aa = dx3 * (i - y2)
        bb = dx2 * (i - y1)
        while i <= y3:
            a = x2 + aa // dy3
            b = x1 + bb // dy2
            aa += dx3
            bb += dx2
            if a > b:
                a, b = b, a
            self.Draw_Hline(a, i, b - a + 1, color)
            i += 1
    def Draw_Circle(self, x, y, r, color):
        f = 1 - r
        dfy = -2 * r
        dfx = 1
        xs = -1
        xe = 0
        dlen = 0
        flag = True
        while True:
            while f < 0:
                xe += 1
                dfx += 2
                f += dfx
            dfy += 2
            f += dfy
            if (xe - xs) > 1:
                if flag:
                    dlen = 2 * (xe - xs) - 1
                    self.Draw_Hline(x - xe, y + r, dlen, color)
                    self.Draw_Hline(x - xe, y - r, dlen, color)
                    self.Draw_Vline(x + r, y - xe, dlen, color)
                    self.Draw_Vline(x - r, y - xe, dlen, color)
                    flag = False
                else:
                    dlen = xe - xs
                    xs += 1
                    self.Draw_Hline(x - xe, y + r, dlen, color)
                    self.Draw_Hline(x - xe, y - r, dlen, color)
                    self.Draw_Hline(x + xs, y - r, dlen, color)
                    self.Draw_Hline(x + xs, y + r, dlen, color)
                    self.Draw_Vline(x + r, y + xs, dlen, color)
                    self.Draw_Vline(x + r, y - xe, dlen, color)
                    self.Draw_Vline(x - r, y - xe, dlen, color)
                    self.Draw_Vline(x - r, y + xs, dlen, color)
            else:
                xs += 1
                self.Draw_Point(x - xe, y + r, color)
                self.Draw_Point(x - xe, y - r, color)
                self.Draw_Point(x + xs, y - r, color)
                self.Draw_Point(x + xs, y + r, color)
                self.Draw_Point(x + r, y + xs, color)
                self.Draw_Point(x + r, y - xe, color)
                self.Draw_Point(x - r, y - xe, color)
                self.Draw_Point(x - r, y + xs, color)
            xs = xe
            r -= 1
            if xe >= r:
                break
    def Fill_Circle(self, x, y, r, color):
        bx = 0
        dx = 1
        dy = 2*r
        p = -(r>>1)
        self.Draw_Hline(x - r, y, dy + 1, color)
        while bx < r:
            if p >= 0:
                self.Draw_Hline(x - bx, y + r, dx, color)
                self.Draw_Hline(x - bx, y - r, dx, color)
                dy -= 2
                p -= dy
                r -= 1
            dx += 2
            p += dx
            bx += 1
            self.Draw_Hline(x - r, y + bx, dy + 1, color)
            self.Draw_Hline(x - r, y - bx, dy + 1, color)            
    def Draw_Ellipse(self, x, y, xr, yr, color):
        if xr < 2:
            return
        if yr < 2:
            return
        xr1 = xr * xr
        yr1 = yr * yr
        fx = 4 * xr1
        fy = 4 * yr1
        bx = 0
        by = yr
        s = 2 * yr1 + xr1 * (1 - 2 * yr)
        while yr1 * bx <= xr1 * by:
            self.Draw_Point(x + bx, y + by, color)
            self.Draw_Point(x - bx, y + by, color)
            self.Draw_Point(x - bx, y - by, color)
            self.Draw_Point(x + bx, y - by, color)
            if s >= 0:
                s += fx * (1 - by)
                by -= 1
            s += yr1 * (4 * bx + 6)
            bx += 1
        bx = xr
        by = 0
        s = 2 * xr1 + yr1 * (1 - 2 * xr)
        while xr1 * by <= yr1 * bx:
            self.Draw_Point(x + bx, y + by, color)
            self.Draw_Point(x - bx, y + by, color)
            self.Draw_Point(x - bx, y - by, color)
            self.Draw_Point(x + bx, y - by, color)
            if s >= 0:
                s += fy * (1 - bx)
                bx -= 1
            s += xr1 * (4 * by + 6)
            by += 1        
    def Fill_Ellipse(self, x, y, xr, yr, color):
        if xr < 2:
            return
        if yr < 2:
            return
        xr1 = xr * xr
        yr1 = yr * yr
        fx = 4 * xr1
        fy = 4 * yr1
        bx = 0
        by = yr
        s = 2 * yr1 + xr1 * (1 - 2 * yr)
        while yr1 * bx <= xr1 * by:
            self.Draw_Hline(x - bx, y - by, 2 * bx + 1, color)
            self.Draw_Hline(x - bx, y + by, 2 * bx + 1, color) 
            if s >= 0:
                s += fx * (1 - by)
                by -= 1
            s += yr1 * (4 * bx + 6)
            bx += 1
        bx = xr
        by = 0
        s = 2 * xr1 + yr1 * (1 - 2 * xr)
        while xr1 * by <= yr1 * bx:
            self.Draw_Hline(x - bx, y - by, 2 * bx + 1, color)
            self.Draw_Hline(x - bx, y + by, 2 * bx + 1, color) 
            if s >= 0:
                s += fy * (1 - bx)
                bx -= 1
            s += xr1 * (4 * by + 6)
            by += 1
    def Show_BMP_Pic(self, picpath, x, y):
        with open(picpath, 'rb') as file:
            if file.read(2) == b'BM':
                dummy = file.read(8)
                offset = int.from_bytes(file.read(4), 'little')
                hdrsize = int.from_bytes(file.read(4), 'little')
                width = int.from_bytes(file.read(4), 'little')
                height = int.from_bytes(file.read(4), 'little')
                rbuf = bytearray(width * 2)
                if int.from_bytes(file.read(2), 'little') == 1:
                    depth = int.from_bytes(file.read(2), 'little')
                    if depth == 24 and int.from_bytes(file.read(4), 'little') == 0:
                        rowsize = (width * 3 + 3) & ~3
                        if height < 0:
                            height = -height
                            flip = False
                        else:
                            flip = True
                        self.LCD_Set_Windows(x, y, x + width - 1, y + height - 1)
                        for row in range(height):
                            if flip:
                                pos = offset + (height - 1 - row) * rowsize
                            else:
                                pos = offset + row * rowsize
                            if file.tell() != pos:
                                dummy = file.seek(pos)
                            bgr = file.read(3 * width)
                            for col in range(width):
                                buf = ustruct.pack(">H",((bgr[2 + col * 3] & 0xF8) << 8) | ((bgr[1 + col * 3] & 0xFC) << 3) | (bgr[0 + col * 3] >> 3))
                                rbuf[col * 2] = buf[0]
                                rbuf[col * 2 + 1] = buf[1]
                            self.lcd_cs(0)
                            self.lcd_rs(1)
                            self.spi.write(rbuf)
                            self.lcd_cs(1)
    def Show_Char(self, x, y, char, font, color, backcolor = None):
        if font == None:
            return
        if font.get('start') == None:
            return
        chw = font['width']
        chh = font['height']
        num = ord(char) - 32
        if num < font['start'] or num > font['end']:
            return
        bytes_line , res = divmod(chw, 8)
        if res != 0:
            bytes_line += 1
        bytes_char = bytes_line * chh
        char_data = font["data"][bytes_char * num:bytes_char * (num + 1)]
        if backcolor is not None:
            color_buf = bytearray(backcolor.to_bytes(2, 'big') * chw * chh)
        else:
            color_buf = self.Read_Region(x, y, chw, chh)
        for i in range(chh):
            pdata = int.from_bytes(char_data[i * bytes_line:(i + 1) * bytes_line], 'big')
            for j in range(chw):
                if pdata & (0x1 << (8 * bytes_line -1)):
                    color_buf[(i * chw + j) * 2] = color >> 8
                    color_buf[(i * chw + j) * 2 + 1] = color & 0xFF
                pdata <<= 1
        self.LCD_Set_Windows(x, y, x + chw - 1, y + chh - 1)
        self.lcd_cs(0)
        self.lcd_rs(1)
        self.spi.write(color_buf)
        self.lcd_cs(1)
    def Show_Signal_CN(self, x, y, char, font, color, backcolor = None):
        if font == None:
            return
        cs_data = font.get(char)
        if cs_data == None:
            return
        cn_data = cs_data['Mask']
        chw = font['width']
        chh = font['height']
        bytes_line , res = divmod(chw, 8)
        if res != 0:
            bytes_line += 1
        if backcolor is not None:
            color_buf = bytearray(backcolor.to_bytes(2, 'big') * chw * chh)
        else:
            color_buf = self.Read_Region(x, y, chw, chh)
        for i in range(chh):
            pdata = int.from_bytes(cn_data[i * bytes_line:(i + 1) * bytes_line], 'big')
            for j in range(chw):
                if pdata & (0x1 << (8 * bytes_line -1)):
                    color_buf[(i * chw + j) * 2] = color >> 8
                    color_buf[(i * chw + j) * 2 + 1] = color & 0xFF
                pdata <<= 1
        self.LCD_Set_Windows(x, y, x + chw - 1, y + chh - 1)
        self.lcd_cs(0)
        self.lcd_rs(1)
        self.spi.write(color_buf)
        self.lcd_cs(1)
    def Show_String(self, x, y, string, font, color, backcolor = None):
        x0 = x
        chw = font['width']
        chh = font['height'] 
        for char in string:
            num = ord(char)
            if num >= 0 and num <= 255:
                self.Show_Char(x, y, char, font, color, backcolor)
            else: 
                self.Show_Signal_CN(x, y, char, font, color, backcolor)             
            x += chw
            if (x + chw) > self.lcd_width:
                x = x0
                y += chh       