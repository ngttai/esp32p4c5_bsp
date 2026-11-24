/*
 * Audio for the KSDIY ESP32-P4C5 board: ES8311 (DAC, speaker) and ES7210 (ADC, 2 MEMS mics)
 * on I2C0, I2S0 for data. Speaker amp enable is PA_CTRL (GPIO3), active high.
 * ALDO3 (codecs) is not switched by this BSP.
 * Built on espressif/esp_codec_dev, as the esp-bsp ESP32-P4 Function EV Board BSP does.
 */
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "es7210_adc.h"
#include "es8311_codec.h"
#include "esp_check.h"
#include "esp_log.h"
#include "bsp/esp32p4c5_bsp.h"
#include "bsp_err_check.h"

static const char *TAG = "esp32p4c5_audio";

#define BSP_AUDIO_DEFAULT_RATE 22050   /* esp-bsp default; esp_codec_dev_open() sets the real rate */
#define BSP_AUDIO_PA_GPIO     BSP_SPEAKER_PA
#define ES8311_ADDR_7BIT      0x18
#define ES7210_ADDR_7BIT      0x40

static i2s_chan_handle_t s_tx_chan;
static i2s_chan_handle_t s_rx_chan;
static const audio_codec_data_if_t *s_data_if;
static esp_codec_dev_handle_t s_speaker;
static esp_codec_dev_handle_t s_mic;

esp_err_t bsp_audio_init(const i2s_std_config_t *i2s_config)
{
    if (s_data_if != NULL) {
        return ESP_OK;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BSP_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    esp_err_t err = i2s_new_channel(&chan_cfg, &s_tx_chan, &s_rx_chan);
    ESP_RETURN_ON_ERROR(err, TAG, "i2s_new_channel failed");

    const i2s_std_config_t default_cfg = BSP_I2S_DUPLEX_MONO_CFG(BSP_AUDIO_DEFAULT_RATE);
    const i2s_std_config_t *std_cfg = i2s_config ? i2s_config : &default_cfg;
    err = i2s_channel_init_std_mode(s_tx_chan, std_cfg);
    if (err == ESP_OK) {
        err = i2s_channel_init_std_mode(s_rx_chan, std_cfg);
    }
    if (err == ESP_OK) {
        err = i2s_channel_enable(s_tx_chan);
    }
    if (err == ESP_OK) {
        err = i2s_channel_enable(s_rx_chan);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S init failed: %s", esp_err_to_name(err));
        (void)i2s_del_channel(s_tx_chan);
        (void)i2s_del_channel(s_rx_chan);
        s_tx_chan = NULL;
        s_rx_chan = NULL;
        return err;
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = BSP_I2S_NUM,
        .rx_handle = s_rx_chan,
        .tx_handle = s_tx_chan,
    };
    s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    if (s_data_if == NULL) {
        ESP_LOGE(TAG, "audio_codec_new_i2s_data failed");
        (void)i2s_channel_disable(s_tx_chan);
        (void)i2s_channel_disable(s_rx_chan);
        (void)i2s_del_channel(s_tx_chan);
        (void)i2s_del_channel(s_rx_chan);
        s_tx_chan = NULL;
        s_rx_chan = NULL;
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* The codecs are powered from ALDO3, which this BSP does not switch. If they do not answer,
 * the rail is off (or the codec is missing): report it here instead of failing later. */
static bool bsp_audio_codec_present(uint8_t addr_7bit, const char *name)
{
    esp_err_t err = i2c_master_probe(bsp_i2c_get_handle(), addr_7bit, 50);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s (0x%02X) does not ACK: %s. Check that ALDO3 (codec rail) is on.",
                 name, addr_7bit, esp_err_to_name(err));
        return false;
    }
    return true;
}

esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    if (s_speaker != NULL) {
        return s_speaker;
    }
    if (bsp_i2c_init() != ESP_OK || bsp_audio_init(NULL) != ESP_OK) {
        return NULL;
    }
    if (!bsp_audio_codec_present(ES8311_ADDR_7BIT, "ES8311 DAC")) {
        return NULL;
    }

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = bsp_i2c_get_handle(),
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (ctrl_if == NULL) {
        return NULL;
    }

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = ctrl_if,
        .gpio_if = audio_codec_new_gpio(),
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = BSP_AUDIO_PA_GPIO,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = {
            .pa_voltage = 5.0,
            .codec_dac_voltage = 3.3,
        },
    };
    const audio_codec_if_t *codec_if = es8311_codec_new(&es8311_cfg);
    if (codec_if == NULL) {
        return NULL;
    }

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if,
        .data_if = s_data_if,
    };
    s_speaker = esp_codec_dev_new(&dev_cfg);
    if (s_speaker == NULL) {
        ESP_LOGE(TAG, "speaker codec device create failed");
    }
    return s_speaker;
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    if (s_mic != NULL) {
        return s_mic;
    }
    if (bsp_i2c_init() != ESP_OK || bsp_audio_init(NULL) != ESP_OK) {
        return NULL;
    }
    if (!bsp_audio_codec_present(ES7210_ADDR_7BIT, "ES7210 ADC")) {
        return NULL;
    }

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_I2C_NUM,
        .addr = ES7210_CODEC_DEFAULT_ADDR,
        .bus_handle = bsp_i2c_get_handle(),
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (ctrl_if == NULL) {
        return NULL;
    }

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = ctrl_if,
        .master_mode = false,
        /* MIC3 is the speaker signal looped back from the ES8311 output (R7 / R10, 0 ohm): the echo reference.
         * Three inputs put the ES7210 in TDM: open the device with 2 channels of 32 bits, each frame then
         * holds 4 slots of 16 bits */
        .mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2 | ES7210_SEL_MIC3,
    };
    const audio_codec_if_t *codec_if = es7210_codec_new(&es7210_cfg);
    if (codec_if == NULL) {
        return NULL;
    }

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = codec_if,
        .data_if = s_data_if,
    };
    s_mic = esp_codec_dev_new(&dev_cfg);
    if (s_mic == NULL) {
        ESP_LOGE(TAG, "mic codec device create failed");
    }
    return s_mic;
}
