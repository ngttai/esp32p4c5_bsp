/*
 * Audio: ES8311 + speaker and ES7210 + two MEMS mics through esp_codec_dev.
 *  - Speaker: plays a 1 kHz tone, then asks whether it was heard.
 *  - Microphone: records 3 s, shows the peak level and plays the recording back.
 *    PASS when the recording carries sound (peak above the noise floor).
 * Playback and recording share the I2S clock, so both run at the same rate.
 */
#include <math.h>
#include <stdlib.h>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/esp-bsp.h"
#include "test_items.h"
#include "test_ui.h"

#define RATE        16000
#define TONE_HZ     1000
#define TONE_MS     1000
#define REC_MS      3000
#define VOLUME      70
#define MIC_GAIN_DB 30.0f
#define PEAK_SOUND  500     /* peaks above this mean the mics picked up sound */

typedef enum { JOB_NONE, JOB_TONE, JOB_RECORD } job_t;

static test_row_t s_spk_row;
static test_row_t s_mic_row;
static lv_obj_t *s_answer;          /* Yes / No buttons after the tone */
static volatile bool s_busy;
static volatile job_t s_done_job;
static volatile int s_peak;
static volatile bool s_failed;

static esp_codec_dev_sample_info_t pcm16_mono(void)
{
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = RATE,
        .channel = 1,
        .bits_per_sample = 16,
    };
    return fs;
}

static bool play(const int16_t *pcm, size_t samples)
{
    esp_codec_dev_handle_t spk = bsp_audio_codec_speaker_init();
    esp_codec_dev_sample_info_t fs = pcm16_mono();
    if (!spk || esp_codec_dev_open(spk, &fs) != ESP_CODEC_DEV_OK) {
        return false;
    }
    esp_codec_dev_set_out_vol(spk, VOLUME);
    bool ok = esp_codec_dev_write(spk, (void *)pcm, samples * sizeof(int16_t)) == ESP_CODEC_DEV_OK;
    esp_codec_dev_close(spk);
    return ok;
}

