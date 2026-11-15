/*
 * Camera (MIPI CSI, SC2336 / OV2710): bsp_camera_start() powers the rails and starts esp_video,
 * then a few frames are captured through the V4L2 device. PASS when they all arrive.
 * The camera is stopped again afterwards.
 */
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "driver/i2c_master.h"
#include "esp_video_device.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "linux/videodev2.h"
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

#define FRAMES 5
#define BUFS   2

static test_row_t s_row;
static volatile bool s_busy;
static volatile bool s_done;
static volatile int s_frames;
static char s_info[96];

#define SENSOR_ADDR 0x30

/* 16-bit product ID at two 16-bit register addresses, 0 when the sensor does not answer */
static uint16_t sensor_read_id(i2c_master_dev_handle_t dev, uint16_t reg_hi, uint16_t reg_lo)
{
    uint8_t hi = 0, lo = 0;
    const uint8_t a[2] = {reg_hi >> 8, reg_hi & 0xFF};
    const uint8_t b[2] = {reg_lo >> 8, reg_lo & 0xFF};
    if (i2c_master_transmit_receive(dev, a, 2, &hi, 1, 100) != ESP_OK ||
            i2c_master_transmit_receive(dev, b, 2, &lo, 1, 100) != ESP_OK) {
        return 0;
    }
    return (uint16_t)(hi << 8 | lo);
}

/* Which of the two supported sensors is fitted; the camera rails must be on */
static const char *sensor_name(void)
{
    const i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t dev;
    if (i2c_master_bus_add_device(bsp_i2c_get_handle(), &cfg, &dev) != ESP_OK) {
        return "sensor ?";
    }
    const char *name = "unknown sensor";
    if (sensor_read_id(dev, 0x3107, 0x3108) == 0xCB3A) {
        name = "SC2336";
    } else if (sensor_read_id(dev, 0x300A, 0x300B) == 0x2710) {
        name = "OV2710";
    }
    i2c_master_bus_rm_device(dev);
    return name;
}

static void camera_task(void *arg)
{
    s_frames = 0;
    strlcpy(s_info, "camera start failed", sizeof(s_info));
    if (bsp_camera_start(NULL) != ESP_OK) {
        goto out;
    }
    int fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY);
    void *map[BUFS] = {0};
    size_t len[BUFS] = {0};
    struct v4l2_format fmt = {.type = V4L2_BUF_TYPE_VIDEO_CAPTURE};
    struct v4l2_requestbuffers req = {.count = BUFS, .type = V4L2_BUF_TYPE_VIDEO_CAPTURE, .memory = V4L2_MEMORY_MMAP};
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (fd < 0 || ioctl(fd, VIDIOC_G_FMT, &fmt) || ioctl(fd, VIDIOC_REQBUFS, &req)) {
        strlcpy(s_info, "V4L2 device setup failed", sizeof(s_info));
        goto close_dev;
    }
    for (int i = 0; i < BUFS; i++) {
        struct v4l2_buffer buf = {.type = V4L2_BUF_TYPE_VIDEO_CAPTURE, .memory = V4L2_MEMORY_MMAP, .index = i};
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            goto close_dev;
        }
        map[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        len[i] = buf.length;
        if (map[i] == MAP_FAILED || ioctl(fd, VIDIOC_QBUF, &buf)) {
            map[i] = NULL;
            goto close_dev;
        }
    }
    if (ioctl(fd, VIDIOC_STREAMON, &type) == 0) {
        for (int f = 0; f < FRAMES; f++) {
            struct v4l2_buffer buf = {.type = V4L2_BUF_TYPE_VIDEO_CAPTURE, .memory = V4L2_MEMORY_MMAP};
            if (ioctl(fd, VIDIOC_DQBUF, &buf) || ioctl(fd, VIDIOC_QBUF, &buf)) {
                break;
            }
            s_frames++;
        }
        ioctl(fd, VIDIOC_STREAMOFF, &type);
    }
    const uint32_t pf = fmt.fmt.pix.pixelformat;
    snprintf(s_info, sizeof(s_info), "%s, %lu x %lu %c%c%c%c, %d of %d frames", sensor_name(),
             (unsigned long)fmt.fmt.pix.width,
             (unsigned long)fmt.fmt.pix.height, (char)pf, (char)(pf >> 8), (char)(pf >> 16), (char)(pf >> 24), s_frames,
             FRAMES);
close_dev:
    for (int i = 0; i < BUFS; i++) {
        if (map[i]) {
            munmap(map[i], len[i]);
        }
    }
    if (fd >= 0) {
        close(fd);
    }
    bsp_camera_stop();
out:
    s_done = true;
    s_busy = false;
    vTaskDelete(NULL);
}

static void on_capture(lv_event_t *e)
{
    if (s_busy) {
        return;
    }
    s_busy = true;
    s_done = false;
    test_ui_set(&s_row, TEST_RUNNING, "Starting the camera...");
    /* Core 0, where app_main started the display: the CSI and DSI controllers share the DW-GDMA
     * interrupt, and a shared interrupt can only be attached on the core that first allocated it */
    xTaskCreatePinnedToCore(camera_task, "cam_test", 6144, NULL, 3, NULL, 0);
}

static void camera_build(lv_obj_t *body)
{
    test_ui_row(body, "Camera", &s_row);
    test_ui_set(&s_row, TEST_IDLE, "Captures %d frames from the CSI sensor", FRAMES);
    test_ui_button(s_row.ctrl, LV_SYMBOL_VIDEO " Capture", on_capture, NULL);
}

static void camera_poll(void)
{
    if (!s_done) {
        return;
    }
    s_done = false;
    test_ui_set(&s_row, s_frames == FRAMES ? TEST_PASS : TEST_FAIL, "%s", s_info);
}

const test_item_t item_camera = {.build = camera_build, .poll = camera_poll};
