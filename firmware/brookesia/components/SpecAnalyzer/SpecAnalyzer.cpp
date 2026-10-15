#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "esp_task.h"
#include "esp_heap_caps.h"
#include "esp_err.h"
#include "brookesia/service_helper/media/audio.hpp"

#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:SpecAnalyzer"
#include "esp_lib_utils.h"
#include "SpecAnalyzer.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>
#define APP_NAME "SpecAnalyzer"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;

namespace {
using AudioEncoderHelper = esp_brookesia::service::helper::AudioEncoder<0>;
using AudioHelper = esp_brookesia::service::helper::Audio;
}

LV_IMG_DECLARE(img_app_specanalyzer);

namespace esp_brookesia::apps {

SpecAnalyzer *SpecAnalyzer::_instance = nullptr;

SpecAnalyzer *SpecAnalyzer::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new SpecAnalyzer(use_status_bar, use_navigation_bar);
        // The smoothing buffer belongs to the singleton instance and must be reset
        // after allocation, before any timer callback can read it.
        memset(&(_instance->_smooth_spectrum[0][0]), 0, sizeof(_instance->_smooth_spectrum));
    }
    return _instance;
}

SpecAnalyzer::SpecAnalyzer(bool use_status_bar, bool use_navigation_bar) : 
    App(APP_NAME, &img_app_specanalyzer, true, use_status_bar, use_navigation_bar),
    _canvas(nullptr),
    _mic_label(nullptr),
    _timer(nullptr),
    _audio_task_handle(nullptr),
    _audio_task_running(false),
    _audio_encoder(nullptr),
    _audio_encoder_started(false),
    _capture_accepting(false),
    _capture_write_index(0),
    _capture_read_index(0),
    _canvas_width(0),
    _canvas_height(0),
    _draw_buf(nullptr)
{
    // Clear DSP state so the first rendered frame starts from silence.
    memset(_raw_data, 0, sizeof(_raw_data));
    memset(_audio_buffer, 0, sizeof(_audio_buffer));
    memset(_fft_buffer, 0, sizeof(_fft_buffer));
    memset(_spectrum, 0, sizeof(_spectrum));
    memset(_display_spectrum, 0, sizeof(_display_spectrum));
    memset(_peak, 0, sizeof(_peak));
    memset(_smooth_spectrum, 0, sizeof(_smooth_spectrum));
}

SpecAnalyzer::~SpecAnalyzer()
{
    close();
}

bool SpecAnalyzer::openMicrophone()
{
    if (_audio_encoder_started) {
        return true;
    }

    auto &manager = service::ServiceManager::get_instance();
    if (!manager.is_initialized() && !manager.init()) {
        ESP_UTILS_LOGE("Initialize service manager failed");
        return false;
    }
    if (!manager.is_running() && !manager.start()) {
        ESP_UTILS_LOGE("Start service manager failed");
        return false;
    }

    _audio_encoder_binding = manager.bind(AudioEncoderHelper::get_name().data());
    if (!_audio_encoder_binding.is_valid()) {
        ESP_UTILS_LOGE("Bind AudioEncoder service failed");
        return false;
    }

    _audio_encoder = service::AudioEncoder::get_instance(0);
    if (_audio_encoder == nullptr) {
        ESP_UTILS_LOGE("AudioEncoder service instance is unavailable");
        (void)closeMicrophone();
        return false;
    }

    _capture_write_index.store(0, std::memory_order_relaxed);
    _capture_read_index.store(0, std::memory_order_relaxed);
    _recorder_data_connection = _audio_encoder->connect_recorder_data([this](const service::RawBuffer &data) {
        onRecorderData(data);
    });
    if (!_recorder_data_connection.connected()) {
        ESP_UTILS_LOGE("Connect AudioEncoder recorder data failed");
        (void)closeMicrophone();
        return false;
    }

    // Request mono PCM from the board audio service. The raw recorder callback
    // still supplies native FC,RE stereo frames; onRecorderData keeps FC.
    AudioHelper::EncoderDynamicConfig config{
        .type = AudioHelper::CodecFormat::PCM,
        .general = {
            .channels = 1,
            .sample_bits = 16,
            .sample_rate = MIC_SAMPLE_RATE,
            .frame_duration = 60,
        },
        .enable_afe = false,
    };
    auto start_result = AudioEncoderHelper::call_function_sync(
                            AudioEncoderHelper::FunctionId::Start,
                            BROOKESIA_DESCRIBE_TO_JSON(config).as_object()
                        );
    if (!start_result) {
        ESP_UTILS_LOGE("Start AudioEncoder service failed: %s", start_result.error().c_str());
        (void)closeMicrophone();
        return false;
    }

    _audio_encoder_started = true;
    _capture_accepting.store(true, std::memory_order_release);
    return true;
}

