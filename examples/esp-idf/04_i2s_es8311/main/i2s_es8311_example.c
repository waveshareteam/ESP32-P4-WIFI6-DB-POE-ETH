/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_device.h"
#include "esp_check.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_vol.h"
#include "esp_log.h"
#include "dev_audio_codec.h"
#include "example_config.h"

static const char *TAG = "i2s_es8311";
static esp_codec_dev_handle_t s_dac;
#if CONFIG_EXAMPLE_MODE_ECHO
static esp_codec_dev_handle_t s_adc;
#endif

#if CONFIG_EXAMPLE_MODE_MUSIC
extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[] asm("_binary_canon_pcm_end");
#endif

static esp_err_t codec_open(const char *name, uint32_t sample_rate, uint8_t channels,
                            esp_codec_dev_handle_t *codec)
{
    ESP_RETURN_ON_ERROR(esp_board_device_init(name), TAG, "Initialize %s", name);
    dev_audio_codec_handles_t *handles = NULL;
    esp_err_t ret = esp_board_device_get_handle(name, (void **)&handles);
    if (ret != ESP_OK || handles == NULL || handles->codec_dev == NULL) {
        esp_board_device_deinit(name);
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

    esp_codec_dev_sample_info_t sample = {
        .sample_rate = sample_rate,
        .channel = channels,
        .channel_mask = channels == 1 ? 0x01 : 0x03,
        .bits_per_sample = 16,
        .mclk_multiple = 256,
    };
    if (esp_codec_dev_open(handles->codec_dev, &sample) != ESP_CODEC_DEV_OK) {
        esp_board_device_deinit(name);
        return ESP_FAIL;
    }
    *codec = handles->codec_dev;
    return ESP_OK;
}

static void codec_close(const char *name, esp_codec_dev_handle_t codec)
{
    if (codec != NULL) {
        esp_codec_dev_close(codec);
        esp_board_device_deinit(name);
    }
}

#if CONFIG_EXAMPLE_MODE_MUSIC
static void play_music_task(void *arg)
{
    const size_t data_size = music_pcm_end - music_pcm_start;
    while (true) {
        const uint8_t *data = music_pcm_start;
        while (data < music_pcm_end) {
            size_t chunk_size = data_size - (size_t)(data - music_pcm_start);
            if (chunk_size > 4096) {
                chunk_size = 4096;
            }
            if (esp_codec_dev_write(s_dac, (void *)data, chunk_size) != ESP_CODEC_DEV_OK) {
                ESP_LOGE(TAG, "Playback failed");
                codec_close("audio_dac", s_dac);
                s_dac = NULL;
                vTaskDelete(NULL);
                return;
            }
            data += chunk_size;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#else
static void echo_task(void *arg)
{
    int16_t input[600];
    int16_t output[1200];
    while (true) {
        if (esp_codec_dev_read(s_adc, input, sizeof(input)) != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Microphone read failed");
            break;
        }
        for (size_t i = 0; i < sizeof(input) / sizeof(input[0]); i++) {
            output[i * 2] = input[i];
            output[i * 2 + 1] = input[i];
        }
        if (esp_codec_dev_write(s_dac, output, sizeof(output)) != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "Speaker write failed");
            break;
        }
    }
    codec_close("audio_adc", s_adc);
    codec_close("audio_dac", s_dac);
    s_adc = NULL;
    s_dac = NULL;
    vTaskDelete(NULL);
}
#endif

void app_main(void)
{
    ESP_ERROR_CHECK(codec_open("audio_dac", EXAMPLE_SAMPLE_RATE, 2, &s_dac));
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(s_dac, EXAMPLE_VOICE_VOLUME) == ESP_CODEC_DEV_OK ?
                    ESP_OK : ESP_FAIL);
#if CONFIG_EXAMPLE_MODE_MUSIC
    if (xTaskCreate(play_music_task, "i2s_music", 4096, NULL, 5, NULL) != pdPASS) {
        codec_close("audio_dac", s_dac);
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }
#else
    ESP_ERROR_CHECK(codec_open("audio_adc", EXAMPLE_SAMPLE_RATE, 1, &s_adc));
    ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(s_adc, CONFIG_EXAMPLE_MIC_GAIN) == ESP_CODEC_DEV_OK ?
                    ESP_OK : ESP_FAIL);
    if (xTaskCreate(echo_task, "i2s_echo", 4096, NULL, 5, NULL) != pdPASS) {
        codec_close("audio_adc", s_adc);
        codec_close("audio_dac", s_dac);
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }
#endif
}
