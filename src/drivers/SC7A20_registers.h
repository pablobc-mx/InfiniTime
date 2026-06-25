#pragma once

// SC7A20 ±2G/±4G/±8G/±16G Three-axis micromachined digital accelerometer
// Hangzhou Silan Microelectronics Co., Ltd. – LIS2DH clone

// Registers
#define OUT_TEMP_L    0x0C
#define OUT_TEMP_H    0x0D
#define WHO_AM_I      0x0F
#define USER_CAL      0x13
#define CTRL_REG1     0x20
#define CTRL_REG2     0x21
#define CTRL_REG3     0x22
#define CTRL_REG4     0x23
#define CTRL_REG5     0x24
#define CTRL_REG6     0x25
#define STATUS_REG    0x27
#define OUT_X_L       0x28
#define OUT_X_H       0x29
#define OUT_Y_L       0x2A
#define OUT_Y_H       0x2B
#define OUT_Z_L       0x2C
#define OUT_Z_H       0x2D
#define FIFO_CTRL_REG 0x2E
#define FIFO_SRC_REG  0x2F
#define INT1_CFG      0x30
#define INT1_SOURCE   0x31
#define INT1_THS      0x32
#define INT1_DURATION 0x33
#define CLICK_CFG     0x38
#define CLICK_SRC     0x39
#define CLICK_THS     0x3A

// CTRL_REG1
#define CTRL_REG1_X_EN        (1 << 0)
#define CTRL_REG1_Y_EN        (1 << 1)
#define CTRL_REG1_Z_EN        (1 << 2)
#define CTRL_REG1_LP_EN       (1 << 3)
#define CTRL_REG1_ODR_1HZ     (1 << 4)
#define CTRL_REG1_ODR_10HZ    (2 << 4)
#define CTRL_REG1_ODR_25HZ    (3 << 4)
#define CTRL_REG1_ODR_50HZ    (4 << 4)
#define CTRL_REG1_ODR_100HZ   (5 << 4)
#define CTRL_REG1_ODR_200HZ   (6 << 4)
#define CTRL_REG1_ODR_400HZ   (7 << 4)

// CTRL_REG4
#define CTRL_REG4_HR  (1 << 3)
#define CTRL_REG4_FS_2G  (0 << 4)
#define CTRL_REG4_BDU (1 << 7)

// CTRL_REG5
#define CTRL_REG5_FIFO_EN (1 << 6)
#define CTRL_REG5_BOOT    (1 << 7)

// FIFO
#define FIFO_CTRL_REG_BYPASS (0 << 6)
#define FIFO_CTRL_REG_FIFO   (1 << 6)
#define FIFO_SRC_REG_FSS_MASK 0x1F