/*
 * Wi-Fi (ESP32-C5 over ESP-Hosted): bsp_wifi_init() once, then a scan. PASS when at least one
 * access point is found. "Scan" runs it again.
 */
#include <string.h>
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

static test_row_t s_row;
static volatile int s_result;       /* 0 running, 1 done, -1 failed */
static volatile int s_count;
static char s_best[33];
static volatile int s_best_rssi;
static volatile bool s_busy;
static SemaphoreHandle_t s_done;

static void on_scan_done(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    xSemaphoreGive(s_done);
}

static void scan_task(void *arg)
{
    static bool ready;
    s_result = -1;
    if (!ready) {
        if (bsp_wifi_init() != ESP_OK) {
            goto out;
        }
        s_done = xSemaphoreCreateBinary();
        esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, on_scan_done, NULL);
        ready = true;
    }
    /* Non-blocking scan: a blocking one starves the other RPC calls to the C5 */
    if (esp_wifi_scan_start(NULL, false) != ESP_OK || xSemaphoreTake(s_done, pdMS_TO_TICKS(20000)) != pdTRUE) {
        goto out;
    }
    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    s_count = n;
    s_best[0] = 0;
    s_best_rssi = 0;
    if (n) {
        wifi_ap_record_t *aps = calloc(n, sizeof(*aps));
        if (aps && esp_wifi_scan_get_ap_records(&n, aps) == ESP_OK && n) {
            strlcpy(s_best, (const char *)aps[0].ssid, sizeof(s_best));   /* sorted by RSSI */
            s_best_rssi = aps[0].rssi;
        }
        free(aps);
    }
    s_result = 1;
out:
    s_busy = false;
    vTaskDelete(NULL);
}

static void start_scan(void)
{
    if (s_busy) {
        return;
    }
    s_busy = true;
    s_result = 0;
    test_ui_set(&s_row, TEST_RUNNING, "Scanning (first run also starts the C5)...");
    xTaskCreate(scan_task, "wifi_test", 4096, NULL, 3, NULL);
}

static void on_scan(lv_event_t *e)
{
    start_scan();
}

static void build(lv_obj_t *body)
{
    test_ui_row(body, "Wi-Fi (ESP32-C5)", &s_row);
    test_ui_button(s_row.ctrl, LV_SYMBOL_REFRESH " Scan", on_scan, NULL);
    start_scan();
}

static void poll(void)
{
    if (s_busy || s_row.state != TEST_RUNNING) {
        return;
    }
    if (s_result < 0) {
        test_ui_set(&s_row, TEST_FAIL, "Wi-Fi start or scan failed");
    } else if (s_count == 0) {
        test_ui_set(&s_row, TEST_FAIL, "No access point found");
    } else {
        test_ui_set(&s_row, TEST_PASS, "%d APs, strongest: %s (%d dBm)", s_count, s_best[0] ? s_best : "(hidden)",
                    s_best_rssi);
    }
}

const test_item_t item_wifi = {.build = build, .poll = poll};
