#pragma once

#include "lvgl.h"

// ST7789 panel + backlight + LVGL port. Returns the LVGL display.
lv_display_t* display_init(void);

// Backlight 0-100 %.
void display_set_brightness(int percent);
