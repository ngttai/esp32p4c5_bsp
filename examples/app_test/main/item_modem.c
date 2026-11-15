/*
 * 4G modem (ML307C): bsp_modem_init() powers it and waits for "AT", then the model, firmware,
 * IMEI, SIM state and signal are read with bsp_modem_at(). PASS when the module identifies
 * itself; SIM and signal are shown for information. The modem is switched off afterwards.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

static test_row_t s_row;
static volatile bool s_busy;
static volatile int s_result;       /* 0 none, 1 pass, -1 fail */
static char s_info[200];

/* First non-empty line of an AT answer that is not the final "OK", with an optional prefix removed */
static void at_value(const char *cmd, const char *prefix, char *out, size_t out_size)
{
    char resp[160];
    strlcpy(out, "?", out_size);
    if (bsp_modem_at(cmd, resp, sizeof(resp), 2000) != ESP_OK) {
        return;
    }
    for (char *line = strtok(resp, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        if (strcmp(line, "OK") == 0 || strncmp(line, "AT", 2) == 0) {
            continue;
        }
        if (prefix && strncmp(line, prefix, strlen(prefix)) == 0) {
            line += strlen(prefix);
            while (*line == ' ') {
                line++;
            }
        }
        strlcpy(out, line, out_size);
        return;
    }
}

static void modem_task(void *arg)
{
    s_result = -1;
    if (bsp_modem_init() != ESP_OK) {
        strlcpy(s_info, "No answer to AT (the modem runs from the battery)", sizeof(s_info));
        goto out;
    }
    char model[32], fw[40], imei[24], sim[24], csq[16];
    at_value("AT+CGMM", "+CGMM:", model, sizeof(model));
    at_value("AT+CGMR", "+CGMR:", fw, sizeof(fw));
    at_value("AT+CGSN=1", "+CGSN:", imei, sizeof(imei));
    at_value("AT+CPIN?", "+CPIN:", sim, sizeof(sim));
    at_value("AT+CSQ", "+CSQ:", csq, sizeof(csq));
    snprintf(s_info, sizeof(s_info), "%s, FW %s\nIMEI %s\nSIM: %s  |  CSQ %s", model, fw, imei,
             strcmp(sim, "?") ? sim : "not ready", csq);
    s_result = strcmp(model, "?") ? 1 : -1;
    bsp_modem_deinit();
out:
    s_busy = false;
    vTaskDelete(NULL);
}

static void on_read(lv_event_t *e)
{
    if (s_busy) {
        return;
    }
    s_busy = true;
    s_result = 0;
    test_ui_set(&s_row, TEST_RUNNING, "Powering the modem, up to 15 s...");
    xTaskCreate(modem_task, "modem_test", 4096, NULL, 3, NULL);
}

static void build(lv_obj_t *body)
{
    test_ui_row(body, "4G modem", &s_row);
    test_ui_set(&s_row, TEST_IDLE, "Reads the ML307C model, firmware, IMEI, SIM and signal");
    test_ui_button(s_row.ctrl, LV_SYMBOL_DOWNLOAD " Read info", on_read, NULL);
}

static void modem_poll(void)
{
    if (s_busy || s_result == 0 || s_row.state != TEST_RUNNING) {
        return;
    }
    test_ui_set(&s_row, s_result > 0 ? TEST_PASS : TEST_FAIL, "%s", s_info);
}

const test_item_t item_modem = {.build = build, .poll = modem_poll};
