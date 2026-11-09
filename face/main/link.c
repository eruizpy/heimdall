#include "link.h"

#include <driver/uart.h>
#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <cJSON.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

#include "board_pins.h"
#include "display.h"
#include "face_ui.h"
#include "touch.h"

#define TAG "link"
#define MAX_LINE 256
#define CORE_UART UART_NUM_2
#define USB_UART UART_NUM_0

static SemaphoreHandle_t s_tx_mutex;

void link_send(const char* json) {
    if (s_tx_mutex == NULL) {
        return;
    }
    xSemaphoreTake(s_tx_mutex, portMAX_DELAY);
    size_t len = strlen(json);
    uart_write_bytes(CORE_UART, json, len);
    uart_write_bytes(CORE_UART, "\n", 1);
    uart_write_bytes(USB_UART, json, len);
    uart_write_bytes(USB_UART, "\n", 1);
    xSemaphoreGive(s_tx_mutex);
}

static void on_face_event(const char* ev, int x, int y) {
    int rx, ry;
    touch_get_last_raw(&rx, &ry);
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"t\":\"touch\",\"ev\":\"%s\",\"x\":%d,\"y\":%d,\"raw\":[%d,%d]}",
             ev, x, y, rx, ry);
    link_send(buf);
}

static const char* str_field(const cJSON* root, const char* key) {
    const cJSON* item = cJSON_GetObjectItem(root, key);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static double num_field(const cJSON* root, const char* key, double fallback) {
    const cJSON* item = cJSON_GetObjectItem(root, key);
    return cJSON_IsNumber(item) ? item->valuedouble : fallback;
}

static void handle_line(const char* line) {
    cJSON* root = cJSON_Parse(line);
    if (root == NULL) {
        return;  // logs or noise
    }
    const char* t = str_field(root, "t");
    if (t == NULL) {
        cJSON_Delete(root);
        return;
    }

    if (strcmp(t, "ping") == 0) {
        link_send("{\"t\":\"pong\"}");
    } else if (strcmp(t, "calibrate") == 0) {
        touch_start_calibration();
    } else if (strcmp(t, "bright") == 0) {
        display_set_brightness((int)num_field(root, "v", 80));
    } else if (lvgl_port_lock(0)) {
        if (strcmp(t, "emo") == 0) {
            face_ui_set_emotion(str_field(root, "v"));
        } else if (strcmp(t, "status") == 0) {
            face_ui_set_status(str_field(root, "v"));
        } else if (strcmp(t, "text") == 0) {
            face_ui_set_text(str_field(root, "role"), str_field(root, "v"));
        } else if (strcmp(t, "look") == 0) {
            face_ui_look((float)num_field(root, "x", 0), (float)num_field(root, "y", 0));
        } else if (strcmp(t, "blink") == 0) {
            face_ui_blink();
        }
        lvgl_port_unlock();
    }
    cJSON_Delete(root);
}

static void reader_task(void* arg) {
    uart_port_t port = (uart_port_t)(intptr_t)arg;
    char line[MAX_LINE];
    size_t len = 0;
    bool overflow = false;
    uint8_t buf[64];
    while (true) {
        // Block for one byte, then drain what is buffered (a full-size blocking read
        // would wait until the buffer fills).
        int n = uart_read_bytes(port, buf, 1, portMAX_DELAY);
        if (n > 0) {
            int more = uart_read_bytes(port, buf + 1, sizeof(buf) - 1, 0);
            n += more > 0 ? more : 0;
        }
        for (int i = 0; i < n; i++) {
            char c = (char)buf[i];
            if (c == '\n' || c == '\r') {
                if (len > 0 && !overflow) {
                    line[len] = '\0';
                    handle_line(line);
                }
                len = 0;
                overflow = false;
            } else if (len < MAX_LINE - 1) {
                line[len++] = c;
            } else {
                overflow = true;  // drop over-long lines
            }
        }
    }
}

void link_init(void) {
    s_tx_mutex = xSemaphoreCreateMutex();

    const uart_config_t cfg = {
        .baud_rate = LINK_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(CORE_UART, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CORE_UART, &cfg));
    ESP_ERROR_CHECK(
        uart_set_pin(CORE_UART, PIN_LINK_TX, PIN_LINK_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // UART0 is also the log console; installing the driver only adds buffered RX.
    ESP_ERROR_CHECK(uart_driver_install(USB_UART, 1024, 0, 0, NULL, 0));

    face_ui_set_event_cb(on_face_event);
    xTaskCreate(reader_task, "link_core", 4096, (void*)(intptr_t)CORE_UART, 5, NULL);
    xTaskCreate(reader_task, "link_usb", 4096, (void*)(intptr_t)USB_UART, 5, NULL);
}
