#include <stdarg.h>
#include <stdio.h>
#include "test_ui.h"

#define HEADER_H 72

lv_obj_t *test_ui_screen(const char *title, lv_event_cb_t on_back)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(TEST_COLOR_BG), 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(TEST_COLOR_TEXT), 0);
    lv_obj_set_style_text_font(scr, &lv_font_montserrat_20, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *head = lv_obj_create(scr);
    lv_obj_remove_style_all(head);
    lv_obj_set_size(head, LV_PCT(100), HEADER_H);
    lv_obj_remove_flag(head, LV_OBJ_FLAG_SCROLLABLE);
    if (on_back) {
        lv_obj_t *back = lv_button_create(head);
        lv_obj_set_size(back, 64, 48);
        lv_obj_align(back, LV_ALIGN_LEFT_MID, 12, 0);
        lv_obj_set_style_bg_color(back, lv_color_hex(TEST_COLOR_CARD), 0);
        lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, NULL);
        lv_obj_t *l = lv_label_create(back);
        lv_label_set_text(l, LV_SYMBOL_LEFT);
        lv_obj_center(l);
    }
    lv_obj_t *t = lv_label_create(head);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_28, 0);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *body = lv_obj_create(scr);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, LV_PCT(100), LV_VER_RES - HEADER_H);
    lv_obj_set_y(body, HEADER_H);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(body, 14, 0);
    lv_obj_set_style_pad_bottom(body, 16, 0);
    lv_obj_set_style_pad_row(body, 12, 0);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    return body;
}

void test_ui_load(lv_obj_t *body)
{
    lv_obj_t *old = lv_screen_active();
    lv_obj_t *scr = lv_obj_get_screen(body);
    lv_screen_load(scr);
    if (old && old != scr) {
        lv_obj_delete_async(old);
    }
}

void test_ui_row(lv_obj_t *body, const char *title, test_row_t *row)
{
    lv_obj_t *card = lv_obj_create(body);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(TEST_COLOR_CARD), 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *top = lv_obj_create(card);
    lv_obj_remove_style_all(top);
    lv_obj_set_size(top, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *t = lv_label_create(top);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);
    row->status = lv_label_create(top);

    row->detail = lv_label_create(card);
    lv_obj_set_width(row->detail, LV_PCT(100));
    lv_obj_set_style_text_color(row->detail, lv_color_hex(TEST_COLOR_DIM), 0);
    lv_obj_set_style_text_font(row->detail, &lv_font_montserrat_16, 0);

    row->ctrl = lv_obj_create(card);
    lv_obj_remove_style_all(row->ctrl);
    lv_obj_set_size(row->ctrl, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row->ctrl, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(row->ctrl, 10, 0);
    lv_obj_set_style_pad_row(row->ctrl, 8, 0);
    lv_obj_remove_flag(row->ctrl, LV_OBJ_FLAG_SCROLLABLE);
    test_ui_set(row, TEST_IDLE, "");
}

void test_ui_set(test_row_t *row, test_state_t state, const char *detail_fmt, ...)
{
    static const char *const text[] = {"--", "...", "PASS", "FAIL"};
    static const uint32_t color[] = {TEST_COLOR_DIM, TEST_COLOR_RUN, TEST_COLOR_PASS, TEST_COLOR_FAIL};
    row->state = state;
    lv_label_set_text(row->status, text[state]);
    lv_obj_set_style_text_color(row->status, lv_color_hex(color[state]), 0);
    if (detail_fmt) {
        char buf[160];
        va_list ap;
        va_start(ap, detail_fmt);
        vsnprintf(buf, sizeof(buf), detail_fmt, ap);
        va_end(ap);
        lv_label_set_text(row->detail, buf);
    }
}

lv_obj_t *test_ui_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, void *user)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_height(b, 48);
    lv_obj_set_style_bg_color(b, lv_color_hex(TEST_COLOR_ACCENT), 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    return b;
}
