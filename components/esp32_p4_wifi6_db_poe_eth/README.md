# BSP: ESP32-P4-WIFI6-DB-POE-ETH

[中文说明](README_zh.md)

## Overview

This component provides the board support package for ESP32-P4-WIFI6-DB-POE-ETH.
The current manifest version is `0.0.1`.

Supported board interfaces:

- SD card over 4-bit SDMMC.
- ES8311 playback and one analog microphone input.
- Shared I2C bus.
- RMII Ethernet MAC/PHY initialization, using the generic IEEE 802.3 PHY driver.
- ESP32-C5 Wi-Fi wiring.
- Selectable two-lane MIPI-DSI display using JD9365, ILI9881C, or HX8394.
- GT911 capacitive touch controller.
- MIPI-CSI camera through `esp_video`.
- USB Host.
- Expansion-header GPIO enumeration.

The BSP does not configure an external RTC or an LCD reset GPIO. The LCD
backlight is controlled through the shared I2C bus rather than a dedicated
GPIO. The audio power amplifier is enabled by GPIO53. The GT911 reset and
interrupt GPIOs are configured as unused. Display/touch support is retained
from the original BSP, including all four panel options. USB and CSI code is
also retained; the current peripheral examples do not validate those interfaces.

## Pin Assignment

### I2C

| Signal | GPIO |
| --- | ---: |
| SDA | 7 |
| SCL | 8 |

The ES8311, GT911, LCD backlight controller, and camera SCCB bus share this I2C
bus.

### ES8311

| ES8311 signal | ESP32-P4 signal | GPIO |
| --- | --- | ---: |
| MCLK | I2S MCLK | 13 |
| SCLK | I2S BCLK | 12 |
| LRCK | I2S WS | 10 |
| DSDIN | I2S DOUT | 9 |
| ASDOUT | I2S DIN | 11 |
| Power amplifier enable | GPIO | 53 |

`bsp_audio_codec_speaker_init()` and
`bsp_audio_codec_microphone_init()` both create ES8311 codec devices. The
microphone path is configured for one analog microphone.
The default I2S configuration remains 48 kHz, 16-bit, mono duplex, with a
256x MCLK. `bsp_audio_init()` also accepts an application configuration.
`no_dac_ref=true` is retained, so a stereo capture does not fill the right
channel with the DAC reference signal.

### SD Card

| Signal | GPIO |
| --- | ---: |
| D0 | 39 |
| D1 | 40 |
| D2 | 41 |
| D3 | 42 |
| CMD | 44 |
| CLK | 43 |
| Power enable | 45, active low |

`bsp_sdcard_mount()` controls the card power-enable pin and starts SDMMC with
the existing 4-bit wiring. On ESP32-P4, SDMMC IO voltage is controlled through
on-chip LDO channel 4. The BSP also enables the SDMMC internal pull-ups;
external pull-ups are still required for normal signal integrity.
Before mounting, GPIO45 is driven high for 100 ms and then low, matching
`02_sdmmc`. A second mount returns `ESP_ERR_INVALID_STATE` without cycling power.
The card uses slot 0; Hosted Wi-Fi uses slot 1.

### Ethernet and Wi-Fi

| Interface | Signal | GPIO / value |
| --- | --- | --- |
| Ethernet | MDC / MDIO / PHY reset | 31 / 52 / 51 |
| Ethernet | PHY address | 1 |
| RMII | 50 MHz reference clock input | 50 |
| RMII | TX_EN / TXD0 / TXD1 | 49 / 34 / 35 |
| RMII | CRS_DV / RXD0 / RXD1 | 28 / 29 / 30 |
| ESP32-C5 SDIO | CLK / CMD | 18 / 19 |
| ESP32-C5 SDIO | D0 / D1 / D2 / D3 | 14 / 15 / 16 / 17 |
| ESP32-C5 | Reset, active low | 54 |
| ESP32-C5 SDIO | Slot / width / clock | 1 / 4 bits / 40 MHz |

These values come from the configured `06_eth2ap` and `07_ethernet_basic`
examples and their ESP-IDF 6.0.1 EMAC defaults. The bridge example selects
IP101; the BSP uses the generic PHY implementation, as in `07_ethernet_basic`.
The Ethernet API was also checked against installed ESP-IDF 5.5.4 headers.

### Display

Select the connected panel in `menuconfig`. The 10.1-inch JD9365 panel is the
default.

| Panel option | Controller | Resolution | DSI lane bit rate |
| --- | --- | ---: | ---: |
| Waveshare 5-DSI-TOUCH-A | HX8394 | 720x1280 | 700 Mbps |
| Waveshare 7-DSI-TOUCH-A | ILI9881C | 720x1280 | 1000 Mbps |
| Waveshare 8-DSI-TOUCH-A | JD9365 | 800x1280 | 1500 Mbps |
| Waveshare 10.1-DSI-TOUCH-A | JD9365 | 800x1280 | 1500 Mbps |

All four options use two DSI data lanes. The LCD reset pin is `GPIO_NUM_NC`,
and the board has no dedicated LCD backlight GPIO.

Backlight brightness is controlled on the shared I2C bus:

