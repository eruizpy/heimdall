// Spec 002-robot-face, CA4: 3-point touch calibration.
#include "touch_calib.h"
#include "unity.h"

#define W 320
#define H 240

static const int kTargets[3][2] = {{30, 30}, {W - 30, 30}, {30, H - 30}};

void setUp(void) {}
void tearDown(void) {}

// Simulated panels: how the controller reports a screen point.
typedef void (*panel_fn)(int sx, int sy, int* rx, int* ry);

static void panel_straight(int sx, int sy, int* rx, int* ry) {
    *rx = 200 + sx * 10;
    *ry = 300 + sy * 14;
}

static void panel_swapped_inverted(int sx, int sy, int* rx, int* ry) {
    // Like this CYD: raw X follows screen Y (inverted), raw Y follows screen X.
    *rx = 3800 - sy * 14;
    *ry = 250 + sx * 11;
}

static void panel_inverted_x(int sx, int sy, int* rx, int* ry) {
    *rx = 3700 - sx * 10;
    *ry = 300 + sy * 14;
}

static touch_calib_t calibrate(panel_fn panel) {
    int raw[3][2];
    for (int i = 0; i < 3; i++) {
        panel(kTargets[i][0], kTargets[i][1], &raw[i][0], &raw[i][1]);
    }
    touch_calib_t c;
    TEST_ASSERT_TRUE(touch_calib_compute(kTargets, raw, W, H, &c));
    return c;
}

static void assert_maps(panel_fn panel, const touch_calib_t* c, int sx, int sy) {
    int rx, ry, x, y;
    panel(sx, sy, &rx, &ry);
    touch_calib_map(c, rx, ry, W, H, &x, &y);
    TEST_ASSERT_INT_WITHIN(1, sx, x);
    TEST_ASSERT_INT_WITHIN(1, sy, y);
}

static void check_panel(panel_fn panel, int expect_swap) {
    touch_calib_t c = calibrate(panel);
    TEST_ASSERT_EQUAL_INT(expect_swap, c.swap);
    assert_maps(panel, &c, 160, 120);
    assert_maps(panel, &c, 10, 10);
    assert_maps(panel, &c, 300, 220);
    assert_maps(panel, &c, 290, 30);
}

static void test_straight_panel(void) { check_panel(panel_straight, 0); }

static void test_swapped_and_inverted_panel(void) { check_panel(panel_swapped_inverted, 1); }

static void test_inverted_x_panel(void) { check_panel(panel_inverted_x, 0); }

static void test_output_is_clamped_to_screen(void) {
    touch_calib_t c = calibrate(panel_straight);
    int x, y;
    touch_calib_map(&c, -5000, -5000, W, H, &x, &y);
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_INT(0, y);
    touch_calib_map(&c, 9000, 9000, W, H, &x, &y);
    TEST_ASSERT_EQUAL_INT(W - 1, x);
    TEST_ASSERT_EQUAL_INT(H - 1, y);
}

static void test_degenerate_points_are_rejected(void) {
    int same[3][2] = {{2000, 2000}, {2000, 2000}, {2000, 2000}};
    touch_calib_t c;
    TEST_ASSERT_FALSE(touch_calib_compute(kTargets, same, W, H, &c));

    // Second tap moved along screen X but third tap did not move along Y.
    int flat[3][2] = {{500, 500}, {3500, 500}, {500, 500}};
    TEST_ASSERT_FALSE(touch_calib_compute(kTargets, flat, W, H, &c));
}

static void test_map_with_degenerate_calibration_does_not_crash(void) {
    touch_calib_t zero = {0};
    int x = -1, y = -1;
    touch_calib_map(&zero, 1000, 1000, W, H, &x, &y);
    TEST_ASSERT_TRUE(x >= 0 && x < W);
    TEST_ASSERT_TRUE(y >= 0 && y < H);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_straight_panel);
    RUN_TEST(test_swapped_and_inverted_panel);
    RUN_TEST(test_inverted_x_panel);
    RUN_TEST(test_output_is_clamped_to_screen);
    RUN_TEST(test_degenerate_points_are_rejected);
    RUN_TEST(test_map_with_degenerate_calibration_does_not_crash);
    return UNITY_END();
}
