// Heimdall face: the ESP32-2432S028 (CYD2USB) as the robot's face and touch input.
// Protocol: docs/heimdall/face-protocol.md

#include <esp_log.h>
#include <esp_lvgl_port.h>
#include <nvs_flash.h>

#include "display.h"
#include "face_ui.h"
#include "link.h"
#include "touch.h"

void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    lv_display_t* disp = display_init();
    if (lvgl_port_lock(0)) {
        face_ui_create();
        lvgl_port_unlock();
    }
    touch_init(disp);
    display_set_brightness(80);

    link_init();
    // Keep the USB console readable: protocol lines start with '{', logs are warnings only.
    esp_log_level_set("*", ESP_LOG_WARN);
    link_send("{\"t\":\"hello\",\"fw\":\"heimdall-face 0.1\"}");

    if (!touch_has_calibration()) {
        touch_start_calibration();
    }
}
