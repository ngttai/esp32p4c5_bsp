/*
 * One test item of the automatic test page. build() adds its row, poll() runs every 300 ms in
 * the LVGL task (keep it short, long work goes to a task), stop() releases the hardware.
 */
#pragma once

#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*build)(lv_obj_t *body);
    void (*poll)(void);
    void (*stop)(void);
} test_item_t;

extern const test_item_t item_wifi;
extern const test_item_t item_audio;
extern const test_item_t item_imu;
extern const test_item_t item_sd;
extern const test_item_t item_camera;
extern const test_item_t item_power;
extern const test_item_t item_buttons;
extern const test_item_t item_led;
extern const test_item_t item_modem;

/** Set from the PMIC event callback (main.c) when the power key is pressed briefly */
extern volatile bool g_power_key_pressed;

#ifdef __cplusplus
}
#endif