| Item | Value |
| --- | --- |
| I2C address | `0x45` |
| Brightness register | `0x96` |
| Brightness data | `0..255`, scaled from `0..100%` |

`bsp_display_brightness_set()` sends `{0x96, data}` where
`data = 255 * brightness_percent / 100`.

### Touch

The GT911 uses the shared I2C bus on GPIO7/GPIO8. Its reset and interrupt GPIOs
are both `GPIO_NUM_NC`, so the driver polls the controller. `bsp_touch_new()`
tries both valid GT911 addresses, `0x5D` and `0x14`.

`bsp_display_start()` and `bsp_display_start_with_config()` automatically
create the GT911 device and register it with the LVGL adapter.

### Expansion Header

`bsp_get_header_gpios()` returns the following BSP-owned, read-only array:

```text
23, 5, 20, 21, 25, 26, 32, 4, 22, 24, 27, 33, 36, 3, 2, 54,
47, 46, 45, 6, 53, 48
```

GPIO45 and GPIO53 are also used by the BSP for SD-card power control and
ES8311 power-amplifier enable, respectively. GPIO54 resets ESP32-C5.
The array is preserved from the original BSP as requested; it is not a list
of currently free pins. Do not drive these shared pins independently
while those peripherals are active.

## Configuration

Use `menuconfig` to select:

- I2C controller and 100/400 kHz bus speed.
- I2S controller.
- Connected MIPI-DSI panel type.
- RGB565 or RGB888 display format.
- One to three MIPI-DPI frame buffers.
- SD card and SPIFFS mount options.

## Basic Usage

Add this directory to the application's `EXTRA_COMPONENT_DIRS` before
including ESP-IDF's `project.cmake`. Include `bsp/esp-bsp.h` or
`bsp/esp32_p4_wifi6_db_poe_eth.h`; the copied Nano-specific filename has been
replaced. Existing `bsp_*` display/audio/storage APIs are retained.

```c
#include "bsp/esp-bsp.h"

ESP_ERROR_CHECK(bsp_i2c_init());

esp_codec_dev_handle_t playback = bsp_audio_codec_speaker_init();
esp_codec_dev_handle_t microphone = bsp_audio_codec_microphone_init();

esp_lcd_panel_handle_t panel = NULL;
esp_lcd_panel_io_handle_t io = NULL;
ESP_ERROR_CHECK(bsp_display_new(NULL, &panel, &io));
ESP_ERROR_CHECK(bsp_display_brightness_set(50));

esp_lcd_touch_handle_t touch = NULL;
ESP_ERROR_CHECK(bsp_touch_new(NULL, &touch));
```

### Ethernet lifecycle

Enable `CONFIG_ETH_USE_ESP32_EMAC`. `bsp_eth_init()` installs the driver only;
the application owns its event loop, netif, DHCP/static addressing and start/stop:

```c
#include <assert.h>
#include "bsp/ethernet.h"
#include "esp_event.h"
#include "esp_netif.h"

// Initialize these shared services once in the application.
ESP_ERROR_CHECK(esp_netif_init());
ESP_ERROR_CHECK(esp_event_loop_create_default());
esp_eth_handle_t eth = NULL;
ESP_ERROR_CHECK(bsp_eth_init(&eth));
esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
esp_netif_t *netif = esp_netif_new(&cfg);
assert(netif);
esp_eth_netif_glue_handle_t glue = esp_eth_new_netif_glue(eth);
assert(glue);
ESP_ERROR_CHECK(esp_netif_attach(netif, glue));
ESP_ERROR_CHECK(esp_eth_start(eth));

// After all users have finished with the interface:
ESP_ERROR_CHECK(esp_eth_stop(eth));
ESP_ERROR_CHECK(esp_eth_del_netif_glue(glue));
esp_netif_destroy(netif);
ESP_ERROR_CHECK(bsp_eth_deinit(eth));
```

Do not initialize the same EMAC again using `ethernet_init_all()` or the
standalone example's initializer. When EMAC is disabled, the BSP Ethernet
functions return `ESP_ERR_NOT_SUPPORTED` for valid arguments.

### Wi-Fi configuration

Wi-Fi uses the application's `esp_hosted` and `esp_wifi_remote` components,
as in `03_wifistation` / `06_eth2ap`; the BSP does not create a second transport.
The existing bridge lockfile resolves Hosted 3.0.7 and Wi-Fi Remote 1.6.4.
No component packages or dependency versions were changed by this adaptation.

Configure Hosted in the application, referring to `03_wifistation` / `06_eth2ap`.
Verify the generated configuration
selects ESP32-C5, SDIO slot 1, and the pins above. The reset polarity choice
derives `CONFIG_ESP_HOSTED_HOST_RESET_ACTIVE_LOW=y` in the referenced Hosted version.
Use the matching Hosted firmware on ESP32-C5; STA/AP policy and credentials
remain application settings.

This checkout has been statically reviewed against the board configuration.
Build, flash, and feature-specific hardware checks are still required for
Ethernet, Wi-Fi, display output, audio playback/capture, SD card operation, USB, CSI, touch
input, and electrical behavior.
