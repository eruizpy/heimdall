#pragma once

// ESP32-2432S028 "CYD2USB" (ST7789 + XPT2046). Same pins as main/boards/esp32-2432s028.

#define LCD_H_RES 320
#define LCD_V_RES 240

#define PIN_LCD_MOSI 13
#define PIN_LCD_MISO 12
#define PIN_LCD_SCLK 14
#define PIN_LCD_CS 15
#define PIN_LCD_DC 2
#define PIN_LCD_BL 21

#define PIN_TOUCH_SCLK 25
#define PIN_TOUCH_MOSI 32
#define PIN_TOUCH_MISO 39
#define PIN_TOUCH_CS 33
#define PIN_TOUCH_IRQ 36

// Link to the robot core on connector CN1.
#define PIN_LINK_TX 27
#define PIN_LINK_RX 22
#define LINK_BAUD 115200
