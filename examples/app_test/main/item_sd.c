/*
 * microSD: bsp_sdcard_mount() in a task (never formats), then the FAT size is shown.
 * The card is unmounted when leaving the page.
 */
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

static test_row_t s_row;
static volatile int s_result;    /* 0 running, 1 mounted, -1 failed */
static volatile esp_err_t s_err;

static void mount_task(void *arg)
{
    s_err = bsp_sdcard_mount();
    s_result = s_err == ESP_OK ? 1 : -1;
    vTaskDelete(NULL);
}

static void build(lv_obj_t *body)
{
    test_ui_row(body, "microSD", &s_row);
    test_ui_set(&s_row, TEST_RUNNING, "Mounting...");
    s_result = 0;
    xTaskCreate(mount_task, "sd_test", 4096, NULL, 3, NULL);
}

static void poll(void)
{
    if (s_result == 0 || s_row.state != TEST_RUNNING) {
        return;
    }
    if (s_result < 0) {
        test_ui_set(&s_row, TEST_FAIL, "No card or not FAT (%s)", esp_err_to_name(s_err));
        return;
    }
    uint64_t total = 0, free = 0;
    esp_vfs_fat_info(CONFIG_BSP_SD_MOUNT_POINT, &total, &free);
    test_ui_set(&s_row, TEST_PASS, "%.1f GB, %.1f GB free, mounted at %s", total / 1e9, free / 1e9,
                CONFIG_BSP_SD_MOUNT_POINT);
}

static void stop(void)
{
    if (s_result == 1) {
        bsp_sdcard_unmount();
    }
}

const test_item_t item_sd = {.build = build, .poll = poll, .stop = stop};
