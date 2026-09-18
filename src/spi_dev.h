#ifndef __SPI_DEV_H__
#define __SPI_DEV_H__
#include <stdint.h>
#include "soc/spi_reg.h"
#include "driver/spi_master.h"
#include "hal/gpio_ll.h"

//SPI port select
//#define FSPI 1
//#define HSPI 2

#define REG_SPI_BASE(i) (((i)>1) ? (DR_REG_SPI3_BASE) : (DR_REG_SPI2_BASE))

#define SPI_PORT FSPI  //FSPI
#define SPI_FREQUENCY 80000000
#define SPI_MODE SPI_MODE0

volatile uint32_t* spi_write_len = (volatile uint32_t*)(SPI_MS_DLEN_REG(SPI_PORT));
volatile uint32_t* spi_write_buf = (volatile uint32_t*)(SPI_W0_REG(SPI_PORT));
volatile uint32_t* spi_cmd = (volatile uint32_t*)(SPI_CMD_REG(SPI_PORT));
volatile uint32_t* spi_usr = (volatile uint32_t*)(SPI_USER_REG(SPI_PORT));

//pin define for ESP32, include lcd pin and Capacitive touch screen pin
#define LCD_CS  10
#define LCD_RST -1
#define LCD_DC  46
#define LCD_BL  45
//below is SPI pin, The hardware SPI can be mapped to any pins
#define SPI_MISO 13
#define SPI_MOSI 11
#define SPI_SCLK 12

//Pin operation
#if ((LCD_CS>=0) && (LCD_CS<32))
    #define LCD_CS_LOW  GPIO.out_w1tc = (1 << LCD_CS)
    #define LCD_CS_HIGH GPIO.out_w1ts = (1 << LCD_CS)
#elif (LCD_CS>=32)
    #define LCD_CS_LOW  GPIO.out1_w1tc.val = (1 << (LCD_CS - 32))
    #define LCD_CS_HIGH GPIO.out1_w1ts.val = (1 << (LCD_CS - 32))
#endif

#if ((LCD_RST>=0) && (LCD_RST<32))
    #define LCD_RST_LOW  GPIO.out_w1tc = (1 << LCD_RST)
    #define LCD_RST_HIGH GPIO.out_w1ts = (1 << LCD_RST)
#elif (LCD_RST>=32)
    #define LCD_RST_LOW  GPIO.out1_w1tc.val = (1 << (LCD_RST - 32))   
    #define LCD_RST_HIGH GPIO.out1_w1ts.val = (1 << (LCD_RST - 32))
#endif

#if ((LCD_DC>=0) && (LCD_DC<32))
    #define LCD_DC_LOW  GPIO.out_w1tc = (1 << LCD_DC)
    #define LCD_DC_HIGH GPIO.out_w1ts = (1 << LCD_DC)
#elif (LCD_DC>=32)
    #define LCD_DC_LOW  GPIO.out1_w1tc.val = (1 << (LCD_DC - 32))
    #define LCD_DC_HIGH GPIO.out1_w1ts.val = (1 << (LCD_DC - 32))
#endif

#if ((LCD_BL>=0) && (LCD_BL<32))
    #define LCD_BL_LOW  GPIO.out_w1tc = (1 << LCD_BL)
    #define LCD_BL_HIGH GPIO.out_w1ts = (1 << LCD_BL)
#elif (LCD_BL>=32)
    #define LCD_BL_LOW  GPIO.out1_w1tc.val = (1 << (LCD_BL - 32))
    #define LCD_BL_HIGH GPIO.out1_w1ts.val = (1 << (LCD_BL - 32))
#endif

#define SPI_WRITE_BITS(D, L) *spi_write_len = L-1;\
                             *spi_write_buf = D;\
                             *spi_cmd = SPI_UPDATE;\
                             while(*spi_cmd & SPI_UPDATE);\
                             *spi_cmd = SPI_USR;\
                             while(*spi_cmd & SPI_USR);

#define spi_write_8bit(D) SPI_WRITE_BITS(D, 8)

#define spi_write_16bit(D) SPI_WRITE_BITS((D)<<8|(D)>>8, 16)

#define spi_write_16Bit_nw(D)  *spi_write_len = 16-1;\
                             *spi_write_buf = ((D)<<8|(D)>>8);\
                             *spi_cmd = SPI_UPDATE;\
                             while(*spi_cmd & SPI_UPDATE);\
                             *spi_cmd = SPI_USR;

#define SET_SPI_WRITE_MODE *spi_usr = SPI_USR_MOSI
#define SET_SPI_READ_MODE  *spi_usr = SPI_USR_MOSI | SPI_USR_MISO | SPI_DOUTDIN

#endif
