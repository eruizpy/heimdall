#include "link.h"

#include <driver/uart.h>
#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

#include "board_pins.h"
#include "display.h"
#include "face_proto.h"
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
    if (face_proto_format_touch(buf, sizeof(buf), ev, x, y, rx, ry) > 0) {
        link_send(buf);
    }
}

// The command struct (~240 bytes) lives in static storage: each reader task handles one
// line at a time, and the two readers are serialized by this mutex.
static SemaphoreHandle_t s_dispatch_mutex;

static void handle_line(const char* line) {
    static face_cmd_t cmd;
    xSemaphoreTake(s_dispatch_mutex, portMAX_DELAY);
    if (face_proto_parse(line, &cmd)) {  // logs, noise and unknown commands are ignored
        switch (cmd.type) {
            case FACE_CMD_PING:
                link_send("{\"t\":\"pong\"}");
                break;
            case FACE_CMD_CALIBRATE:
                touch_start_calibration();
                break;
            case FACE_CMD_BRIGHT:
                display_set_brightness(cmd.value);
                break;
            default:
                if (lvgl_port_lock(0)) {
                    if (cmd.type == FACE_CMD_EMO) {
                        face_ui_set_emotion(cmd.text);
                    } else if (cmd.type == FACE_CMD_STATUS) {
                        face_ui_set_status(cmd.text);
                    } else if (cmd.type == FACE_CMD_TEXT) {
                        face_ui_set_text(cmd.role, cmd.text);
                    } else if (cmd.type == FACE_CMD_LOOK) {
                        face_ui_look(cmd.x, cmd.y);
                    } else if (cmd.type == FACE_CMD_BLINK) {
                        face_ui_blink();
                    }
                    lvgl_port_unlock();
                }
                break;
        }
    }
    xSemaphoreGive(s_dispatch_mutex);
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
    s_dispatch_mutex = xSemaphoreCreateMutex();

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
