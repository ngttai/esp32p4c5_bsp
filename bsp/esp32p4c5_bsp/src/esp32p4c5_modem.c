/*
 * 4G modem (ML307C, AT firmware) for the KSDIY ESP32-P4C5 board.
 * UART: P4 TX GPIO53, P4 RX GPIO52 (level-shifted), 115200 8N1. Power: GPIO4 switches the
 * battery rail to the module (BSP_POWER_MODEM). DTR (GPIO51) is not driven.
 */
#include <string.h>
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp32p4c5_bsp.h"

static const char *TAG = "esp32p4c5_modem";

#define BSP_MODEM_UART          ((uart_port_t)CONFIG_BSP_MODEM_UART_NUM)
#define BSP_MODEM_BAUD          (115200)
#define BSP_MODEM_RX_BUF        (2048)
#define BSP_MODEM_BOOT_MS       (15000)     /* give up waiting for the first "OK" after power-on */
#define BSP_MODEM_PROBE_MS      (500)

static bool s_uart_ready;
static bool s_modem_ready;

static esp_err_t modem_uart_init(void)
{
    if (s_uart_ready) {
        return ESP_OK;
    }
    const uart_config_t cfg = {
        .baud_rate = BSP_MODEM_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_RETURN_ON_ERROR(uart_driver_install(BSP_MODEM_UART, BSP_MODEM_RX_BUF, 0, 0, NULL, 0), TAG, "UART install failed");
    ESP_RETURN_ON_ERROR(uart_param_config(BSP_MODEM_UART, &cfg), TAG, "UART config failed");
    ESP_RETURN_ON_ERROR(uart_set_pin(BSP_MODEM_UART, BSP_MODEM_TX, BSP_MODEM_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
                        TAG, "UART pins failed");
    s_uart_ready = true;
    return ESP_OK;
}

esp_err_t bsp_modem_at(const char *cmd, char *resp, size_t resp_size, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(cmd && resp && resp_size > 1, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(s_uart_ready, ESP_ERR_INVALID_STATE, TAG, "modem not initialized");

    uart_flush_input(BSP_MODEM_UART);
    uart_write_bytes(BSP_MODEM_UART, cmd, strlen(cmd));
    uart_write_bytes(BSP_MODEM_UART, "\r\n", 2);

    size_t len = 0;
    resp[0] = '\0';
    const int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    while (esp_timer_get_time() < deadline && len < resp_size - 1) {
        const int n = uart_read_bytes(BSP_MODEM_UART, resp + len, resp_size - 1 - len, pdMS_TO_TICKS(20));
        if (n <= 0) {
            continue;
        }
        len += n;
        resp[len] = '\0';
        if (strstr(resp, "\r\nOK\r\n")) {
            return ESP_OK;
        }
        if (strstr(resp, "ERROR")) {
            return ESP_FAIL;
        }
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t bsp_modem_init(void)
{
    if (s_modem_ready) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(modem_uart_init(), TAG, "UART init failed");
    ESP_RETURN_ON_ERROR(bsp_power_set(BSP_POWER_MODEM, true), TAG, "modem power failed");

    /* The module answers a few seconds after power-on: probe with "AT" until it does */
    char resp[64];
    const int64_t deadline = esp_timer_get_time() + (int64_t)BSP_MODEM_BOOT_MS * 1000;
    while (esp_timer_get_time() < deadline) {
        if (bsp_modem_at("AT", resp, sizeof(resp), BSP_MODEM_PROBE_MS) == ESP_OK) {
            (void)bsp_modem_at("ATE0", resp, sizeof(resp), BSP_MODEM_PROBE_MS);     /* no command echo */
            s_modem_ready = true;
            ESP_LOGI(TAG, "modem ready");
            return ESP_OK;
        }
    }
    ESP_LOGE(TAG, "modem does not answer (no battery? the module runs from the battery rail)");
    (void)bsp_power_set(BSP_POWER_MODEM, false);
    return ESP_ERR_TIMEOUT;
}

esp_err_t bsp_modem_deinit(void)
{
    s_modem_ready = false;
    if (s_uart_ready) {
        (void)uart_driver_delete(BSP_MODEM_UART);
        s_uart_ready = false;
    }
    return bsp_power_set(BSP_POWER_MODEM, false);
}
