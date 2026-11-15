/*
 * Hardware test app for the KSDIY ESP32-P4C5 board.
 *
 * Menu: automatic test (one row per part with PASS / FAIL), screen colors, touch grid.
 * Each part of the automatic test lives in its own item_*.c file and only uses the BSP API,
 * so the files also show how to use each part of the board.
 */
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

static const char *TAG = "app_test";

volatile bool g_power_key_pressed;

static void on_pmic_event(bsp_pmic_event_t event, void *arg)
{
    if (event == BSP_PMIC_EVENT_KEY_SHORT_PRESS) {
        g_power_key_pressed = true;
    }
}

void app_main(void)
{
    if (bsp_display_start() == NULL) {
        ESP_LOGE(TAG, "display start failed");
        return;
    }
    if (bsp_pmic_event_register(on_pmic_event, NULL) != ESP_OK) {
        ESP_LOGW(TAG, "PMIC events unavailable");
    }

    bsp_display_lock(0);
    test_menu_show();
    bsp_display_unlock();
    bsp_display_backlight_on();
    ESP_LOGI(TAG, "app test ready");
}