void SpecAnalyzer::onRecorderData(const service::RawBuffer &data)
{
    constexpr size_t bytes_per_frame = CAPTURE_CHANNELS * sizeof(int16_t);
    if (!_capture_accepting.load(std::memory_order_acquire) || data.data_ptr == nullptr ||
            data.data_size == 0 || (data.data_size % bytes_per_frame) != 0) {
        return;
    }

    const size_t sample_count = data.data_size / sizeof(int16_t);
    if (sample_count > CAPTURE_RING_SAMPLES) {
        return;
    }

    std::lock_guard<std::mutex> lock(_capture_callback_mutex);
    if (!_capture_accepting.load(std::memory_order_relaxed)) {
        return;
    }

    const uint32_t write_index = _capture_write_index.load(std::memory_order_relaxed);
    const uint32_t read_index = _capture_read_index.load(std::memory_order_acquire);
    const uint32_t buffered_samples = write_index - read_index;
    if (buffered_samples > CAPTURE_RING_SAMPLES ||
            sample_count > CAPTURE_RING_SAMPLES - buffered_samples) {
        return;
    }

    const uint32_t ring_offset = write_index & (CAPTURE_RING_SAMPLES - 1);
    const size_t first_count = (sample_count < CAPTURE_RING_SAMPLES - ring_offset) ?
                               sample_count : (CAPTURE_RING_SAMPLES - ring_offset);
    std::memcpy(&_capture_ring[ring_offset], data.data_ptr, first_count * sizeof(int16_t));
    if (first_count < sample_count) {
        std::memcpy(
            _capture_ring, data.data_ptr + first_count * sizeof(int16_t),
            (sample_count - first_count) * sizeof(int16_t)
        );
    }
    _capture_write_index.store(write_index + sample_count, std::memory_order_release);
}

bool SpecAnalyzer::closeMicrophone()
{
    _capture_accepting.store(false, std::memory_order_release);
    _recorder_data_connection.disconnect();

    bool stop_succeeded = true;
    if (_audio_encoder_started) {
        auto stop_result = AudioEncoderHelper::call_function_sync(AudioEncoderHelper::FunctionId::Stop);
        if (!stop_result) {
            ESP_UTILS_LOGE("Stop AudioEncoder service failed: %s", stop_result.error().c_str());
            stop_succeeded = false;
        }
        _audio_encoder_started = false;
    }

    _audio_encoder = nullptr;
    _audio_encoder_binding.release();

    {
        std::lock_guard<std::mutex> lock(_capture_callback_mutex);
        _capture_write_index.store(0, std::memory_order_relaxed);
        _capture_read_index.store(0, std::memory_order_relaxed);
    }
    return stop_succeeded;
}

