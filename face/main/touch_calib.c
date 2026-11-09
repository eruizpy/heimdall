#include "touch_calib.h"

#include <stdlib.h>

#define MIN_SPAN 50  // raw units between calibration taps; less means the taps did not register

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

bool touch_calib_compute(const int targets[3][2], const int raw_in[3][2], int width, int height,
                         touch_calib_t* out) {
    int raw[3][2];
    // Moving along the screen x axis (target 0 -> 1): whichever raw axis changes more is x.
    int dxr = abs(raw_in[1][0] - raw_in[0][0]);
    int dyr = abs(raw_in[1][1] - raw_in[0][1]);
    bool swap = dyr > dxr;
    for (int i = 0; i < 3; i++) {
        raw[i][0] = swap ? raw_in[i][1] : raw_in[i][0];
        raw[i][1] = swap ? raw_in[i][0] : raw_in[i][1];
    }
    int span_x = raw[1][0] - raw[0][0];
    int span_y = raw[2][1] - raw[0][1];
    int dx_screen = targets[1][0] - targets[0][0];
    int dy_screen = targets[2][1] - targets[0][1];
    if (abs(span_x) < MIN_SPAN || abs(span_y) < MIN_SPAN || dx_screen == 0 || dy_screen == 0) {
        return false;
    }

    float sx = (float)span_x / dx_screen;
    float sy = (float)span_y / dy_screen;
    out->swap = swap;
    out->x0 = (int16_t)(raw[0][0] - targets[0][0] * sx);
    out->x1 = (int16_t)(out->x0 + width * sx);
    out->y0 = (int16_t)(raw[0][1] - targets[0][1] * sy);
    out->y1 = (int16_t)(out->y0 + height * sy);
    return true;
}

void touch_calib_map(const touch_calib_t* c, int rx, int ry, int width, int height, int* x,
                     int* y) {
    if (c->swap) {
        int t = rx;
        rx = ry;
        ry = t;
    }
    int span_x = c->x1 - c->x0;
    int span_y = c->y1 - c->y0;
    *x = span_x == 0 ? width / 2 : clampi((rx - c->x0) * width / span_x, 0, width - 1);
    *y = span_y == 0 ? height / 2 : clampi((ry - c->y0) * height / span_y, 0, height - 1);
}
