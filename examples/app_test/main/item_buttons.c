/*
 * Keys: BOOT and IO0 through bsp_iot_button_create(), the power key through the PMIC events
 * (main.c). PASS once all three were pressed.
 */
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

static test_row_t s_row;
static lv_obj_t *s_chip[3];
static volatile bool s_pressed[3];
static button_handle_t s_btns[BSP_BUTTON_NUM];
static bool s_created;

static void on_press(void *button, void *user)
{
    s_pressed[(intptr_t)user] = true;
}

static lv_obj_t *chip(lv_obj_t *parent, const char *text)
{
    lv_obj_t *c = lv_label_create(parent);
    lv_label_set_text(c, text);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(0x2A2F3A), 0);
    lv_obj_set_style_radius(c, 10, 0);
    lv_obj_set_style_pad_hor(c, 16, 0);
    lv_obj_set_style_pad_ver(c, 8, 0);
    return c;
}

static void build(lv_obj_t *body)
{
    test_ui_row(body, "Keys", &s_row);
    test_ui_set(&s_row, TEST_RUNNING, "Press BOOT, IO0 and the power key");
    s_chip[0] = chip(s_row.ctrl, "BOOT");
    s_chip[1] = chip(s_row.ctrl, "IO0");
    s_chip[2] = chip(s_row.ctrl, "POWER");
    s_pressed[0] = s_pressed[1] = s_pressed[2] = false;
    g_power_key_pressed = false;
    if (!s_created) {
        if (bsp_iot_button_create(s_btns, NULL, BSP_BUTTON_NUM) == ESP_OK) {
            iot_button_register_cb(s_btns[BSP_BUTTON_BOOT], BUTTON_PRESS_DOWN, NULL, on_press, (void *)0);
            iot_button_register_cb(s_btns[BSP_BUTTON_IO0], BUTTON_PRESS_DOWN, NULL, on_press, (void *)1);
            s_created = true;
        } else {
            test_ui_set(&s_row, TEST_FAIL, "Button init failed");
        }
    }
}

static void poll(void)
{
    if (g_power_key_pressed) {
        s_pressed[2] = true;
    }
    int done = 0;
    for (int i = 0; i < 3; i++) {
        if (s_pressed[i]) {
            lv_obj_set_style_bg_color(s_chip[i], lv_color_hex(TEST_COLOR_PASS), 0);
            done++;
        }
    }
    if (done == 3 && s_row.state != TEST_PASS) {
        test_ui_set(&s_row, TEST_PASS, "All keys work");
    }
}

const test_item_t item_buttons = {.build = build, .poll = poll};
