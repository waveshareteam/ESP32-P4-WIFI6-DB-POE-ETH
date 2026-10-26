/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_attr.h"
#include "esp_codec_dev_defaults.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_vfs_fat.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/semphr.h"

#include "bsp/esp-bsp.h"
#include "bsp_board_extra.h"

static const char *TAG = "bsp_extra_board";

static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;
static i2s_chan_handle_t i2s_tx_chan;
static i2s_chan_handle_t i2s_rx_chan;
static const audio_codec_data_if_t *i2s_data_if;
/* One ES8311 drives both directions; both codec devices share these. */
static const audio_codec_gpio_if_t *codec_gpio_if;
static const audio_codec_ctrl_if_t *codec_ctrl_if;
static const audio_codec_if_t *codec_if;

static bool _is_audio_init = false;
static bool _is_player_init = false;
static int _vloume_intensity = CODEC_DEFAULT_VOLUME;
/* Last output format opened on the codec, so dev_resume() restores the track's
 * sample rate instead of falling back to the default one. */
static uint32_t _out_rate = CODEC_DEFAULT_SAMPLE_RATE;
static uint32_t _out_bits = CODEC_DEFAULT_BIT_WIDTH;
static i2s_slot_mode_t _out_ch = CODEC_DEFAULT_CHANNEL;
static SemaphoreHandle_t audio_mutex;
static SemaphoreHandle_t audio_out_mutex;
static SemaphoreHandle_t audio_in_mutex;

static audio_player_cb_t audio_idle_callback = NULL;
static void *audio_idle_cb_user_data = NULL;
static char audio_file_path[128];

static esp_err_t audio_file_path_length(const char *file_path, size_t *length)
{
    if (file_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    size_t path_length = strnlen(file_path, sizeof(audio_file_path));
    if (path_length >= sizeof(audio_file_path)) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (length != NULL) {
        *length = path_length;
    }
    return ESP_OK;
}

static esp_err_t set_audio_file_path(const char *file_path)
{
    size_t path_length;
    ESP_RETURN_ON_ERROR(audio_file_path_length(file_path, &path_length), TAG, "Invalid audio file path");
    memcpy(audio_file_path, file_path, path_length + 1);
    return ESP_OK;
}

static esp_err_t audio_lock(void);
static void audio_unlock(void);
static esp_err_t audio_output_lock(void);
static void audio_output_unlock(void);

#define BSP_EXTRA_I2S_GPIO_CFG  \
    {                           \
        .mclk = BSP_I2S_MCLK,   \
        .bclk = BSP_I2S_SCLK,   \
        .ws = BSP_I2S_LRCK,     \
        .dout = BSP_I2S_DOUT,   \
        .din = BSP_I2S_DIN,     \
        .invert_flags = {       \
            .mclk_inv = false,  \
            .bclk_inv = false,  \
            .ws_inv = false,    \
        },                      \
    }

/**************************************************************************************************
 *
 * Extra Board Function
 *
 **************************************************************************************************/

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    // Volume saved when muting and restored when unmuting.

    ESP_RETURN_ON_ERROR(audio_output_lock(), TAG, "Lock player mute failed");
    esp_err_t ret = bsp_extra_codec_mute_set(setting == AUDIO_PLAYER_MUTE ? true : false);

    // restore the voice volume upon unmuting
    if (ret == ESP_OK && setting == AUDIO_PLAYER_UNMUTE) {
        ret = esp_codec_dev_set_out_vol(play_dev_handle, _vloume_intensity);
    }
    audio_output_unlock();

    return ret;
}

static void audio_callback(audio_player_cb_ctx_t *ctx)
{
    if (audio_idle_callback) {
        ctx->user_ctx = audio_idle_cb_user_data;
        audio_idle_callback(ctx);
    }
}

bool bsp_extra_display_lock(int32_t timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
}

void bsp_extra_display_unlock(void)
{
    esp_lv_adapter_unlock();
}

/*
 * Lock order: audio_mutex (configuration) -> audio_out_mutex -> audio_in_mutex.
 * Blocking reads and writes take only their own direction lock, so a voice
 * capture loop cannot starve concurrent playback on the full-duplex bus.
 */
