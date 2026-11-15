/*
 * Shared look of the hardware test app: screens with a header and a back button,
 * test rows with a PASS / FAIL status, buttons.
 */
#pragma once

#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_COLOR_BG       0x0E1116
#define TEST_COLOR_CARD     0x1B2030
#define TEST_COLOR_TEXT     0xFFFFFF
#define TEST_COLOR_DIM      0x9AA3B2
#define TEST_COLOR_PASS     0x10B981
#define TEST_COLOR_FAIL     0xEF4444
#define TEST_COLOR_RUN      0xF59E0B
#define TEST_COLOR_ACCENT   0x2F80ED

typedef enum {
    TEST_IDLE,
    TEST_RUNNING,
    TEST_PASS,
    TEST_FAIL,
} test_state_t;

typedef struct {
    lv_obj_t *status;   /*!< PASS / FAIL / ... badge */
    lv_obj_t *detail;   /*!< one line of measured values */
    lv_obj_t *ctrl;     /*!< row of buttons under the detail line */
    test_state_t state;
} test_row_t;

/** New screen with a title bar (back button when on_back is set); returns the scrollable body. */
lv_obj_t *test_ui_screen(const char *title, lv_event_cb_t on_back);
/** Load a screen built with test_ui_screen() and delete the previous one. */
void test_ui_load(lv_obj_t *body);
/** Card with a title, a status badge, a detail line and a button row. */
void test_ui_row(lv_obj_t *body, const char *title, test_row_t *row);
void test_ui_set(test_row_t *row, test_state_t state, const char *detail_fmt, ...);
lv_obj_t *test_ui_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, void *user);

void test_menu_show(void);
void test_auto_show(void);
void test_color_show(void);
void test_touch_show(void);

#ifdef __cplusplus
}
#endif
