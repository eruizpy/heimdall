#pragma once

// 3-point touch calibration (spec 002-robot-face, RF5). Pure logic.

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t x0, x1;  // raw value at screen x = 0 and x = width
    int16_t y0, y1;  // raw value at screen y = 0 and y = height
    uint8_t swap;    // raw x/y swapped relative to the screen
} touch_calib_t;

// targets: screen points tapped in order (top-left, top-right, bottom-left);
// raw: controller readings for each. Returns false for degenerate input.
bool touch_calib_compute(const int targets[3][2], const int raw[3][2], int width, int height,
                         touch_calib_t* out);

// Raw reading -> screen coordinates, clamped to the screen.
void touch_calib_map(const touch_calib_t* calib, int raw_x, int raw_y, int width, int height,
                     int* x, int* y);
