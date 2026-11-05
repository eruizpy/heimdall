#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

// ESP32-2432S028 "CYD2USB" (micro-USB + USB-C, ST7789 2.8" 240x320)
// ESP32-WROOM-32, 4MB flash, no PSRAM.

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// External INMP441 I2S microphone (L/R -> GND, VDD -> 3.3V).
// GPIO22/27 are on connector CN1, GPIO35 (input only) on connector P3.
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_22
#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_27
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_35

// Onboard speaker amplifier (SC8002B) is driven by internal DAC channel 2 (GPIO26),
// speaker connector P4.

#define BOOT_BUTTON_GPIO        GPIO_NUM_0

// Onboard RGB LED, active low. Not used yet; kept off.
#define RGB_LED_R_GPIO          GPIO_NUM_4
#define RGB_LED_G_GPIO          GPIO_NUM_16
#define RGB_LED_B_GPIO          GPIO_NUM_17

// Display: ST7789 on HSPI (SPI2_HOST). LCD reset is tied to EN.
#define DISPLAY_SPI_HOST        SPI2_HOST
#define DISPLAY_MOSI_PIN        GPIO_NUM_13
#define DISPLAY_MISO_PIN        GPIO_NUM_12
#define DISPLAY_CLK_PIN         GPIO_NUM_14
#define DISPLAY_CS_PIN          GPIO_NUM_15
#define DISPLAY_DC_PIN          GPIO_NUM_2
#define DISPLAY_RST_PIN         GPIO_NUM_NC
#define DISPLAY_BACKLIGHT_PIN   GPIO_NUM_21
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

// Landscape 320x240. Orientation/color values need confirmation on hardware.
#define DISPLAY_WIDTH           320
#define DISPLAY_HEIGHT          240
#define DISPLAY_SWAP_XY         true
#define DISPLAY_MIRROR_X        true
#define DISPLAY_MIRROR_Y        false
#define DISPLAY_INVERT_COLOR    true
#define DISPLAY_RGB_ORDER       LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0
#define DISPLAY_SPI_MODE        0
#define DISPLAY_SPI_CLOCK_HZ    (40 * 1000 * 1000)

// Resistive touch XPT2046 (not used yet): CLK 25, MOSI 32, MISO 39, CS 33, IRQ 36.

#endif  // _BOARD_CONFIG_H_
