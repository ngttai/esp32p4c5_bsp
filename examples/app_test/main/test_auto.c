#include "test_items.h"
#include "test_ui.h"

#define POLL_MS 300

static const test_item_t *const s_items[] = {
    &item_buttons, &item_power, &item_imu, &item_audio, &item_sd, &item_wifi, &item_camera,
    &item_led, &item_modem,
};
#define ITEM_COUNT (sizeof(s_items) / sizeof(s_items[0]))

static lv_timer_t *s_poll;

static void poll_cb(lv_timer_t *t)
{
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (s_items[i]->poll) {
            s_items[i]->poll();
        }
    }
}

static void on_back(lv_event_t *e)
{
    lv_timer_delete(s_poll);
    s_poll = NULL;
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (s_items[i]->stop) {
            s_items[i]->stop();
        }
    }
    test_menu_show();
}

void test_auto_show(void)
{
    lv_obj_t *body = test_ui_screen("Automatic test", on_back);
    for (int i = 0; i < ITEM_COUNT; i++) {
        s_items[i]->build(body);
    }
    s_poll = lv_timer_create(poll_cb, POLL_MS, NULL);
    test_ui_load(body);
}
