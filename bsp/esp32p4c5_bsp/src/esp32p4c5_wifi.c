/*
 * Wi-Fi for the KSDIY ESP32-P4C5 board. The radio is the ESP32-C5 co-processor (inside the module),
 * reached over SDIO slot 1 through ESP-Hosted. esp_hosted resets the C5 on GPIO54 and uses
 * CLK 18, CMD 19, D0-D3 14-17 (the defaults for ESP32-P4 in esp_hosted's Kconfig).
 * The host-side configuration (SDIO transport, WIFI_REMOTE_LIBRARY_HOSTED) lives in the example's
 * sdkconfig.defaults, not in this BSP.
 */
#include <stdbool.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include "esp_hosted.h"
#include "bsp/esp32p4c5_bsp.h"
#include "bsp_err_check.h"

static const char *TAG = "esp32p4c5_wifi";
static bool s_wifi_ready;

esp_err_t bsp_wifi_init(void)
{
    if (s_wifi_ready) {
        return ESP_OK;
    }

    /* NVS is used by the Wi-Fi driver for calibration and config */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "NVS erase failed");
        err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, TAG, "NVS init failed");

    err = esp_netif_init();
    ESP_RETURN_ON_ERROR(err, TAG, "netif init failed");

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "event loop create failed: %s", esp_err_to_name(err));
        return err;
    }

    if (esp_netif_create_default_wifi_sta() == NULL) {
        ESP_LOGE(TAG, "create default STA netif failed");
        return ESP_FAIL;
    }

    /* Bring the SDIO link up first: resets the C5 and waits for its INIT event */
    ESP_RETURN_ON_FALSE(esp_hosted_connect_to_slave() == ESP_OK, ESP_FAIL, TAG, "C5 not reachable over SDIO");

    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "esp_wifi_init failed (C5 not reachable?)");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set STA mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start failed");

    s_wifi_ready = true;
    ESP_LOGI(TAG, "Wi-Fi STA ready (C5 over SDIO)");
    return ESP_OK;
}