static void audio_task(void *arg)
{
    const job_t job = (job_t)(intptr_t)arg;
    s_failed = false;
    if (job == JOB_TONE) {
        const size_t n = RATE * TONE_MS / 1000;
        int16_t *tone = heap_caps_malloc(n * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        if (tone) {
            for (size_t i = 0; i < n; i++) {
                tone[i] = (int16_t)(12000.0f * sinf(2.0f * (float)M_PI * TONE_HZ * i / RATE));
            }
            s_failed = !play(tone, n);
            free(tone);
        } else {
            s_failed = true;
        }
    } else {
        const size_t n = RATE * REC_MS / 1000;
        int16_t *rec = heap_caps_malloc(n * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        /* the microphone frame: BSP_MIC_FRAME_SLOTS int16 (MIC1, MIC2, the echo reference), read as 2 x 32 bit */
        int16_t *frames = heap_caps_malloc(n * BSP_MIC_FRAME_SLOTS * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        esp_codec_dev_handle_t mic = bsp_audio_codec_microphone_init();
        esp_codec_dev_sample_info_t fs = {
            .sample_rate = RATE,
            .channel = BSP_MIC_CHANNELS,
            .bits_per_sample = BSP_MIC_BITS,
        };
        if (rec && frames && mic && esp_codec_dev_open(mic, &fs) == ESP_CODEC_DEV_OK) {
            esp_codec_dev_set_in_gain(mic, MIC_GAIN_DB);
            s_failed = esp_codec_dev_read(mic, frames, n * BSP_MIC_FRAME_SLOTS * sizeof(int16_t)) != ESP_CODEC_DEV_OK;
            esp_codec_dev_close(mic);
            for (size_t i = 0; i < n; i++) {
                const int16_t *fr = frames + i * BSP_MIC_FRAME_SLOTS;
                rec[i] = (int16_t)((fr[BSP_MIC_SLOT_MIC1] + fr[BSP_MIC_SLOT_MIC2]) / 2);
            }
            int peak = 0;
            for (size_t i = 0; i < n; i++) {
                peak = abs(rec[i]) > peak ? abs(rec[i]) : peak;
            }
            s_peak = peak;
            if (!s_failed) {
                s_failed = !play(rec, n);
            }
        } else {
            s_failed = true;
        }
        free(rec);
        free(frames);
    }
    s_done_job = job;
    s_busy = false;
    vTaskDelete(NULL);
}

static void start(job_t job)
{
    if (s_busy) {
        return;
    }
    s_busy = true;
    s_done_job = JOB_NONE;
    if (job == JOB_TONE) {
        test_ui_set(&s_spk_row, TEST_RUNNING, "Playing a 1 kHz tone...");
        lv_obj_add_flag(s_answer, LV_OBJ_FLAG_HIDDEN);
    } else {
        test_ui_set(&s_mic_row, TEST_RUNNING, "Recording 3 s, speak or tap near the mics...");
    }
    xTaskCreate(audio_task, "audio_test", 4096, (void *)(intptr_t)job, 4, NULL);
}

static void on_tone(lv_event_t *e)
{
    start(JOB_TONE);
}

static void on_record(lv_event_t *e)
{
    start(JOB_RECORD);
}

static void on_answer(lv_event_t *e)
{
    const bool heard = lv_event_get_user_data(e) != NULL;
    test_ui_set(&s_spk_row, heard ? TEST_PASS : TEST_FAIL, heard ? "Tone heard" : "Tone not heard");
    lv_obj_add_flag(s_answer, LV_OBJ_FLAG_HIDDEN);
}

static void build(lv_obj_t *body)
{
    test_ui_row(body, "Speaker", &s_spk_row);
    test_ui_set(&s_spk_row, TEST_IDLE, "Plays a tone through ES8311 and the speaker");
    test_ui_button(s_spk_row.ctrl, LV_SYMBOL_PLAY " Play tone", on_tone, NULL);
    s_answer = lv_obj_create(s_spk_row.ctrl);
    lv_obj_remove_style_all(s_answer);
    lv_obj_set_size(s_answer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_answer, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_answer, 10, 0);
    lv_obj_t *yes = test_ui_button(s_answer, LV_SYMBOL_OK " Heard", on_answer, (void *)1);
    lv_obj_set_style_bg_color(yes, lv_color_hex(TEST_COLOR_PASS), 0);
    lv_obj_t *no = test_ui_button(s_answer, LV_SYMBOL_CLOSE " Silent", on_answer, NULL);
    lv_obj_set_style_bg_color(no, lv_color_hex(TEST_COLOR_FAIL), 0);
    lv_obj_add_flag(s_answer, LV_OBJ_FLAG_HIDDEN);

    test_ui_row(body, "Microphones", &s_mic_row);
    test_ui_set(&s_mic_row, TEST_IDLE, "Records 3 s through ES7210, then plays it back");
    test_ui_button(s_mic_row.ctrl, LV_SYMBOL_AUDIO " Record 3 s", on_record, NULL);
}

static void poll(void)
{
    const job_t job = s_done_job;
    if (job == JOB_NONE) {
        return;
    }
    s_done_job = JOB_NONE;
    if (job == JOB_TONE) {
        if (s_failed) {
            test_ui_set(&s_spk_row, TEST_FAIL, "Speaker codec error (ES8311)");
        } else {
            test_ui_set(&s_spk_row, TEST_RUNNING, "Did you hear the tone?");
            lv_obj_remove_flag(s_answer, LV_OBJ_FLAG_HIDDEN);
        }
    } else if (s_failed) {
        test_ui_set(&s_mic_row, TEST_FAIL, "Microphone codec error (ES7210)");
    } else {
        const bool sound = s_peak > PEAK_SOUND;
        test_ui_set(&s_mic_row, sound ? TEST_PASS : TEST_FAIL, "Peak %d of 32767%s, played back", s_peak,
                    sound ? "" : " (silent)");
    }
}

const test_item_t item_audio = {.build = build, .poll = poll};
