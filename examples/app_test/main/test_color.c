/*
 * Screen colors: full-screen test patterns, one per tap, to look for dead pixels, tint, banding
 * and line defects. A tap on the last one goes back to the menu.
 *
 * The patterns are written straight into an RGB565 canvas buffer (PSRAM).
 */
#include "esp_heap_caps.h"
#include "test_ui.h"

typedef enum {
    PAT_WHITE,
    PAT_BLACK,
    PAT_RED,
    PAT_GREEN,
    PAT_BLUE,
    PAT_GRAY_STEPS,     /* 16 vertical gray bands */
    PAT_COLOR_BARS,     /* white, yellow, cyan, green, magenta, red, blue, black */
    PAT_RAINBOW,        /* hue sweep */
    PAT_LINES,          /* 1 px lines: horizontal in the top half, vertical in the bottom half */
    PAT_GRAY_GRADIENT,  /* black to white */
    PAT_COUNT
} pattern_t;

static const char *const s_names[PAT_COUNT] = {
    "White", "Black", "Red", "Green", "Blue", "Gray steps", "Color bars", "Rainbow", "1 px lines", "Gray gradient",
};

/* What the pattern should look like, so that an intended pattern is not taken for a defect */
static const char *const s_notes[PAT_COUNT] = {
    "Look for dark or colored dots",
    "Look for bright dots and backlight leaks",
    "Even red, no dots",
    "Even green, no dots",
    "Even blue, no dots",
    "16 gray bands, each one visible",
    "8 bars: white yellow cyan green magenta red blue black",
    "Smooth hue sweep, no steps",
    "Top: horizontal lines, bottom: vertical lines.\nBoth halves look evenly gray",
    "Smooth black to white, no bands",
};

static lv_obj_t *s_canvas;
static lv_obj_t *s_hint;
static uint16_t *s_buf;
static int s_w, s_h;
static int s_index;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static uint16_t hex565(uint32_t hex)
{
    return rgb565(hex >> 16, hex >> 8, hex);
}

/* Fully saturated color of a hue in degrees */
static uint16_t hue565(int h)
{
    const int f = (h % 60) * 255 / 60;
    switch ((h / 60) % 6) {
    case 0:
        return rgb565(255, f, 0);
    case 1:
        return rgb565(255 - f, 255, 0);
    case 2:
        return rgb565(0, 255, f);
    case 3:
        return rgb565(0, 255 - f, 255);
    case 4:
        return rgb565(f, 0, 255);
    default:
        return rgb565(255, 0, 255 - f);
    }
}

/* Color of one pixel column for the patterns that only change along x */
static uint16_t column_color(pattern_t p, int x)
{
    static const uint32_t bars[] = {0xFFFFFF, 0xFFFF00, 0x00FFFF, 0x00FF00, 0xFF00FF, 0xFF0000, 0x0000FF, 0x000000};
    switch (p) {
    case PAT_WHITE:
        return 0xFFFF;
    case PAT_RED:
        return hex565(0xFF0000);
    case PAT_GREEN:
        return hex565(0x00FF00);
    case PAT_BLUE:
        return hex565(0x0000FF);
    case PAT_GRAY_STEPS: {
        const uint8_t g = (uint8_t)((x * 16 / s_w) * 255 / 15);
        return rgb565(g, g, g);
    }
    case PAT_COLOR_BARS:
        return hex565(bars[x * 8 / s_w]);
    case PAT_RAINBOW:
        return hue565(x * 360 / s_w);
    case PAT_GRAY_GRADIENT: {
        const uint8_t g = (uint8_t)(x * 255 / (s_w - 1));
        return rgb565(g, g, g);
    }
    default:
        return 0x0000;
    }
}

static void draw(pattern_t p)
{
    for (int y = 0; y < s_h; y++) {
        uint16_t *row = s_buf + y * s_w;
        for (int x = 0; x < s_w; x++) {
            if (p == PAT_LINES) {
                row[x] = ((y < s_h / 2 ? y : x) & 1) ? 0xFFFF : 0x0000;
            } else {
                row[x] = column_color(p, x);
            }
        }
    }
    lv_obj_invalidate(s_canvas);
    lv_label_set_text_fmt(s_hint, "%s  (%d/%d)\n%s", s_names[p], p + 1, PAT_COUNT, s_notes[p]);
    lv_obj_align(s_hint, LV_ALIGN_BOTTOM_MID, 0, -16);
}

static void on_tap(lv_event_t *e)
{
    if (++s_index >= PAT_COUNT) {
        test_menu_show();
        return;
    }
    draw((pattern_t)s_index);
}

static void on_delete(lv_event_t *e)
{
    heap_caps_free(s_buf);
    s_buf = NULL;
}

void test_color_show(void)
{
    s_w = LV_HOR_RES;
    s_h = LV_VER_RES;
    s_buf = heap_caps_malloc((size_t)s_w * s_h * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!s_buf) {
        return;
    }
    lv_obj_t *old = lv_screen_active();
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr, on_tap, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(scr, on_delete, LV_EVENT_DELETE, NULL);

    s_canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(s_canvas, s_buf, s_w, s_h, LV_COLOR_FORMAT_RGB565);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_CLICKABLE);     /* taps go to the screen */

    s_hint = lv_label_create(scr);
    lv_obj_set_style_text_font(s_hint, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(s_hint, LV_TEXT_ALIGN_CENTER, 0);
    /* white on a dark pill: readable on any pattern */
    lv_obj_set_style_text_color(s_hint, lv_color_white(), 0);
    lv_obj_set_style_bg_color(s_hint, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_hint, LV_OPA_70, 0);
    lv_obj_set_style_radius(s_hint, 14, 0);
    lv_obj_set_style_pad_hor(s_hint, 14, 0);
    lv_obj_set_style_pad_ver(s_hint, 6, 0);
    lv_obj_align(s_hint, LV_ALIGN_BOTTOM_MID, 0, -16);

    s_index = 0;
    draw(PAT_WHITE);
    lv_screen_load(scr);
    lv_obj_delete_async(old);
}
