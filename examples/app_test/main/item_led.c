/*
 * RGB LED (WS2812B): bsp_led_rgb_set() shows red, green and blue in turn; the tester confirms
 * the colors. The LED is switched off when leaving the page.
 */
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

#define LEVEL 40    /* of 255: bright enough, not dazzling */

static test_row_t s_row;
static lv_obj_t *s_answer;
static int s_step;      /* 0 idle, 1..3 = red, green, blue, 4 = waiting for the answer */
static int s_tick;

static void on_start(lv_event_t *e)
{
    if (bsp_led_rgb_init() != ESP_OK) {
        test_ui_set(&s_row, TEST_FAIL, "LED driver init failed");
        return;
    }
    s_step = 1;
    s_tick = 0;
    lv_obj_add_flag(s_answer, LV_OBJ_FLAG_HIDDEN);
}

static void on_answer(lv_event_t *e)
{
    const bool seen = lv_event_get_user_data(e) != NULL;
    test_ui_set(&s_row, seen ? TEST_PASS : TEST_FAIL, seen ? "Red, green and blue seen" : "Colors wrong or LED dark");
    lv_obj_add_flag(s_answer, LV_OBJ_FLAG_HIDDEN);
    s_step = 0;
}

static void build(lv_obj_t *body)
{
    test_ui_row(body, "RGB LED", &s_row);
    test_ui_set(&s_row, TEST_IDLE, "Shows red, green, blue on the WS2812B");
    test_ui_button(s_row.ctrl, LV_SYMBOL_PLAY " Show colors", on_start, NULL);
    s_answer = lv_obj_create(s_row.ctrl);
    lv_obj_remove_style_all(s_answer);
    lv_obj_set_size(s_answer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_answer, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_answer, 10, 0);
    lv_obj_t *yes = test_ui_button(s_answer, LV_SYMBOL_OK " Seen", on_answer, (void *)1);
    lv_obj_set_style_bg_color(yes, lv_color_hex(TEST_COLOR_PASS), 0);
    lv_obj_t *no = test_ui_button(s_answer, LV_SYMBOL_CLOSE " Wrong", on_answer, NULL);
    lv_obj_set_style_bg_color(no, lv_color_hex(TEST_COLOR_FAIL), 0);
    lv_obj_add_flag(s_answer, LV_OBJ_FLAG_HIDDEN);
    s_step = 0;
}

/* Every 300 ms: one color per 900 ms */
static void poll(void)
{
    static const char *const names[] = {"", "Red", "Green", "Blue"};
    if (s_step < 1 || s_step > 3 || s_tick++ % 3) {
        return;
    }
    const int c = (s_tick - 1) / 3 + 1;
    if (c > 3) {
        bsp_led_rgb_set(0, 0, 0);
        s_step = 4;
        test_ui_set(&s_row, TEST_RUNNING, "Did the LED show red, green, blue?");
        lv_obj_remove_flag(s_answer, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    bsp_led_rgb_set(c == 1 ? LEVEL : 0, c == 2 ? LEVEL : 0, c == 3 ? LEVEL : 0);
    test_ui_set(&s_row, TEST_RUNNING, "%s...", names[c]);
}

static void stop(void)
{
    if (s_step) {
        bsp_led_rgb_set(0, 0, 0);
    }
}

const test_item_t item_led = {.build = build, .poll = poll, .stop = stop};
