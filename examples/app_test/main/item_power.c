/*
 * PMIC (AXP2101): battery state and level from the fuel gauge, USB supply, system rail,
 * die temperature and charger settings, read with bsp_battery_get_info() every second.
 */
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

static test_row_t s_row;
static int s_tick;

static void build(lv_obj_t *body)
{
    test_ui_row(body, "PMIC / battery", &s_row);
    test_ui_set(&s_row, TEST_RUNNING, "Reading AXP2101...");
    s_tick = 0;
}

static void poll(void)
{
    if (s_tick++ % 3) {
        return;
    }
    static const char *const state[] = {"no battery", "discharging", "charging", "full"};
    bsp_battery_info_t b;
    if (bsp_battery_get_info(&b) != ESP_OK) {
        test_ui_set(&s_row, TEST_FAIL, "AXP2101 does not answer");
        return;
    }
    test_ui_set(&s_row, TEST_PASS,
                "Battery: %s, %d%%, %.2f V\nUSB: %s %.2f V  |  VSYS %.2f V  |  %.1f C\nCharger: %u mA, %.2f V",
                state[b.state], b.percent, b.voltage_mv / 1000.0, b.vbus_present ? "yes" : "no", b.vbus_mv / 1000.0,
                b.vsys_mv / 1000.0, b.die_temp_c, b.charge_current_ma, b.charge_voltage_mv / 1000.0);
}

const test_item_t item_power = {.build = build, .poll = poll};
