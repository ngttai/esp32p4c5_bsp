/*
 * microSD card (SDMMC slot 0, 4-bit) for the KSDIY ESP32-P4C5 board.
 * Pins are the fixed IO MUX pins of slot 0 (D0-3 = 39-42, CLK = 43, CMD = 44).
 * The card is powered from on-chip LDO channel 4 (ESP_LDO_VO4).
 * Ported from the esp-bsp ESP32-P4 Function EV Board BSP.
 */
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "bsp/esp32p4c5_bsp.h"
#include "bsp_err_check.h"

#define BSP_SD_LDO_CHAN 4

static const char *TAG = "esp32p4c5_sd";
static sdmmc_card_t *s_card;
static sd_pwr_ctrl_handle_t s_pwr_ctrl;
static bool s_mounted;

esp_err_t bsp_sdcard_mount(void)
{
    if (s_mounted) {
        return ESP_OK;
    }

    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = BSP_SD_LDO_CHAN,
    };
    ESP_RETURN_ON_ERROR(sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &s_pwr_ctrl), TAG,
                        "create on-chip LDO power control failed");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    host.pwr_ctrl_handle = s_pwr_ctrl;

    sdmmc_slot_config_t slot = {0};
    slot.cd = SDMMC_SLOT_NO_CD;
    slot.wp = SDMMC_SLOT_NO_WP;
    slot.width = 4;

    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 64 * 1024,
    };

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(CONFIG_BSP_SD_MOUNT_POINT, &host, &slot,
                                            &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "mount failed: %s", esp_err_to_name(ret));
        sd_pwr_ctrl_del_on_chip_ldo(s_pwr_ctrl);
        s_pwr_ctrl = NULL;
        return ret;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "mounted at %s", CONFIG_BSP_SD_MOUNT_POINT);
    return ESP_OK;
}

esp_err_t bsp_sdcard_unmount(void)
{
    if (!s_mounted) {
        return ESP_OK;
    }
    BSP_ERROR_CHECK_RETURN_ERR(esp_vfs_fat_sdcard_unmount(CONFIG_BSP_SD_MOUNT_POINT, s_card));
    s_card = NULL;
    if (s_pwr_ctrl) {
        BSP_ERROR_CHECK_RETURN_ERR(sd_pwr_ctrl_del_on_chip_ldo(s_pwr_ctrl));
        s_pwr_ctrl = NULL;
    }
    s_mounted = false;
    return ESP_OK;
}
