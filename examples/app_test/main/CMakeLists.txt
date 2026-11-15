idf_component_register(
    SRCS "main.c" "test_ui.c" "test_menu.c" "test_auto.c" "test_color.c" "test_touch.c"
         "item_wifi.c" "item_audio.c" "item_imu.c" "item_sd.c" "item_camera.c" "item_power.c" "item_buttons.c" "item_led.c" "item_modem.c"
    INCLUDE_DIRS "."
    REQUIRES esp32p4c5_bsp esp_timer fatfs esp_wifi esp_event esp_app_format)
