#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "test_ui.h"

static void open_page(lv_event_t *e)
{
    void (*show)(void) = (void (*)(void))lv_event_get_user_data(e);
    show();
}

static void menu_entry(lv_obj_t *body, const char *icon, const char *text, const char *sub, void (*show)(void))
{
    lv_obj_t *b = lv_button_create(body);
    lv_obj_set_size(b, LV_PCT(100), 96);
    lv_obj_set_style_bg_color(b, lv_color_hex(TEST_COLOR_CARD), 0);
    lv_obj_set_style_radius(b, 16, 0);
    lv_obj_set_style_pad_hor(b, 20, 0);
    lv_obj_add_event_cb(b, open_page, LV_EVENT_CLICKED, (void *)show);
    lv_obj_t *i = lv_label_create(b);
    lv_label_set_text(i, icon);
    lv_obj_set_style_text_font(i, &lv_font_montserrat_28, 0);
    lv_obj_align(i, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *t = lv_label_create(b);
    lv_label_set_text(t, text);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);
    lv_obj_align(t, LV_ALIGN_TOP_LEFT, 52, 10);
    lv_obj_t *s = lv_label_create(b);
    lv_label_set_text(s, sub);
    lv_obj_set_style_text_font(s, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(s, lv_color_hex(TEST_COLOR_DIM), 0);
    lv_obj_align(s, LV_ALIGN_BOTTOM_LEFT, 52, -10);
}

void test_menu_show(void)
{
    lv_obj_t *body = test_ui_screen("Hardware test", NULL);
    menu_entry(body, LV_SYMBOL_LIST, "Automatic test", "Keys, PMIC, IMU, audio, SD, Wi-Fi, camera, LED, 4G", test_auto_show);
    menu_entry(body, LV_SYMBOL_IMAGE, "Screen colors", "Solid colors, bars, rainbow, lines, gradient", test_color_show);
    menu_entry(body, LV_SYMBOL_EDIT, "Touch", "Touch every cell of the grid", test_touch_show);

    esp_chip_info_t chip;
    esp_chip_info(&chip);
    lv_obj_t *info = lv_label_create(body);
    lv_obj_set_style_text_color(info, lv_color_hex(TEST_COLOR_DIM), 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_16, 0);
    lv_obj_set_style_margin_top(info, 16, 0);
    lv_label_set_text_fmt(info, "ESP32-P4 rev v%d.%d  |  ESP-IDF %s\nApp %s  |  PSRAM %u KB",
                          chip.revision / 100, chip.revision % 100, esp_get_idf_version(),
                          esp_app_get_description()->version,
                          (unsigned)(heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024));
    test_ui_load(body);
}