bool SpecAnalyzer::run(void)
{
    ESP_UTILS_LOGD("Run");

    lv_obj_t *scr = lv_scr_act();
    lv_display_t *display = lv_display_get_default();
    if (display == nullptr) {
        ESP_UTILS_LOGE("Default LVGL display is unavailable");
        return false;
    }
    _canvas_width = lv_display_get_horizontal_resolution(display);
    const int display_height = lv_display_get_vertical_resolution(display);
    _canvas_height = (display_height >= 720) ? 360 : (display_height / 2);
    if (_canvas_width <= 0 || _canvas_height <= 0) {
        ESP_UTILS_LOGE("Invalid display resolution: %d x %d", _canvas_width, display_height);
        return false;
    }

    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
    // Prefer PSRAM for the large canvas buffer and fall back to SRAM if PSRAM is exhausted.
    size_t buf_size = static_cast<size_t>(_canvas_width) * _canvas_height * sizeof(lv_color_t);
    _draw_buf = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    
    if (!_draw_buf) {
        ESP_UTILS_LOGE("PSRAM allocation failed! Using fallback SRAM");
        _draw_buf = (lv_color_t*)malloc(buf_size);
    }
    
    if (!_draw_buf) {
        ESP_UTILS_LOGE("Canvas buffer allocation failed!");
        return false;
    }
    
    // Draw into an LVGL canvas so the FFT task only updates numeric buffers and the
    // GUI timer owns all rendering work on the LVGL thread.
    _canvas = lv_canvas_create(lv_scr_act());
    if (!_canvas) {
        ESP_UTILS_LOGE("Canvas creation failed!");
        free(_draw_buf);
        _draw_buf = nullptr;
        return false;
    }
    lv_obj_set_size(_canvas, _canvas_width, _canvas_height);
    lv_obj_align(_canvas, LV_ALIGN_CENTER, 0, 0);
    lv_canvas_set_buffer(_canvas, _draw_buf, _canvas_width, _canvas_height, LV_COLOR_FORMAT_RGB565);
    
    // Initialize the ESP-DSP radix-2 FFT tables once before the audio task starts.
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_UTILS_LOGE("FFT init failed: %d", ret);
        close();
        return false;
    }
    
    // Hann window reduces spectral leakage before the FFT.
    dsps_wind_hann_f32(_wind, N_SAMPLES);
    ESP_UTILS_LOGI("FFT and window initialized");

    if (!openMicrophone()) {
        ESP_UTILS_LOGE("Audio service microphone init failed");
        close();
        return false;
    }

    // Refresh at about 30 FPS; the values are already smoothed by the DSP task.
    _timer = lv_timer_create(timer_cb, 33, this);
    if (!_timer) {
        ESP_UTILS_LOGE("Spectrum timer creation failed!");
        close();
        return false;
    }
    
    // Process service PCM outside the LVGL task and publish only smoothed bins.
    _audio_task_running = true;
    BaseType_t task_ret = xTaskCreatePinnedToCore(
        audio_fft_task, 
        "audio_fft", 
        8 * 1024, 
        this, 
        5, 
        &_audio_task_handle, 
        PRO_CPU_NUM
    );
    if (task_ret != pdPASS) {
        ESP_UTILS_LOGE("Audio FFT task creation failed!");
        _audio_task_running = false;
        close();
        return false;
    }
    
    _mic_label = lv_label_create(scr);
    lv_label_set_text(_mic_label, _audio_task_running ? "Mic 1" : "Mic unavailable");
    lv_obj_set_style_text_color(_mic_label, lv_color_hex(0x00BFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(_mic_label, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_mic_label, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_mic_label, 6, LV_PART_MAIN);
    lv_obj_align_to(_mic_label, _canvas, LV_ALIGN_TOP_LEFT, 20, 12);
    
    return true;
}

bool SpecAnalyzer::back(void)
{
    ESP_UTILS_LOGD("Back");
    notifyCoreClosed();
    return true;
}

bool SpecAnalyzer::close(void)
{
    ESP_UTILS_LOGD("Close");
    
    pause();
    
    if (_timer) {
        lv_timer_del(_timer);
        _timer = nullptr;
    }
    
    if (_canvas) {
        lv_obj_del(_canvas);
        _canvas = nullptr;
    }

    if (_mic_label) {
        lv_obj_del(_mic_label);
        _mic_label = nullptr;
    }
    
    if (_draw_buf) {
        free(_draw_buf);
        _draw_buf = nullptr;
    }
    _canvas_width = 0;
    _canvas_height = 0;
    
    return true;
}

bool SpecAnalyzer::init() { return true; }
bool SpecAnalyzer::deinit() { return true; }
bool SpecAnalyzer::pause()
{
    if (_timer) {
        lv_timer_pause(_timer);
    }

    _audio_task_running = false;
    if (_audio_task_handle != nullptr) {
        for (int i = 0; i < 20; i++) {
            if (_audio_task_handle == nullptr) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (_audio_task_handle != nullptr) {
            vTaskDelete(_audio_task_handle);
            _audio_task_handle = nullptr;
        }
    }

    bool stopped = closeMicrophone();
    if (!stopped) {
        ESP_UTILS_LOGW("Close AudioEncoder service failed");
    }
    if (_mic_label) {
        lv_label_set_text(_mic_label, "Mic paused");
    }
    return stopped;
}

bool SpecAnalyzer::resume()
{
    if (_audio_task_running) {
        if (_timer) {
            lv_timer_resume(_timer);
        }
        return true;
    }

    if (!openMicrophone()) {
        ESP_UTILS_LOGE("Restore AudioEncoder service failed");
        if (_mic_label) {
            lv_label_set_text(_mic_label, "Mic unavailable");
        }
        return false;
    }

    _audio_task_running = true;
    BaseType_t task_ret = xTaskCreatePinnedToCore(
        audio_fft_task,
        "audio_fft",
        8 * 1024,
        this,
        5,
        &_audio_task_handle,
        PRO_CPU_NUM
    );
    if (task_ret != pdPASS) {
        _audio_task_running = false;
        (void)closeMicrophone();
        if (_mic_label) {
            lv_label_set_text(_mic_label, "Mic unavailable");
        }
        return false;
    }

    if (_mic_label) {
        lv_label_set_text(_mic_label, "Mic 1");
    }
    if (_timer) {
        lv_timer_resume(_timer);
    }
    return true;
}

void SpecAnalyzer::audio_fft_task(void *pvParameters)
{
    SpecAnalyzer *app = static_cast<SpecAnalyzer*>(pvParameters);
    constexpr uint32_t capture_frame_samples = N_SAMPLES * CAPTURE_CHANNELS;
    constexpr uint32_t capture_ring_mask = CAPTURE_RING_SAMPLES - 1;
    TickType_t last_peak_log_tick = xTaskGetTickCount();
    
    while (app->_audio_task_running) {
        const uint32_t read_index = app->_capture_read_index.load(std::memory_order_relaxed);
        const uint32_t write_index = app->_capture_write_index.load(std::memory_order_acquire);
        if ((write_index - read_index) < capture_frame_samples) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        for (uint32_t sample = 0; sample < N_SAMPLES; ++sample) {
            // FC is the first slot in the board's interleaved FC,RE capture.
            app->_raw_data[sample] = app->_capture_ring[
                                         (read_index + sample * CAPTURE_CHANNELS) & capture_ring_mask
                                     ];
        }
        app->_capture_read_index.store(read_index + capture_frame_samples, std::memory_order_release);
        
        TickType_t now = xTaskGetTickCount();
        if ((now - last_peak_log_tick) >= pdMS_TO_TICKS(2000)) {
            int32_t mic_peak = 0;
            for (size_t sample = 0; sample < N_SAMPLES; ++sample) {
                int32_t value = static_cast<int32_t>(app->_raw_data[sample]);
                int32_t magnitude = value < 0 ? -value : value;
                if (magnitude > mic_peak) {
                    mic_peak = magnitude;
                }
            }
            ESP_UTILS_LOGI("Mic peak: %d", static_cast<int>(mic_peak));
            last_peak_log_tick = now;
        }

        for (int i = 0; i < N_SAMPLES; ++i) {
            app->_audio_buffer[0][i] = app->_raw_data[i] / 32768.0f;
        }

        // Apply the window in place before constructing the complex FFT input.
        dsps_mul_f32(app->_audio_buffer[0], app->_wind, app->_audio_buffer[0], N_SAMPLES, 1, 1, 1);

        for (int i = 0; i < N_SAMPLES; ++i) {
            app->_fft_buffer[0][2 * i] = app->_audio_buffer[0][i];
            app->_fft_buffer[0][2 * i + 1] = 0;
        }

        dsps_fft2r_fc32(app->_fft_buffer[0], N_SAMPLES);
        dsps_bit_rev_fc32(app->_fft_buffer[0], N_SAMPLES);

        // Convert complex bins to dB. The floor term avoids log10(0).
        for (int i = 0; i < N_SAMPLES / 2; ++i) {
            float real = app->_fft_buffer[0][2 * i];
            float imag = app->_fft_buffer[0][2 * i + 1];
            float magnitude = sqrtf(real * real + imag * imag);
            app->_spectrum[0][i] = 20 * log10f(magnitude / (N_SAMPLES / 2) + 1e-9);
        }

        // Keep the display range at 0-8 kHz.
        constexpr int display_bin_count =
            (DISPLAY_MAX_FREQUENCY_HZ * N_SAMPLES) / MIC_SAMPLE_RATE;

        // Map FFT bins onto fewer visual bars and smooth the change over time.
        for (int i = 0; i < STRIPE_COUNT; ++i) {
            int fft_idx = i * display_bin_count / STRIPE_COUNT;
            float boosted_db = app->_spectrum[0][fft_idx] + 10.0f;
            app->_display_spectrum[0][i] = fmaxf(-90.0f, fminf(0.0f, boosted_db));

            const float SMOOTH_FACTOR = 0.2f;
            app->_smooth_spectrum[0][i] = app->_smooth_spectrum[0][i] * (1.0f - SMOOTH_FACTOR) +
                                          app->_display_spectrum[0][i] * SMOOTH_FACTOR;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    app->_audio_task_handle = nullptr;
    vTaskDelete(NULL);
}

// LVGL timer callback. It renders the smoothed FFT bins and peak markers into the canvas.
void SpecAnalyzer::timer_cb(lv_timer_t *timer)
{
    SpecAnalyzer *app = static_cast<SpecAnalyzer*>(lv_timer_get_user_data(timer));
    if (!app || !app->_canvas) return;
    
    lv_layer_t layer;
    lv_canvas_init_layer(app->_canvas, &layer);
    lv_canvas_fill_bg(app->_canvas, lv_color_black(), LV_OPA_COVER);
    
    const int side_margin = 20;
    const int channel_gap = (app->MIC_COUNT > 1) ? 24 : 0;
    const int channel_width =
        (app->_canvas_width - side_margin * 2 - channel_gap * (app->MIC_COUNT - 1)) / app->MIC_COUNT;
    const int bar_gap = 2;
    int bar_width = channel_width / app->STRIPE_COUNT - bar_gap;
    if (bar_width < 1) {
        bar_width = 1;
    }
    const int bars_width = app->STRIPE_COUNT * bar_width + (app->STRIPE_COUNT - 1) * bar_gap;
    const int base_y = app->_canvas_height - 20;                 // Bottom margin
    const int max_bar_height = app->_canvas_height - 50;
    const int BASE_BAR_HEIGHT = 5;
    
    for (int ch = 0; ch < app->MIC_COUNT; ch++) {
        int channel_offset_x = side_margin + ch * (channel_width + channel_gap) +
                               (channel_width - bars_width) / 2;
        
        for (int i = 0; i < app->STRIPE_COUNT; i++) {
            // Normalize smoothed dB values from [-90, 0] to [0, 1].
            float db = app->_smooth_spectrum[ch][i];
            float norm = (db + 90.0f) / 90.0f;
            norm = fmaxf(0.0f, fminf(1.0f, norm));
            
            // Nonlinear mapping keeps quiet audio visible while preserving peaks.
            int bar_height = (int)(powf(norm, 0.8) * max_bar_height);
            if (bar_height < BASE_BAR_HEIGHT) {
                bar_height = BASE_BAR_HEIGHT;
            }
            
            // Peak markers rise immediately and fall faster when they are higher,
            // which gives the analyzer a natural decay without another timer.
            if (app->_peak[ch][i] < bar_height) {
                app->_peak[ch][i] = bar_height;
            } else {
                float peak_norm = app->_peak[ch][i] / (float)max_bar_height;
                float fall_speed = 0.16f + (peak_norm * 0.4f);
                app->_peak[ch][i] -= fall_speed;
                
                if (app->_peak[ch][i] < BASE_BAR_HEIGHT) {
                    app->_peak[ch][i] = BASE_BAR_HEIGHT;
                }
            }
            
            int bar_x1 = channel_offset_x + (i * (bar_width + bar_gap));
            int bar_x2 = bar_x1 + bar_width;
            int bar_y1 = base_y - bar_height;
            int bar_y2 = base_y;
            
            // Assign a slightly different hue per bin so adjacent bars remain readable.
            float hue_base = 190.0f;
            float hue_step = 80.0f / app->STRIPE_COUNT;
            uint16_t hue = (uint16_t)(hue_base + i * hue_step);
            lv_color_t main_color = lv_color_hsv_to_rgb(hue, 100, 100);
            lv_color_t light_color = lv_color_hsv_to_rgb(hue, 80, 100);
            lv_color_t dark_color = lv_color_hsv_to_rgb(hue, 100, 70);
            
            lv_draw_rect_dsc_t bar_dsc;
            lv_draw_rect_dsc_init(&bar_dsc);
            bar_dsc.bg_opa = LV_OPA_COVER;
            
            bar_dsc.bg_color = main_color;
            lv_area_t bar_main_area = {
                .x1 = bar_x1, .y1 = bar_y1 + (int)(bar_height * 0.2),
                .x2 = bar_x2, .y2 = bar_y2
            };
            lv_draw_rect(&layer, &bar_dsc, &bar_main_area);
            
            bar_dsc.bg_color = light_color;
            lv_area_t bar_top_area = {
                .x1 = bar_x1, .y1 = bar_y1,
                .x2 = bar_x2, .y2 = bar_y1 + (int)(bar_height * 0.2)
            };
            lv_draw_rect(&layer, &bar_dsc, &bar_top_area);
            
            bar_dsc.bg_color = dark_color;
            lv_area_t bar_bottom_area = {
                .x1 = bar_x1, .y1 = bar_y2 - 1,
                .x2 = bar_x2, .y2 = bar_y2
            };
            lv_draw_rect(&layer, &bar_dsc, &bar_bottom_area);
            
            int peak_y = base_y - (int)app->_peak[ch][i];
            
            lv_draw_rect_dsc_t peak_dsc;
            lv_draw_rect_dsc_init(&peak_dsc);
            uint16_t peak_hue = (uint16_t)(hue_base + i * hue_step + 20);
            peak_dsc.bg_color = lv_color_hsv_to_rgb(peak_hue, 100, 100);
            peak_dsc.bg_opa = LV_OPA_90;
            
            lv_area_t peak_area = {
                .x1 = bar_x1 + 1, .y1 = peak_y - 2,
                .x2 = bar_x2 - 1, .y2 = peak_y
            };
            lv_draw_rect(&layer, &peak_dsc, &peak_area);
        }
    }
    
    lv_canvas_finish_layer(app->_canvas, &layer);
}

} // namespace esp_brookesia::apps
