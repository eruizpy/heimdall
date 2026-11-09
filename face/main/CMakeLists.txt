idf_component_register(
    SRCS "main.c" "display.c" "touch.c" "face_ui.c" "link.c" "face_proto.c" "face_expr.c" "touch_calib.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_spi esp_driver_gpio esp_driver_uart esp_driver_ledc esp_lcd nvs_flash esp_timer
)
