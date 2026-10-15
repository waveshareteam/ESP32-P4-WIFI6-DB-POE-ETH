# esp_hal_boards

该组件为当前 ESP Board Manager 提供 Brookesia 音频 HAL，并封装板级 LCD/LVGL 初始化；目标板默认适配 Waveshare ESP32-P4-WIFI6-DB-POE-ETH。音频接口实例名与 `brookesia_service_audio` 一致：

- `Audio:CodecPlayer`
- `Audio:CodecRecorder`
- `Audio:Playback`
- `Audio:Encoder:0`
- `Audio:Decoder:0`

Board Manager 负责板级 `audio_dac`、`audio_adc`、I2S、I2C 和 `gpio_pa_control` 配置。默认硬件格式为 16 kHz、16 bit、双通道；codec ADC 标签为 `FC,RE`，AFE 输入布局为 `MR`，输入增益为 36 dB。组件注册插件时不会初始化音频设备，首次获取任一 HAL 接口时才初始化 Board Manager codec 设备。

## LCD 初始化

`DisplayDevice::get_instance().init()` 按设备名初始化 Board Manager 的 `display_lcd` 和可选 `lcd_touch`，验证 DSI、RGB565 格式及 framebuffer 配置，然后将面板和触摸设备注册到 `esp_lvgl_adapter` 并启动 adapter。重复调用会返回已注册的 `lv_display_t`。失败时会释放本次成功初始化的 Board Manager 设备引用；LVGL 访问仍由应用通过 `esp_lv_adapter_lock()` / `esp_lv_adapter_unlock()` 保护。背光通过 `display_backlight_set()` 按需控制，该函数首次调用时初始化 `lcd_brightness` 设备，并将百分比限制在 0-100。

```cpp
#include "esp_hal_boards/display_device.hpp"

lv_display_t *display = esp_hal_boards::DisplayDevice::get_instance().init();
esp_err_t err = esp_hal_boards::display_backlight_set(100);
```

## 配置

在 `menuconfig` 的 `ESP HAL Boards` 下启用 `Board Manager audio HAL provider`。该选项要求官方 `BROOKESIA_HAL_ADAPTOR_ENABLE_AUDIO_DEVICE` 关闭，以免重复注册和触发旧 Board Manager 依赖。`Audio playback, encoder and decoder interfaces` 默认开启，并使用 `av_processor 0.6.*`。

如需启用 VAD 或 WakeNet，应在第一次获取任何音频 HAL 接口前设置 processor 配置：

```cpp
#include "esp_hal_boards/audio_device.hpp"
#include <utility>

namespace hal = esp_brookesia::hal;
namespace audio = esp_brookesia::hal::audio;

auto &device = esp_hal_boards::AudioDevice::get_instance();

auto recorder_info = audio::CodecRecorderIface::Info {
    .bits = 16,
    .channels = 2,
    .sample_rate = 16000,
    .mic_layout = "FC,RE",  // ES8311 hardware ADC label
    .general_gain = 36.0f,
    .channel_gains = {},
};
device.set_codec_recorder_info(std::move(recorder_info));
device.set_afe_input_layout("MR");

hal::AudioProcessorConfig processor_config;
processor_config.afe.vad = hal::AudioProcessorAFE_VAD_Config{};
processor_config.afe.wakenet = hal::AudioProcessorAFE_WakeNetConfig {
    .model_partition_label = "model",
    .mn_language = "cn",
};
device.set_processor_config(std::move(processor_config));
```

配置调用返回 `false` 时表示参数无效或 provider 已经被 HAL 接口初始化；此时应在初始化前完成配置。只有启用了 AFE 且配置了 WakeNet 的编码任务会检查 `model` 分区。缺少该分区时编码启动会报错。

应用继续通过 `brookesia_service_audio` 使用播放、编码和解码，不需要替换服务。service 的 HAL 查找名称与上面五个名称匹配。若直接访问 HAL，可使用：

```cpp
auto playback = hal::acquire_interface<audio::PlaybackIface>(
    audio::PlaybackIface::get_default_instance_name()
);
auto encoder = hal::acquire_interface<audio::EncoderIface>(
    audio::EncoderIface::get_default_instance_name(0)
);
```

## 生命周期与验证

- 播放处理打开 codec 时先开 DAC，再开 ADC；关闭时先停 recorder/ADC，再关闭 DAC。
- ADC/DAC 格式不匹配会明确失败。打开失败会回滚已经打开的 codec，HAL 设备释放时只归还本组件初始化的 Board Manager 设备引用。
- 可在应用中分别启动/停止 playback、PCM/Opus encoder 和 decoder，验证重复启停、PA 开关、音量、输入增益和配置错误后的重试。
- AFE 验证需提供带有效模型的 `model` 分区，并检查 VAD/WakeNet 回调。仅有分区表项但没有可用模型时，ESP-SR/av_processor 仍会在运行期报告模型加载错误。

尚需在目标工程完成编译和板上验证。构建、烧录或监视串口请使用工程自身的 ESP-IDF 流程。
