/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

//ESP32-S3 IO Pin define
#define AP_ENABLE 1
#define SD_SCK  38
#define SD_CMD 40
#define SD_D0 39
#define SD_D1 41
#define SD_D2 48
#define SD_D3 47

//I2S IO Pin define
#define I2S_MCK   4
#define I2S_BCK   5
#define I2S_DINT  6
#define I2S_DOUT  8
#define I2S_WS    7
#define I2S_NUM   I2S_NUM_1

#define I2C_SCL           15       /*!< GPIO number used for I2C master clock */
#define I2C_SDA           16       /*!< GPIO number used for I2C master data  */
#define I2C_NUM           I2C_NUM_0       /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_SPEED         400000          /*!< I2C master clock frequency */