/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_codec_dev.h"
#include "esp_err.h"

namespace esp_brookesia::apps {

// Acquire Board Manager devices; stop workers before closing this session.
class XiaozhiBoardAudio {
public:
    static constexpr size_t INPUT_CHANNELS = 2;

    XiaozhiBoardAudio() = default;
    ~XiaozhiBoardAudio();
    XiaozhiBoardAudio(const XiaozhiBoardAudio &) = delete;
    XiaozhiBoardAudio &operator=(const XiaozhiBoardAudio &) = delete;

    esp_err_t open(uint32_t sample_rate, bool capture);
    esp_err_t close();
    esp_err_t read(void *data, size_t size, size_t *bytes_read);
    esp_err_t write(void *data, size_t size, size_t *bytes_written);

private:
    esp_codec_dev_handle_t _adc = nullptr;
    esp_codec_dev_handle_t _dac = nullptr;
    bool _adc_acquired = false;
    bool _dac_acquired = false;
    bool _adc_opened = false;
    bool _dac_opened = false;
};

} // namespace esp_brookesia::apps
