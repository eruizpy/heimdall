#include "touch.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <stdlib.h>

#include "board_pins.h"
#include "face_ui.h"
#include "link.h"
#include "touch_calib.h"

#define TAG "touch"

// XPT2046 commands: 12-bit, differential, PD1:PD0 = 00 (PENIRQ stays enabled).
#define CMD_X 0xD0
#define CMD_Y 0x90
#define CMD_Z1 0xB0
#define SAMPLES 5
#define Z1_MIN 60

#define CALIB_MAGIC 0x48454931  // "HEI1"

// NVS blob; same layout as before the calibration math moved to touch_calib.c.
typedef struct {
    uint32_t magic;
    touch_calib_t cal;
} calib_blob_t;

static spi_device_handle_t s_spi;
static touch_calib_t s_calib = {200, 3700, 240, 3800, 0};  // rough CYD defaults
static bool s_has_calib;
static volatile bool s_calibrating;
static int s_last_raw_x, s_last_raw_y;
static lv_point_t s_last_point;

static int read12(uint8_t cmd) {
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA,
        .length = 24,
        .tx_data = {cmd, 0, 0},
    };
    if (spi_device_polling_transmit(s_spi, &t) != ESP_OK) {
        return 0;
    }
    return ((t.rx_data[1] << 8) | t.rx_data[2]) >> 3;
}

static int cmp_int(const void* a, const void* b) { return *(const int*)a - *(const int*)b; }

static int median_read(uint8_t cmd) {
    int v[SAMPLES];
    for (int i = 0; i < SAMPLES; i++) {
        v[i] = read12(cmd);
    }
    qsort(v, SAMPLES, sizeof(int), cmp_int);
    return (v[SAMPLES / 2 - 1] + v[SAMPLES / 2] + v[SAMPLES / 2 + 1]) / 3;
}

// Raw reading while the panel is pressed.
static bool read_raw(int* rx, int* ry) {
    if (gpio_get_level(PIN_TOUCH_IRQ) != 0) {
        return false;
    }
    if (read12(CMD_Z1) < Z1_MIN) {
        return false;
    }
    *rx = median_read(CMD_X);
    *ry = median_read(CMD_Y);
    // Still pressed after sampling? Otherwise the values may be from the release.
    return gpio_get_level(PIN_TOUCH_IRQ) == 0;
}

static void raw_to_screen(int rx, int ry, int* sx, int* sy) {
    touch_calib_map(&s_calib, rx, ry, LCD_H_RES, LCD_V_RES, sx, sy);
}

static void indev_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    int rx, ry;
    if (!s_calibrating && read_raw(&rx, &ry)) {
        int sx, sy;
        raw_to_screen(rx, ry, &sx, &sy);
        s_last_raw_x = rx;
        s_last_raw_y = ry;
        s_last_point.x = sx;
        s_last_point.y = sy;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->point = s_last_point;
}

static void load_calibration(void) {
    nvs_handle_t nvs;
    if (nvs_open("face", NVS_READONLY, &nvs) != ESP_OK) {
        return;
    }
    calib_blob_t blob;
    size_t len = sizeof(blob);
    if (nvs_get_blob(nvs, "calib", &blob, &len) == ESP_OK && len == sizeof(blob) &&
        blob.magic == CALIB_MAGIC && blob.cal.x1 != blob.cal.x0 && blob.cal.y1 != blob.cal.y0) {
        s_calib = blob.cal;
        s_has_calib = true;
    }
    nvs_close(nvs);
}

static void save_calibration(void) {
    nvs_handle_t nvs;
    if (nvs_open("face", NVS_READWRITE, &nvs) != ESP_OK) {
        return;
    }
    const calib_blob_t blob = {.magic = CALIB_MAGIC, .cal = s_calib};
    nvs_set_blob(nvs, "calib", &blob, sizeof(blob));
    nvs_commit(nvs);
    nvs_close(nvs);
}

// Waits for a press-and-release and returns the average raw position while pressed.
static void wait_tap(int* rx, int* ry) {
    while (true) {
        long sx = 0, sy = 0;
        int n = 0, x, y;
        while (!read_raw(&x, &y)) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        while (read_raw(&x, &y)) {
            sx += x;
            sy += y;
            n++;
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        if (n >= 3) {  // ignore glitches
            *rx = sx / n;
            *ry = sy / n;
            vTaskDelay(pdMS_TO_TICKS(300));
            return;
        }
    }
}

static void calibration_task(void* arg) {
    static const int targets[3][2] = {{30, 30}, {LCD_H_RES - 30, 30}, {30, LCD_V_RES - 30}};
    int raw[3][2];
    touch_calib_t c;

    s_calibrating = true;
    do {  // repeat until the three taps give a usable calibration
        for (int i = 0; i < 3; i++) {
            if (lvgl_port_lock(0)) {
                face_ui_show_calibration_target(targets[i][0], targets[i][1], i + 1, 3);
                lvgl_port_unlock();
            }
            wait_tap(&raw[i][0], &raw[i][1]);
        }
    } while (!touch_calib_compute(targets, raw, LCD_H_RES, LCD_V_RES, &c));

    s_calib = c;
    s_has_calib = true;
    save_calibration();
    ESP_LOGW(TAG, "calibration: swap=%d x %d..%d y %d..%d", c.swap, c.x0, c.x1, c.y0, c.y1);

    if (lvgl_port_lock(0)) {
        face_ui_hide_calibration();
        lvgl_port_unlock();
    }
    s_calibrating = false;
    link_send("{\"t\":\"calibrated\"}");
    vTaskDelete(NULL);
}

void touch_start_calibration(void) {
    if (!s_calibrating) {
        s_calibrating = true;
        xTaskCreate(calibration_task, "calib", 3072, NULL, 3, NULL);
    }
}

bool touch_has_calibration(void) { return s_has_calib; }

void touch_get_last_raw(int* raw_x, int* raw_y) {
    *raw_x = s_last_raw_x;
    *raw_y = s_last_raw_y;
}

void touch_init(lv_display_t* disp) {
    const gpio_config_t irq = {
        .pin_bit_mask = 1ULL << PIN_TOUCH_IRQ,
        .mode = GPIO_MODE_INPUT,  // GPIO36: input only, external pull-up on the board
    };
    ESP_ERROR_CHECK(gpio_config(&irq));

    const spi_bus_config_t bus = {
        .mosi_io_num = PIN_TOUCH_MOSI,
        .miso_io_num = PIN_TOUCH_MISO,
        .sclk_io_num = PIN_TOUCH_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_DISABLED));
    const spi_device_interface_config_t dev = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_TOUCH_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &dev, &s_spi));
    read12(CMD_Y);  // leaves the controller powered down with PENIRQ enabled

    load_calibration();

    if (lvgl_port_lock(0)) {
        lv_indev_t* indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, indev_read_cb);
        lv_indev_set_display(indev, disp);
        lvgl_port_unlock();
    }
}
