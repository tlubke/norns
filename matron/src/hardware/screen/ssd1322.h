#pragma once

#include <bcm2835.h>
#include <cairo.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include <linux/types.h>

#define SPI0_SPEED_HZ 16000000

// Pinout
// see: https://github.com/monome/norns-image/blob/main/readme-hardware.md#pinout-1
#define SSD1322_GPIO_DC    5
#define SSD1322_GPIO_RESET 6

// Commands
// see:
#define SSD1322_ENABLE_GRAY_SCALE_TABLE       0x00U
#define SSD1322_SET_COLUMN_ADDRESS            0x15U
#define SSD1322_WRITE_RAM_COMMAND             0x5CU
#define SSD1322_READ_RAM_COMMAND              0x5DU
#define SSD1322_SET_ROW_ADDRESS               0x75U
#define SSD1322_SET_DUAL_COMM_LINE_MODE       0xA0U
#define SSD1322_SET_DISPLAY_START_LINE        0xA1U
#define SSD1322_SET_DISPLAY_OFFSET            0xA2U
#define SSD1322_SET_DISPLAY_MODE_NORMAL       0xA4U
#define SSD1322_SET_DISPLAY_MODE_ALL_ON       0xA5U
#define SSD1322_SET_DISPLAY_MODE_ALL_OFF      0xA6U
#define SSD1322_SET_DISPLAY_MODE_INVERSE      0xA7U
#define SSD1322_ENABLE_PARTIAL_DISPLAY        0xA8U
#define SSD1322_EXIT_PARTIAL_DISPLAY          0xA9U
#define SSD1322_SET_VDD_REGULATOR             0xABU
#define SSD1322_SET_DISPLAY_ON                0xAEU
#define SSD1322_SET_DISPLAY_OFF               0xAFU
#define SSD1322_SET_PHASE_LENGTH              0xB1U
#define SSD1322_SET_OSCILLATOR_FREQUENCY      0xB3U
#define SSD1322_SET_DISPLAY_ENHANCEMENT_A     0xB4U
#define SSD1322_SET_GPIO                      0xB5U
#define SSD1322_SET_SECOND_PRECHARGE_PERIOD   0xB6U
#define SSD1322_SET_GRAY_SCALE_TABLE          0xB8U
#define SSD1322_SET_DEFAULT_LINEAR_GRAY_SCALE 0xB9U
#define SSD1322_SET_PRECHARGE_VOLTAGE         0xBBU
#define SSD1322_SET_VCOMH_VOLTAGE             0xBEU
#define SSD1322_SET_CONTRAST_CURRENT          0xC1U
#define SSD1322_MASTER_CURRENT_CONTROL        0xC7U
#define SSD1322_SET_MULTIPLEX_RATIO           0xCAU
#define SSD1322_SET_DISPLAY_ENHANCEMENT_B     0xD1U
#define SSD1322_SET_COMMAND_LOCK              0xFDU

void ssd1322_init();
void ssd1322_end();
void ssd1322_update(char * buf, uint32_t buflen);
void ssd1322_set_gamma(double g);
void ssd1322_set_brightness(int b);
void ssd1322_set_contrast(int c);

