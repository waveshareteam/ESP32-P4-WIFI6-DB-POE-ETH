/* SPDX-License-Identifier: Apache-2.0 */
#include "XiaozhiBoardAudio.hpp"

#include <limits.h>
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "dev_audio_codec.h"
#include "esp_log.h"

namespace esp_brookesia::apps {
namespace {
constexpr char TAG[] = "XiaozhiBoardAudio";

esp_err_t acquireCodec(const char *name, bool &acquired,
                       esp_codec_dev_handle_t &codec)
{
    esp_err_t ret = esp_board_manager_init_device_by_name(name);
    if (ret != ESP_OK) {
        return ret;
    }
    acquired = true;
    void *handle = nullptr;
    ret = esp_board_manager_get_device_handle(name, &handle);
    if (ret != ESP_OK) {
        return ret;
    }
    auto *device = static_cast<dev_audio_codec_handles_t *>(handle);
    codec = device ? device->codec_dev : nullptr;
    return codec ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t releaseCodec(const char *name, bool &acquired, bool &opened,
                       esp_codec_dev_handle_t &codec)
{
    if (opened) {
        esp_err_t ret = esp_codec_dev_close(codec);
        if (ret != ESP_OK) {
            return ret;
        }
        opened = false;
    }
    if (acquired) {
        esp_err_t ret = esp_board_manager_deinit_device_by_name(name);
        if (ret != ESP_OK) {
            return ret;
        }
        acquired = false;
    }
    codec = nullptr;
    return ESP_OK;
}
} // namespace

XiaozhiBoardAudio::~XiaozhiBoardAudio()
{
    (void)close();
}

esp_err_t XiaozhiBoardAudio::open(uint32_t sample_rate, bool capture)
{
    esp_err_t ret = close();
    if (ret != ESP_OK) {
        return ret;
    }
    esp_codec_dev_sample_info_t format = {};
    format.sample_rate = sample_rate;
    format.bits_per_sample = 16;
    format.channel = 2;

    ret = acquireCodec(ESP_BOARD_DEVICE_NAME_AUDIO_DAC, _dac_acquired, _dac);
    if (ret == ESP_OK) {
        ret = esp_codec_dev_open(_dac, &format);
        _dac_opened = ret == ESP_OK;
    }
    if (ret == ESP_OK && capture) {
        ret = acquireCodec(ESP_BOARD_DEVICE_NAME_AUDIO_ADC, _adc_acquired, _adc);
        if (ret == ESP_OK) {
            ret = esp_codec_dev_open(_adc, &format);
            _adc_opened = ret == ESP_OK;
        }
        if (ret == ESP_OK) {
            // ES8311 board labels: microphone, analog echo reference.
            ret = esp_codec_dev_set_data_layout_label(_adc, "FC,RE");
        }
    }
    if (ret == ESP_OK) {
        ret = esp_codec_dev_set_out_mute(_dac, false);
    }
    if (ret != ESP_OK) {
        (void)close();
    }
    return ret;
}

esp_err_t XiaozhiBoardAudio::close()
{
    // Release ADC first: its I2S RX shares the DAC's clocks.
    esp_err_t adc_ret = releaseCodec(
        ESP_BOARD_DEVICE_NAME_AUDIO_ADC, _adc_acquired, _adc_opened, _adc
    );
    esp_err_t dac_ret = adc_ret == ESP_OK ? releaseCodec(
        ESP_BOARD_DEVICE_NAME_AUDIO_DAC, _dac_acquired, _dac_opened, _dac
    ) : ESP_OK;
    esp_err_t ret = adc_ret != ESP_OK ? adc_ret : dac_ret;
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Release audio devices failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t XiaozhiBoardAudio::read(void *data, size_t size, size_t *bytes_read)
{
    if (!bytes_read) {
        return ESP_ERR_INVALID_ARG;
    }
    *bytes_read = 0;
    if (!data || size == 0 || size > INT_MAX || size % (INPUT_CHANNELS * sizeof(int16_t))) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!_adc_opened) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret = esp_codec_dev_read(_adc, data, static_cast<int>(size));
    if (ret == ESP_OK) {
        *bytes_read = size;
    }
    return ret;
}

esp_err_t XiaozhiBoardAudio::write(void *data, size_t size, size_t *bytes_written)
{
    if (!bytes_written) {
        return ESP_ERR_INVALID_ARG;
    }
    *bytes_written = 0;
    if (!data || size == 0 || size > INT_MAX || size % (2 * sizeof(int16_t))) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!_dac_opened) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret = esp_codec_dev_write(_dac, data, static_cast<int>(size));
    if (ret == ESP_OK) {
        *bytes_written = size;
    }
    return ret;
}

} // namespace esp_brookesia::apps
