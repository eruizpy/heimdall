#pragma once

#include <stdbool.h>

#include "lvgl.h"

// XPT2046 on its own SPI bus, registered as an LVGL pointer device.
void touch_init(lv_display_t* disp);

bool touch_has_calibration(void);

// Runs the 3-point calibration in the background (crosses on screen), saves it to NVS.
void touch_start_calibration(void);

// Last raw reading (for diagnostics in touch events).
void touch_get_last_raw(int* raw_x, int* raw_y);
