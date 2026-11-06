/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sys/cdefs.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_codec_dev.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "audio_player.h"
#include "file_iterator.h"
#include "bsp_board_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CODEC_DEFAULT_SAMPLE_RATE           (16000)
/* ES8311 runs from MCLK = 256 * fs: 8 to 48 kHz with 16- or 32-bit samples. */
#define CODEC_VOICE_SAMPLE_RATE             (48000)
#define CODEC_MCLK_MULTIPLE                 (I2S_MCLK_MULTIPLE_256)
#define CODEC_DEFAULT_BIT_WIDTH             (16)
#define CODEC_DEFAULT_ADC_VOLUME            (24.0)
#define CODEC_DEFAULT_CHANNEL               (2)
#define CODEC_VOICE_INPUT_CHANNELS          (1)
#define CODEC_DEFAULT_VOLUME                (80)
/* I2S DMA ring: BSP_EXTRA_I2S_DMA_DESC_NUM buffers of BSP_EXTRA_I2S_DMA_FRAME_NUM frames each.
 * Players can use the total depth to estimate how much written audio is still queued. */
#define BSP_EXTRA_I2S_DMA_DESC_NUM          (8)
#define BSP_EXTRA_I2S_DMA_FRAME_NUM         (480)

/*
 * The ES8311 ADC records the board's single analog microphone. Capture is
 * opened mono, so every frame holds one MIC1 sample. There is no hardware
 * echo-reference channel.
 */
#define BSP_EXTRA_MIC1_CHANNEL_INDEX        (0)

#define BSP_LCD_BACKLIGHT_BRIGHTNESS_MAX    (95)
#define BSP_LCD_BACKLIGHT_BRIGHTNESS_MIN    (0)
#define LCD_LEDC_CH                         (CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH)

/**************************************************************************************************
 * BSP Extra interface
 * Mainly provided display-lock and I2S Codec interfaces.
 **************************************************************************************************/
/**
 * @brief Lock LVGL through esp_lv_adapter.
 *
 * The local BSP returns esp_err_t directly from a bool function, which
 * inverts successful ESP_OK results. Use this compatibility wrapper instead.
 *
 * @param timeout_ms Maximum wait in milliseconds; negative waits forever.
 * @return true when the lock was acquired.
 */
bool bsp_extra_display_lock(int32_t timeout_ms);

/** @brief Release the LVGL lock acquired by bsp_extra_display_lock(). */
void bsp_extra_display_unlock(void);

/**
 * @brief Get display brightness.
 *
 * @return
 *   - brightness in percent
 */
int bsp_display_brightness_get(void);

/**
 * @brief Player set mute.
 *
 * @param enable: true or false
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_mute_set(bool enable);

/**
 * @brief Player set volume.
 *
 * @param volume: volume set
 * @param volume_set: volume set response
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_volume_set(int volume, int *volume_set);

/**
 * @brief Player get volume.
 *
 * @return
 *   - volume: volume get
 */
int bsp_extra_codec_volume_get(void);

/**
 * @brief Stop I2S function.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_dev_stop(void);

/**
 * @brief Stop only the playback codec session.
 *
 * The ES8311 stays powered and the capture session is left open, so a voice
 * consumer can keep running while a media app releases its output session.
 */
esp_err_t bsp_extra_codec_output_stop(void);

/**
 * @brief Resume I2S function.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_dev_resume(void);

/**
 * @brief Configure and open the output codec only.
 *
 * Any active input and output codec sessions are closed first. This function is
 * used by ordinary audio players and does not start the microphone input.
 * Input and output share one ES8311 and one I2S clock, so an open capture
 * session follows the new sample rate.
 *
 * @param rate Sample rate
 * @param bits_cfg Bits per sample
 * @param ch Output channel mode
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);

/**
 * @brief Configure full-duplex voice audio on the ES8311.
 *
 * Output is stereo. Input is mono (CODEC_VOICE_INPUT_CHANNELS = 1): the
 * single analog microphone on the left I2S slot.
 *
 * @param rate Shared physical sample rate
 * @param bits_cfg Bits per sample
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_codec_set_voice_fs(uint32_t rate, uint32_t bits_cfg);

/**
 * @brief Read data from recoder.
 *
 * @param audio_buffer: The pointer of receiving data buffer
 * @param len: Max data buffer length
 * @param bytes_read: Byte number that actually be read, can be NULL if not needed
 * @param timeout_ms: Max block time
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Write data to player.
 *
 * @param audio_buffer: The pointer of sent data buffer
 * @param len: Max data buffer length
 * @param bytes_written: Byte number that actually be sent, can be NULL if not needed
 * @param timeout_ms: Max block time
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_extra_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);

/**
 * @brief TEMPORARY diagnostics: number of TX DMA underruns since boot.
 *
 * Counts each time the TX DMA had no new audio queued and played silence.
 */
uint32_t bsp_extra_i2s_get_tx_underrun_count(void);


/**
 * @brief Initialize codec play and record handle.
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t bsp_extra_codec_init(void);

/**
 * @brief Initialize audio player task.
 *
 * @param path file path
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t bsp_extra_player_init(void);

/**
 * @brief Delete audio player task.
 *
 * @return
 *      - ESP_OK: Success
 *      - Others: Fail
 */
esp_err_t bsp_extra_player_del(void);

/**
 * @brief Initialize a file iterator instance
 *
 * @param path The file path for the iterator.
 * @param ret_instance A pointer to the file iterator instance to be returned.
 * @return
 *     - ESP_OK: Successfully initialized the file iterator instance.
 *     - ESP_FAIL: Failed to initialize the file iterator instance due to invalid parameters or memory allocation failure.
 */
esp_err_t bsp_extra_file_instance_init(const char *path, file_iterator_instance_t **ret_instance);

/**
 * @brief Play the audio file at the specified index in the file iterator
 *
 * @param instance The file iterator instance.
 * @param index The index of the file to play within the iterator.
 * @return
 *     - ESP_OK: Successfully started playing the audio file.
 *     - ESP_FAIL: Failed to play the audio file due to invalid parameters or file access issues.
 */
esp_err_t bsp_extra_player_play_index(file_iterator_instance_t *instance, int index);

/**
 * @brief Play the audio file specified by the file path
 *
 * @param file_path The path to the audio file to be played.
 * @return
 *     - ESP_OK: Successfully started playing the audio file.
 *     - ESP_FAIL: Failed to play the audio file due to file access issues.
 */
esp_err_t bsp_extra_player_play_file(const char *file_path);

/**
 * @brief Register a callback function for the audio player
 *
 * @param cb The callback function to be registered.
 * @param user_data User data to be passed to the callback function.
 */
void bsp_extra_player_register_callback(audio_player_cb_t cb, void *user_data);

/**
 * @brief Check if the specified audio file is currently playing
 *
 * @param file_path The path to the audio file to check.
 * @return
 *     - true: The specified audio file is currently playing.
 *     - false: The specified audio file is not currently playing.
 */
bool bsp_extra_player_is_playing_by_path(const char *file_path);

/**
 * @brief Check if the audio file at the specified index is currently playing
 *
 * @param instance The file iterator instance.
 * @param index The index of the file to check.
 * @return
 *     - true: The audio file at the specified index is currently playing.
 *     - false: The audio file at the specified index is not currently playing.
 */
bool bsp_extra_player_is_playing_by_index(file_iterator_instance_t *instance, int index);

#ifdef __cplusplus
}
#endif
