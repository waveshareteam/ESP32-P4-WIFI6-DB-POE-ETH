#pragma once

#include "lvgl.h"
#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "esp_dsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "brookesia/lib_utils/signal.hpp"
#include "brookesia/service_audio/service_audio.hpp"
#include "brookesia/service_manager/service/manager.hpp"

#include <atomic>
#include <mutex>

namespace esp_brookesia::apps
{
    class SpecAnalyzer : public systems::phone::App
    {
    public:
        static SpecAnalyzer *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~SpecAnalyzer();

        using systems::phone::App::endRecordResource;
        using systems::phone::App::startRecordResource;

    protected:
        SpecAnalyzer(bool use_status_bar, bool use_navigation_bar);

        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        static SpecAnalyzer *_instance;

        // The board audio service captures 48 kHz, 16-bit stereo frames (FC,RE).
        static constexpr uint16_t N_SAMPLES = 1024;
        static constexpr uint32_t MIC_SAMPLE_RATE = 48000;
        static constexpr uint32_t DISPLAY_MAX_FREQUENCY_HZ = 8000;
        static constexpr uint16_t MIC_COUNT = 1;
        static constexpr uint16_t CAPTURE_CHANNELS = 2;
        static constexpr uint32_t CAPTURE_RING_SAMPLES = N_SAMPLES * 16;
        static constexpr uint16_t STRIPE_COUNT = 48;

        lv_obj_t *_canvas;
        lv_obj_t *_mic_label;
        lv_timer_t *_timer;
        TaskHandle_t _audio_task_handle;
        bool _audio_task_running;
        service::ServiceBinding _audio_encoder_binding;
        service::AudioEncoder *_audio_encoder;
        esp_brookesia::lib_utils::scoped_connection _recorder_data_connection;
        bool _audio_encoder_started;
        std::atomic_bool _capture_accepting;
        std::atomic<uint32_t> _capture_write_index;
        std::atomic<uint32_t> _capture_read_index;
        std::mutex _capture_callback_mutex;
        __attribute__((aligned(16))) int16_t _capture_ring[CAPTURE_RING_SAMPLES];
        int _canvas_width;
        int _canvas_height;

        // Mono microphone capture and spectrum buffers.
        __attribute__((aligned(16))) int16_t _raw_data[N_SAMPLES];
        __attribute__((aligned(16))) float _audio_buffer[MIC_COUNT][N_SAMPLES];
        __attribute__((aligned(16))) float _wind[N_SAMPLES];
        __attribute__((aligned(16))) float _fft_buffer[MIC_COUNT][N_SAMPLES * 2];
        __attribute__((aligned(16))) float _spectrum[MIC_COUNT][N_SAMPLES / 2];
        float _display_spectrum[MIC_COUNT][STRIPE_COUNT];
        float _peak[MIC_COUNT][STRIPE_COUNT];
        float _smooth_spectrum[MIC_COUNT][STRIPE_COUNT];

        lv_color_t *_draw_buf;

        bool openMicrophone();
        bool closeMicrophone();
        void onRecorderData(const service::RawBuffer &data);
        static void audio_fft_task(void *pvParameters);
        static void timer_cb(lv_timer_t *timer);
    };
}
