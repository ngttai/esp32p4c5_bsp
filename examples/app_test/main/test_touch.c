/*
 * Touch: a grid of cells over the whole screen. Every cell the finger crosses turns green;
 * PASS when all of them did (no dead zone). The back button stays usable at the top left.
 */
#include "test_ui.h"

#define COLS 6
#define ROWS 10

static lv_obj_t *s_cells[ROWS][COLS];
static int s_left;
static lv_obj_t *s_status;

static void on_back(lv_event_t *e)
{
    test_menu_show();
}

static void on_press(lv_event_t *e)
{
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);
    const int c = p.x * COLS / LV_HOR_RES;
    const int r = p.y * ROWS / LV_VER_RES;
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS || !s_cells[r][c]) {
        return;
    }
    lv_obj_set_style_bg_color(s_cells[r][c], lv_color_hex(TEST_COLOR_PASS), 0);
    s_cells[r][c] = NULL;
    if (--s_left == 0) {
        lv_label_set_text(s_status, "PASS - every cell responds");
        lv_obj_set_style_text_color(s_status, lv_color_hex(TEST_COLOR_PASS), 0);
    } else {
        lv_label_set_text_fmt(s_status, "%d cells left", s_left);
    }
}

void test_touch_show(void)
{
    lv_obj_t *old = lv_screen_active();
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(TEST_COLOR_BG), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr, on_press, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(scr, on_press, LV_EVENT_PRESSED, NULL);

    const int w = LV_HOR_RES / COLS;
    const int h = LV_VER_RES / ROWS;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            lv_obj_t *cell = lv_obj_create(scr);
            lv_obj_remove_style_all(cell);
            lv_obj_set_pos(cell, c * w + 2, r * h + 2);
            lv_obj_set_size(cell, w - 4, h - 4);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(cell, lv_color_hex(TEST_COLOR_CARD), 0);
            lv_obj_set_style_radius(cell, 6, 0);
            lv_obj_remove_flag(cell, LV_OBJ_FLAG_CLICKABLE);   /* touches go to the screen */
            s_cells[r][c] = cell;
        }
    }
    s_left = ROWS * COLS;

    s_status = lv_label_create(scr);
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(TEST_COLOR_TEXT), 0);
    lv_label_set_text_fmt(s_status, "Slide over every cell (%d)", s_left);
    lv_obj_align(s_status, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *back = lv_button_create(scr);
    lv_obj_set_size(back, 64, 48);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(back);
    lv_label_set_text(l, LV_SYMBOL_LEFT);
    lv_obj_center(l);

    lv_screen_load(scr);
    lv_obj_delete_async(old);
}