static esp_err_t audio_mutex_init(void)
{
    if (!audio_mutex) {
        audio_mutex = xSemaphoreCreateRecursiveMutex();
    }
    if (!audio_out_mutex) {
        audio_out_mutex = xSemaphoreCreateRecursiveMutex();
    }
    if (!audio_in_mutex) {
        audio_in_mutex = xSemaphoreCreateRecursiveMutex();
    }
    return audio_mutex && audio_out_mutex && audio_in_mutex ? ESP_OK : ESP_ERR_NO_MEM;
}

static esp_err_t audio_take(SemaphoreHandle_t mutex)
{
    return xSemaphoreTakeRecursive(mutex, portMAX_DELAY) == pdTRUE
           ? ESP_OK : ESP_ERR_TIMEOUT;
}

/* Reconfigures both directions. */
static esp_err_t audio_lock(void)
{
    ESP_RETURN_ON_ERROR(audio_mutex_init(), TAG, "Create audio mutex failed");
    ESP_RETURN_ON_ERROR(audio_take(audio_mutex), TAG, "Lock audio config failed");
    if (audio_take(audio_out_mutex) != ESP_OK) {
        xSemaphoreGiveRecursive(audio_mutex);
        return ESP_ERR_TIMEOUT;
    }
    if (audio_take(audio_in_mutex) != ESP_OK) {
        xSemaphoreGiveRecursive(audio_out_mutex);
        xSemaphoreGiveRecursive(audio_mutex);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static void audio_unlock(void)
{
    xSemaphoreGiveRecursive(audio_in_mutex);
    xSemaphoreGiveRecursive(audio_out_mutex);
    xSemaphoreGiveRecursive(audio_mutex);
}

/* Reconfigures the playback side only; capture keeps running. */
static esp_err_t audio_output_lock(void)
{
    ESP_RETURN_ON_ERROR(audio_mutex_init(), TAG, "Create audio mutex failed");
    ESP_RETURN_ON_ERROR(audio_take(audio_mutex), TAG, "Lock audio config failed");
    if (audio_take(audio_out_mutex) != ESP_OK) {
        xSemaphoreGiveRecursive(audio_mutex);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static void audio_output_unlock(void)
{
    xSemaphoreGiveRecursive(audio_out_mutex);
    xSemaphoreGiveRecursive(audio_mutex);
}

/* Data path locks: exclude reconfiguration of the same direction only. */
static esp_err_t audio_write_lock(void)
{
    ESP_RETURN_ON_ERROR(audio_mutex_init(), TAG, "Create audio mutex failed");
    return audio_take(audio_out_mutex);
}

static void audio_write_unlock(void)
{
    xSemaphoreGiveRecursive(audio_out_mutex);
}

static esp_err_t audio_read_lock(void)
{
    ESP_RETURN_ON_ERROR(audio_mutex_init(), TAG, "Create audio mutex failed");
    return audio_take(audio_in_mutex);
}

static void audio_read_unlock(void)
{
    xSemaphoreGiveRecursive(audio_in_mutex);
}

static void update_codec_result(esp_err_t *result, esp_err_t codec_result)
{
    if (*result == ESP_OK && codec_result != ESP_OK) {
        *result = codec_result;
    }
}

static void update_codec_interface_result(esp_err_t *result, int codec_result)
{
    if (*result == ESP_OK && codec_result != ESP_CODEC_DEV_OK) {
        *result = ESP_FAIL;
    }
}

static esp_err_t delete_i2s_channel(i2s_chan_handle_t *channel)
{
    if (!channel || !*channel) {
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;
    esp_err_t err = i2s_channel_disable(*channel);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ret = err;
    }
    update_codec_result(&ret, i2s_del_channel(*channel));
    *channel = NULL;
    return ret;
}

static esp_err_t delete_codec_devices(void)
{
    esp_err_t ret = ESP_OK;

    if (record_dev_handle) {
        update_codec_result(&ret, esp_codec_dev_close(record_dev_handle));
        esp_codec_dev_delete(record_dev_handle);
        record_dev_handle = NULL;
    }
    if (play_dev_handle) {
        update_codec_result(&ret, esp_codec_dev_close(play_dev_handle));
        esp_codec_dev_delete(play_dev_handle);
        play_dev_handle = NULL;
    }
    if (codec_if) {
        update_codec_interface_result(&ret, audio_codec_delete_codec_if(codec_if));
        codec_if = NULL;
    }
    if (codec_ctrl_if) {
        update_codec_interface_result(&ret, audio_codec_delete_ctrl_if(codec_ctrl_if));
        codec_ctrl_if = NULL;
    }
    if (codec_gpio_if) {
        update_codec_interface_result(&ret, audio_codec_delete_gpio_if(codec_gpio_if));
        codec_gpio_if = NULL;
    }
    return ret;
}

static esp_err_t bsp_extra_audio_deinit(void)
{
    esp_err_t ret = ESP_OK;

    update_codec_result(&ret, delete_codec_devices());
    if (i2s_data_if) {
        update_codec_interface_result(&ret, audio_codec_delete_data_if(i2s_data_if));
        i2s_data_if = NULL;
    }
    update_codec_result(&ret, delete_i2s_channel(&i2s_rx_chan));
    update_codec_result(&ret, delete_i2s_channel(&i2s_tx_chan));
    _is_audio_init = false;
    return ret;
}

/* TEMPORARY diagnostics: TX DMA ran out of queued audio (played silence). */
static volatile uint32_t s_i2s_tx_underruns;

static bool IRAM_ATTR i2s_tx_underrun_cb(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx)
{
    (void)handle;
    (void)event;
    (void)user_ctx;
    s_i2s_tx_underruns = s_i2s_tx_underruns + 1;
    return false;
}

uint32_t bsp_extra_i2s_get_tx_underrun_count(void)
{
    return s_i2s_tx_underruns;
}

static esp_err_t bsp_extra_audio_bus_init(void)
{
    ESP_RETURN_ON_FALSE(
        !i2s_tx_chan && !i2s_rx_chan && !i2s_data_if,
        ESP_ERR_INVALID_STATE,
        TAG,
        "Audio bus is already initialized"
    );

    esp_err_t ret = ESP_OK;
    i2s_chan_config_t channel_config =
        I2S_CHANNEL_DEFAULT_CONFIG(CONFIG_BSP_I2S_NUM, I2S_ROLE_MASTER);
    channel_config.auto_clear = true;
    // The default 6 x 240 frames hold only 30 ms at 48 kHz, less than one video
    // frame of audio (33 ms at 30 fps). The AVI player feeds audio once per video
    // frame, so playback underran on every frame. 8 x 480 frames hold 80 ms; one
    // buffer of 480 frames stays below the 4 KB DMA limit even at 32-bit stereo.
    channel_config.dma_desc_num = BSP_EXTRA_I2S_DMA_DESC_NUM;
    channel_config.dma_frame_num = BSP_EXTRA_I2S_DMA_FRAME_NUM;

    i2s_std_config_t tx_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(CODEC_VOICE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO
        ),
        .gpio_cfg = BSP_EXTRA_I2S_GPIO_CFG,
    };
    tx_config.clk_cfg.mclk_multiple = CODEC_MCLK_MULTIPLE;
    i2s_std_config_t rx_config = tx_config;
    tx_config.gpio_cfg.din = I2S_GPIO_UNUSED;
    rx_config.gpio_cfg.dout = I2S_GPIO_UNUSED;

    ESP_GOTO_ON_ERROR(
        i2s_new_channel(&channel_config, &i2s_tx_chan, &i2s_rx_chan),
        fail,
        TAG,
        "Create I2S channels failed"
    );
    ESP_GOTO_ON_ERROR(
        i2s_channel_init_std_mode(i2s_tx_chan, &tx_config),
        fail,
        TAG,
        "Initialize I2S TX failed"
    );
    ESP_GOTO_ON_ERROR(
        i2s_channel_init_std_mode(i2s_rx_chan, &rx_config),
        fail,
        TAG,
        "Initialize I2S RX failed"
    );
    // TEMPORARY diagnostics: count TX underruns. Must be registered before enable.
    {
        const i2s_event_callbacks_t tx_callbacks = {
            .on_send_q_ovf = i2s_tx_underrun_cb,
        };
        ESP_GOTO_ON_ERROR(
            i2s_channel_register_event_callback(i2s_tx_chan, &tx_callbacks, NULL),
            fail, TAG, "Register I2S TX callbacks failed"
        );
    }
    ESP_GOTO_ON_ERROR(
        i2s_channel_enable(i2s_tx_chan), fail, TAG, "Enable I2S TX failed"
    );
    ESP_GOTO_ON_ERROR(
        i2s_channel_enable(i2s_rx_chan), fail, TAG, "Enable I2S RX failed"
    );

    audio_codec_i2s_cfg_t i2s_config = {
        .port = CONFIG_BSP_I2S_NUM,
        .rx_handle = i2s_rx_chan,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_config);
    ESP_GOTO_ON_FALSE(
        i2s_data_if, ESP_ERR_NO_MEM, fail, TAG, "Create codec I2S data interface failed"
    );
    return ESP_OK;

fail:
    {
        esp_err_t cleanup_ret = bsp_extra_audio_deinit();
        if (cleanup_ret != ESP_OK) {
            ESP_LOGE(TAG, "Clean up audio bus failed: %s", esp_err_to_name(cleanup_ret));
        }
    }
    return ret;
}

static esp_codec_dev_handle_t create_codec_device(esp_codec_dev_type_t dev_type)
{
    esp_codec_dev_cfg_t device_config = {
        .dev_type = dev_type,
        .codec_if = codec_if,
        .data_if = i2s_data_if,
    };
    esp_codec_dev_handle_t device = esp_codec_dev_new(&device_config);
    if (device) {
        // Disabling the codec on close would power down the shared ES8311 and
        // stop the other direction as well. Rate changes restart it explicitly
        // through codec_standby() instead.
        (void)esp_codec_set_disable_when_closed(device, false);
    }
    return device;
}

static esp_err_t create_codec_devices(void)
{
    esp_err_t ret = bsp_i2c_init();
    ESP_RETURN_ON_ERROR(ret, TAG, "Initialize codec I2C bus failed");
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(i2c_bus, ESP_ERR_INVALID_STATE, TAG, "Codec I2C bus is unavailable");

    codec_gpio_if = audio_codec_new_gpio();
    ESP_GOTO_ON_FALSE(codec_gpio_if, ESP_ERR_NO_MEM, fail, TAG, "Create codec GPIO interface failed");

    audio_codec_i2c_cfg_t i2c_config = {
        .port = BSP_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus,
    };
    codec_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_config);
    ESP_GOTO_ON_FALSE(codec_ctrl_if, ESP_ERR_NO_MEM, fail, TAG, "Create codec I2C interface failed");

    es8311_codec_cfg_t codec_config = {
        .ctrl_if = codec_ctrl_if,
        .gpio_if = codec_gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .pa_pin = BSP_POWER_AMP_IO,
        .pa_reverted = false,
        .master_mode = false,
        // BSP_I2S_MCLK drives the ES8311 MCLK pin at CODEC_MCLK_MULTIPLE * fs.
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = {
            .pa_voltage = 5.0,
            .codec_dac_voltage = 3.3,
        },
        // The ADC carries the single analog microphone only; no DAC loopback
        // reference is mixed into the right slot.
        .no_dac_ref = true,
        .mclk_div = CODEC_MCLK_MULTIPLE,
    };
    codec_if = es8311_codec_new(&codec_config);
    ESP_GOTO_ON_FALSE(codec_if, ESP_FAIL, fail, TAG, "Create ES8311 codec failed");

    play_dev_handle = create_codec_device(ESP_CODEC_DEV_TYPE_OUT);
    ESP_GOTO_ON_FALSE(play_dev_handle, ESP_ERR_NO_MEM, fail, TAG, "Create playback device failed");
    record_dev_handle = create_codec_device(ESP_CODEC_DEV_TYPE_IN);
    ESP_GOTO_ON_FALSE(record_dev_handle, ESP_ERR_NO_MEM, fail, TAG, "Create record device failed");
    return ESP_OK;

fail:
    (void)delete_codec_devices();
    return ret;
}

esp_err_t bsp_extra_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (bytes_read) {
        *bytes_read = 0;
    }
    ESP_RETURN_ON_FALSE(record_dev_handle && audio_buffer, ESP_ERR_INVALID_STATE, TAG, "Recorder is not initialized");

    ESP_RETURN_ON_ERROR(audio_read_lock(), TAG, "Lock audio input failed");
    esp_err_t ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    if (ret == ESP_OK && bytes_read) {
        *bytes_read = len;
    }
    audio_read_unlock();
    return ret;
}

esp_err_t bsp_extra_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (bytes_written) {
        *bytes_written = 0;
    }
    ESP_RETURN_ON_FALSE(play_dev_handle && audio_buffer, ESP_ERR_INVALID_STATE, TAG, "Player is not initialized");

    ESP_RETURN_ON_ERROR(audio_write_lock(), TAG, "Lock audio output failed");
    esp_err_t ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    if (ret == ESP_OK && bytes_written) {
        *bytes_written = len;
    }
    audio_write_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_output_stop(void)
{
    if (!play_dev_handle) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(audio_output_lock(), TAG, "Lock playback codec failed");
    esp_err_t ret = esp_codec_dev_close(play_dev_handle);
    audio_output_unlock();
    return ret == ESP_CODEC_DEV_OK ? ESP_OK : ret;
}

/* The ES8311 mute callback only touches the DAC (reg 0x31); the ADC keeps running. */
static esp_err_t codec_set_output_mute(bool mute)
{
    ESP_RETURN_ON_FALSE(esp_codec_dev_set_out_mute(play_dev_handle, mute) == ESP_CODEC_DEV_OK,
                        ESP_FAIL, TAG, "Set DAC mute failed");
    return ESP_OK;
}

/*
 * Put the shared ES8311 into standby so the next esp_codec_dev_open() applies
 * the new clock coefficients and restarts the chip. Closing a device does not
 * disable the codec (see create_codec_device()), so without this the driver
 * would rewrite the clock registers while running and never restart it.
 */
static void codec_standby(void)
{
    if (codec_if && codec_if->enable) {
        (void)codec_if->enable(codec_if, false);
    }
}

/*
 * The ES8311 driver looks up its clock coefficients by MCLK = rate *
 * CODEC_MCLK_MULTIPLE. 24-bit slots make esp_codec_dev switch the I2S MCLK to
 * 384 * fs, which no longer matches that divider, so only 16/32-bit is allowed.
 */
static bool codec_fs_supported(uint32_t rate, uint32_t bits_cfg)
{
    if (bits_cfg != 16 && bits_cfg != 32) {
        return false;
    }
    switch (rate) {
    case 8000:
    case 11025:
    case 12000:
    case 16000:
    case 22050:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
        return true;
    default:
        return false;
    }
}

esp_err_t bsp_extra_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    ESP_RETURN_ON_FALSE(play_dev_handle && record_dev_handle, ESP_ERR_INVALID_STATE, TAG, "Codec is not initialized");
    ESP_RETURN_ON_FALSE(codec_fs_supported(rate, bits_cfg), ESP_ERR_NOT_SUPPORTED, TAG,
                        "ES8311 cannot clock %lu Hz / %lu-bit", (unsigned long)rate,
                        (unsigned long)bits_cfg);
    ESP_RETURN_ON_ERROR(audio_output_lock(), TAG, "Lock playback reconfiguration failed");
    // Ordinary media/output reconfiguration must not close the voice capture
    // session. Voice mode uses bsp_extra_codec_set_voice_fs(), which
    // intentionally owns both sides of the shared bus.
    esp_err_t ret = bsp_extra_codec_output_stop();
    if (ret != ESP_OK) {
        audio_output_unlock();
        ESP_LOGE(TAG, "Close playback codec failed: %s", esp_err_to_name(ret));
        return ret;
    }
    codec_standby();

    esp_codec_dev_sample_info_t output_fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    ret = esp_codec_dev_open(play_dev_handle, &output_fs);
    if (ret == ESP_OK) {
        ret = codec_set_output_mute(false);
    }
    if (ret == ESP_OK) {
        _out_rate = rate;
        _out_bits = bits_cfg;
        _out_ch = ch;
    }
    if (ret != ESP_OK) {
        (void)esp_codec_dev_close(play_dev_handle);
    }
    audio_output_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_set_voice_fs(uint32_t rate, uint32_t bits_cfg)
{
    ESP_RETURN_ON_FALSE(play_dev_handle && record_dev_handle, ESP_ERR_INVALID_STATE, TAG, "Codec is not initialized");
    ESP_RETURN_ON_FALSE(codec_fs_supported(rate, bits_cfg), ESP_ERR_NOT_SUPPORTED, TAG,
                        "ES8311 cannot clock %lu Hz / %lu-bit", (unsigned long)rate,
                        (unsigned long)bits_cfg);
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock voice codec reconfiguration failed");
    esp_err_t ret = bsp_extra_codec_dev_stop();
    if (ret != ESP_OK) {
        audio_unlock();
        ESP_LOGE(TAG, "Close codec devices failed: %s", esp_err_to_name(ret));
        return ret;
    }
    codec_standby();

    esp_codec_dev_sample_info_t output_fs = {
        .sample_rate = rate,
        .channel = I2S_SLOT_MODE_STEREO,
        .bits_per_sample = bits_cfg,
    };
    esp_codec_dev_sample_info_t input_fs = {
        .sample_rate = rate,
        .channel = CODEC_VOICE_INPUT_CHANNELS,
        .bits_per_sample = bits_cfg,
    };

    ret = esp_codec_dev_open(play_dev_handle, &output_fs);
    if (ret == ESP_OK) {
        ret = codec_set_output_mute(false);
    }
    if (ret != ESP_OK) {
        (void)bsp_extra_codec_dev_stop();
        audio_unlock();
        return ret;
    }

    ret = esp_codec_dev_open(record_dev_handle, &input_fs);
    if (ret == ESP_OK) {
        ret = esp_codec_dev_set_in_gain(record_dev_handle, CODEC_DEFAULT_ADC_VOLUME);
    }
    if (ret != ESP_OK) {
        (void)bsp_extra_codec_dev_stop();
    }
    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_volume_set(int volume, int *volume_set)
{
    (void)volume_set;
    ESP_RETURN_ON_ERROR(audio_output_lock(), TAG, "Lock codec volume failed");
    _vloume_intensity = volume;
    esp_err_t ret = esp_codec_dev_set_out_vol(play_dev_handle, volume);
    audio_output_unlock();
    ESP_RETURN_ON_ERROR(ret, TAG, "Set Codec volume failed");

    ESP_LOGI(TAG, "Setting volume: %d", volume);

    return ESP_OK;
}

int bsp_extra_codec_volume_get(void)
{
    return _vloume_intensity;
}

esp_err_t bsp_extra_codec_mute_set(bool enable)
{
    ESP_RETURN_ON_ERROR(audio_output_lock(), TAG, "Lock codec mute failed");
    esp_err_t ret = play_dev_handle ? codec_set_output_mute(enable) : ESP_ERR_INVALID_STATE;
    audio_output_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_dev_stop(void)
{
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock codec stop failed");
    esp_err_t ret = ESP_OK;

    if (record_dev_handle) {
        update_codec_result(&ret, esp_codec_dev_close(record_dev_handle));
    }
    if (play_dev_handle) {
        update_codec_result(&ret, esp_codec_dev_close(play_dev_handle));
    }

    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_codec_dev_resume(void)
{
    /* Re-apply the format that was opened last. Reopening with the default
     * format made a resumed 44.1 kHz track play back at the wrong pitch. */
    return bsp_extra_codec_set_fs(_out_rate, _out_bits, _out_ch);
}

esp_err_t bsp_extra_codec_init(void)
{
    ESP_RETURN_ON_ERROR(audio_lock(), TAG, "Lock codec initialization failed");
    if (_is_audio_init) {
        audio_unlock();
        return ESP_OK;
    }

    esp_err_t ret = bsp_extra_audio_bus_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize full-duplex audio bus failed: %s", esp_err_to_name(ret));
        audio_unlock();
        return ret;
    }

    ret = create_codec_devices();
    if (ret != ESP_OK) {
        goto cleanup;
    }

    ret = bsp_extra_codec_set_fs(
              CODEC_DEFAULT_SAMPLE_RATE,
              CODEC_DEFAULT_BIT_WIDTH,
              CODEC_DEFAULT_CHANNEL
          );
    if (ret != ESP_OK) {
        goto cleanup;
    }
    _is_audio_init = true;
    audio_unlock();
    return ESP_OK;

cleanup:
    {
        esp_err_t cleanup_ret = bsp_extra_audio_deinit();
        if (cleanup_ret != ESP_OK) {
            ESP_LOGE(TAG, "Clean up codec initialization failed: %s", esp_err_to_name(cleanup_ret));
        }
    }
    audio_unlock();
    return ret;
}

esp_err_t bsp_extra_player_init(void)
{
    if (_is_player_init) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(bsp_extra_codec_init(), TAG, "Initialize codec failed");
    ESP_RETURN_ON_ERROR(bsp_extra_codec_dev_resume(), TAG, "Resume output codec failed");
    audio_player_config_t config = { .mute_fn = audio_mute_function,
                                     .write_fn = bsp_extra_i2s_write,
                                     .clk_set_fn = bsp_extra_codec_set_fs,
                                     .priority = 5
                                   };
    ESP_RETURN_ON_ERROR(audio_player_new(config), TAG, "audio_player_init failed");
    audio_player_callback_register(audio_callback, NULL);

    _is_player_init = true;

    return ESP_OK;
}

esp_err_t bsp_extra_player_del(void)
{
    esp_err_t ret = audio_player_delete();
    ESP_RETURN_ON_ERROR(ret, TAG, "Delete audio player failed");
    _is_player_init = false;
    ESP_RETURN_ON_ERROR(bsp_extra_codec_output_stop(), TAG, "Stop playback codec failed");

    return ESP_OK;
}

esp_err_t bsp_extra_file_instance_init(const char *path, file_iterator_instance_t **ret_instance)
{
    ESP_RETURN_ON_FALSE(path, ESP_FAIL, TAG, "path is NULL");
    ESP_RETURN_ON_FALSE(ret_instance, ESP_FAIL, TAG, "ret_instance is NULL");

    file_iterator_instance_t *file_iterator = file_iterator_new(path);
    ESP_RETURN_ON_FALSE(file_iterator, ESP_FAIL, TAG, "file_iterator_new failed, %s", path);

    *ret_instance = file_iterator;

    return ESP_OK;
}

esp_err_t bsp_extra_player_play_index(file_iterator_instance_t *instance, int index)
{
    ESP_RETURN_ON_FALSE(instance, ESP_FAIL, TAG, "instance is NULL");

    ESP_LOGI(TAG, "play_index(%d)", index);
    char filename[128];
    int retval = file_iterator_get_full_path_from_index(instance, index, filename, sizeof(filename));
    ESP_RETURN_ON_FALSE(retval != 0, ESP_FAIL, TAG, "file_iterator_get_full_path_from_index failed");
    ESP_RETURN_ON_ERROR(audio_file_path_length(filename, NULL), TAG, "Invalid audio file path");

    ESP_LOGI(TAG, "opening file '%s'", filename);
    FILE *fp = fopen(filename, "rb");
    ESP_RETURN_ON_FALSE(fp, ESP_FAIL, TAG, "unable to open file");

    ESP_LOGI(TAG, "Playing '%s'", filename);
    esp_err_t ret = audio_player_play(fp);
    if (ret != ESP_OK) {
        fclose(fp);
        ESP_LOGE(TAG, "audio_player_play failed: %s", esp_err_to_name(ret));
        return ret;
    }

    file_iterator_set_index(instance, index);
    ESP_RETURN_ON_ERROR(set_audio_file_path(filename), TAG, "Invalid audio file path");

    return ESP_OK;
}

esp_err_t bsp_extra_player_play_file(const char *file_path)
{
    ESP_RETURN_ON_ERROR(audio_file_path_length(file_path, NULL), TAG, "Invalid audio file path");
    ESP_LOGI(TAG, "opening file '%s'", file_path);
    FILE *fp = fopen(file_path, "rb");
    ESP_RETURN_ON_FALSE(fp, ESP_FAIL, TAG, "unable to open file");

    ESP_LOGI(TAG, "Playing '%s'", file_path);
    esp_err_t ret = audio_player_play(fp);
    if (ret != ESP_OK) {
        fclose(fp);
        ESP_LOGE(TAG, "audio_player_play failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_RETURN_ON_ERROR(set_audio_file_path(file_path), TAG, "Invalid audio file path");

    return ESP_OK;
}

void bsp_extra_player_register_callback(audio_player_cb_t cb, void *user_data)
{
    audio_idle_callback = cb;
    audio_idle_cb_user_data = user_data;
}

bool bsp_extra_player_is_playing_by_path(const char *file_path)
{
    if (audio_file_path_length(file_path, NULL) != ESP_OK) {
        return false;
    }
    return (strcmp(audio_file_path, file_path) == 0);
}

bool bsp_extra_player_is_playing_by_index(file_iterator_instance_t *instance, int index)
{
    return (index == file_iterator_get_index(instance));
}
