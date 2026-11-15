/*
 * IMU (LSM6DS3TR-C): bsp_imu_init() checks WHO_AM_I, then the accelerometer is read every
 * 300 ms. PASS when the measured gravity is close to 1 g (board resting).
 */
#include <math.h>
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

#define ACCEL_LSB_PER_G 16384.0f    /* +-2 g full scale */

static test_row_t s_row;
static bool s_ready;

static void build(lv_obj_t *body)
{
    test_ui_row(body, "IMU", &s_row);
    s_ready = bsp_imu_init() == ESP_OK;
    if (!s_ready) {
        test_ui_set(&s_row, TEST_FAIL, "LSM6DS3TR-C not found");
    }
}

static void poll(void)
{
    bsp_imu_raw_t r;
    if (!s_ready || bsp_imu_read(&r) != ESP_OK) {
        return;
    }
    const float x = r.accel[0] / ACCEL_LSB_PER_G;
    const float y = r.accel[1] / ACCEL_LSB_PER_G;
    const float z = r.accel[2] / ACCEL_LSB_PER_G;
    const float g = sqrtf(x * x + y * y + z * z);
    const bool ok = g > 0.8f && g < 1.2f;
    if (ok || s_row.state != TEST_PASS) {
        test_ui_set(&s_row, ok ? TEST_PASS : TEST_RUNNING, "Accel x %+.2f  y %+.2f  z %+.2f  |g| %.2f\nGyro %d %d %d",
                    x, y, z, g, r.gyro[0], r.gyro[1], r.gyro[2]);
    }
}

const test_item_t item_imu = {.build = build, .poll = poll};
