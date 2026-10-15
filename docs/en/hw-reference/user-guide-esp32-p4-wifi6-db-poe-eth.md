# ESP32-P4-WIFI6-DB-POE-ETH

[中文版本](../../zh_CN/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md)

ESP32-P4-WIFI6-DB-POE-ETH is a board built around the ESP32-P4 application
processor (two high-performance RISC-V cores and one low-power core), paired with
an ESP32-C5 Wi-Fi 6 / Bluetooth 5 (LE) coprocessor over SDIO. The board adds
an RMII Ethernet interface, a MIPI-DSI display interface that can be paired
with a touch panel, and a MIPI-CSI camera interface. The PoE module is
integrated on the board. To power the board over PoE, connect it to a router or
switch that supports the IEEE 802.3af PoE standard.

<!-- image placeholder: docs/_static/esp32-p4-wifi6-db-poe-eth-board.png (board photo, top view) -->

## Onboard Resources

- **Main processor**: ESP32-P4NRW32X, with two high-performance RISC-V cores and one low-power core.
- **Flash**: 32 MB, QIO mode.
- **PSRAM**: Package-integrated PSRAM, operating at 200 MHz; check the capacity for the specific hardware revision.
- **Wireless coprocessor**: ESP32-C5, supporting Wi-Fi 6 and Bluetooth 5 LE over 4-bit SDIO.
- **Ethernet**: RMII MAC/PHY, PHY address 1, with a 50 MHz external REF_CLK input.
- **Display**: Two-lane MIPI-DSI. Compatible panels include HX8394 (5", 720x1280), ILI9881C (7", 720x1280), and JD9365 (8"/10.1", 800x1280). This guide uses the 10.1-inch JD9365 as its default configuration. The LCD reset pin is not connected; the backlight is controlled over I2C.
- **Touch**: GT911 capacitive touch controller on the shared I2C bus. Touch status is read by polling; reset and interrupt signals are not connected.
- **Audio**: ES8311 codec, speaker output + one analog microphone input.
- **Storage**: microSD over 4-bit SDMMC, with power-enable control.
- **Camera**: Two-lane MIPI-CSI; supports OV5647 and SC2336 sensors. Confirm the camera module's clock and reset requirements against its documentation and the board revision.
- **USB**: USB 2.0 Host interface connected to the ESP32-P4's internal USB PHY (DM/DP), with over-current protection.
- **Expansion header**: Provides multiple GPIOs. GPIO45 is shared with microSD power control, GPIO53 with speaker amplifier enable, and GPIO6 wakes the ESP32-C5. See the pin information below.

<!-- image placeholder: docs/_static/esp32-p4-wifi6-db-poe-eth-connectors.png (connector/port labels) -->

## Peripheral Quick Reference

| Module | Device / Function | Interface | Address / Parameters | GPIO / Signal |
| --- | --- | --- | --- | --- |
| LCD | MIPI-DSI panel | MIPI-DSI | 2-lane; HX8394/ILI9881C/JD9365 | LCD reset pin not connected; backlight over shared I2C |
| Touch | GT911 capacitive touch | I2C | 7-bit address `0x5D` or `0x14`, depending on controller configuration | SDA=GPIO7, SCL=GPIO8; RST/INT not wired; touch status read by polling |
| LCD backlight | Backlight controller | I2C | 7-bit address `0x45` | SDA=GPIO7, SCL=GPIO8 |
| Camera | MIPI-CSI camera connector | MIPI-CSI + SCCB | 2-lane; OV5647 and SC2336 supported; address per module | SCCB shares GPIO7/GPIO8; confirm clock and reset requirements for the module and board revision |
| Audio | ES8311 codec | I2C + I2S | I2C 7-bit address `0x18`; one analog microphone | I2C: SDA=GPIO7, SCL=GPIO8; MCLK=GPIO13, SCLK=GPIO12, WS=GPIO10, DOUT=GPIO9, DIN=GPIO11 |
| Power amp | Onboard speaker amp enable | GPIO | Active high | GPIO53 |
| microSD | microSD card | SDMMC | 4-bit mode | CLK=GPIO43, CMD=GPIO44, D0=GPIO39, D1=GPIO40, D2=GPIO41, D3=GPIO42; power enable=GPIO45 (active low) |
| Ethernet | RMII PHY | RMII + SMI | PHY address 1; 50 MHz external REF_CLK input | CRS_DV=GPIO28, RXD0=GPIO29, RXD1=GPIO30, MDC=GPIO31, TXD0=GPIO34, TXD1=GPIO35, TX_EN=GPIO49, REF_CLK=GPIO50, PHY RESET=GPIO51, MDIO=GPIO52 |
| Wi-Fi/BT coprocessor | ESP32-C5 | SDIO | 4-bit, 40 MHz | CLK=GPIO18, CMD=GPIO19, D0=GPIO14, D1=GPIO15, D2=GPIO16, D3=GPIO17, RESET=GPIO54 (active low) |
| USB | USB Host | USB 2.0 | Internal USB PHY, with over-current protection | D+/D- wired directly to ESP32-P4 DM/DP |

## Pin Definition

### Expansion Header

The expansion header provides the GPIO pins listed below. GPIO6 also wakes the
ESP32-C5, GPIO45 is shared with microSD power control, and GPIO53 is shared
with the speaker amplifier enable. See Usage Notes before connecting external circuits.

| Type | Signal |
| --- | --- |
| GPIO | GPIO2, GPIO3, GPIO4, GPIO5, GPIO6\*, GPIO20, GPIO21, GPIO22, GPIO23, GPIO24, GPIO25, GPIO26, GPIO27, GPIO32, GPIO33, GPIO36, GPIO45\*, GPIO46, GPIO47, GPIO48, GPIO53\* |

\* GPIO6 also wakes the ESP32-C5; GPIO45 is the microSD power-enable pin; and
GPIO53 is the speaker amplifier enable pin. Do not drive these pins independently
while their corresponding functions are in use.

<!-- image placeholder: docs/_static/esp32-p4-wifi6-db-poe-eth-pinout.png (full pinout diagram) -->

## GPIO Pin Functions

The table below lists the ESP32-P4 GPIO signal functions and their onboard
connections. Check the board pinout for pin multiplexing and expansion-header locations.

| GPIO | Signal | Connected To | Notes |
| --- | --- | --- | --- |
| GPIO0 | XTAL_32K_N | 32.768 kHz crystal | Shares pin functions with GPIO; check and account for the crystal circuit before using it for another purpose |
| GPIO1 | XTAL_32K_P | 32.768 kHz crystal | Shares pin functions with GPIO; check and account for the crystal circuit before using it for another purpose |
| GPIO2 | GPIO2 | Expansion header | Available as a general-purpose GPIO |
| GPIO3 | GPIO3 | Expansion header | Available as a general-purpose GPIO |
| GPIO4 | GPIO4 | Expansion header | Available as a general-purpose GPIO |
| GPIO5 | GPIO5 | Expansion header | Available as a general-purpose GPIO |
| GPIO6 | GPIO6 | ESP32-C5 wake / expansion header | Wakes the ESP32-C5; check that external circuits do not interfere with the wake signal |
| GPIO7 | I2C SDA | ES8311, GT911, LCD backlight controller, camera SCCB | Shared I2C data line |
| GPIO8 | I2C SCL | ES8311, GT911, LCD backlight controller, camera SCCB | Shared I2C clock line |
| GPIO9 | I2S DOUT | ES8311 DSDIN | Audio playback data, ESP32-P4 -> codec |
| GPIO10 | I2S WS | ES8311 LRCK | Audio frame sync |
| GPIO11 | I2S DIN | ES8311 ASDOUT | Audio capture data, codec -> ESP32-P4 |
| GPIO12 | I2S SCLK | ES8311 SCLK | Audio bit clock |
| GPIO13 | I2S MCLK | ES8311 MCLK | Audio master clock |
| GPIO14 | SDIO D0 | ESP32-C5 | SDIO data line |
| GPIO15 | SDIO D1 | ESP32-C5 | SDIO data line |
| GPIO16 | SDIO D2 | ESP32-C5 | SDIO data line |
| GPIO17 | SDIO D3 | ESP32-C5 | SDIO data line |
| GPIO18 | SDIO CLK | ESP32-C5 | SDIO clock |
| GPIO19 | SDIO CMD | ESP32-C5 | SDIO command line |
| GPIO20 | GPIO20 | Expansion header | Available as a general-purpose GPIO |
| GPIO21 | GPIO21 | Expansion header | Available as a general-purpose GPIO |
| GPIO22 | GPIO22 | Expansion header | Available as a general-purpose GPIO |
| GPIO23 | GPIO23 | Expansion header | Available as a general-purpose GPIO |
| GPIO24 | GPIO24 | Expansion header | Available as a general-purpose GPIO |
| GPIO25 | GPIO25 | Expansion header | Available as a general-purpose GPIO |
| GPIO26 | GPIO26 | Expansion header | Available as a general-purpose GPIO |
| GPIO27 | GPIO27 | Expansion header | Available as a general-purpose GPIO |
| GPIO28 | RMII CRS_DV | Ethernet PHY | Carrier sense/RX data valid, PHY -> ESP32-P4 (input) |
| GPIO29 | RMII RXD0 | Ethernet PHY | Receive data bit 0, PHY -> ESP32-P4 (input) |
| GPIO30 | RMII RXD1 | Ethernet PHY | Receive data bit 1, PHY -> ESP32-P4 (input) |
| GPIO31 | SMI MDC | Ethernet PHY | PHY management clock, ESP32-P4 -> PHY (output) |
| GPIO32 | GPIO32 | Expansion header | Available as a general-purpose GPIO |
| GPIO33 | GPIO33 | Expansion header | Available as a general-purpose GPIO |
| GPIO34 | RMII TXD0 | Ethernet PHY | Transmit data bit 0, ESP32-P4 -> PHY (output); also sampled at startup/reset |
| GPIO35 | RMII TXD1 | Ethernet PHY | Transmit data bit 1, ESP32-P4 -> PHY (output); also sampled at startup/reset |
| GPIO36 | GPIO36 | Expansion header | Available as a general-purpose GPIO; also sampled at startup/reset |
| GPIO37 | UART0_TXD | Download/debug UART | UART0 transmit signal, used for flashing and serial logs; also sampled at startup/reset |
| GPIO38 | UART0_RXD | Download/debug UART | UART0 receive signal, used for flashing and serial logs; also sampled at startup/reset |
| GPIO39 | SD D0 | microSD card | SDMMC 4-bit data line |
| GPIO40 | SD D1 | microSD card | SDMMC 4-bit data line |
| GPIO41 | SD D2 | microSD card | SDMMC 4-bit data line |
| GPIO42 | SD D3 | microSD card | SDMMC 4-bit data line |
| GPIO43 | SD CLK | microSD card | SDMMC clock |
| GPIO44 | SD CMD | microSD card | SDMMC command |
| GPIO45 | SD_VDD_EN | microSD power control / expansion header | Active low; avoid driving as a plain GPIO while the SD card is in use |
| GPIO46 | GPIO46 | Expansion header | Available as a general-purpose GPIO |
| GPIO47 | GPIO47 | Expansion header | Available as a general-purpose GPIO |
| GPIO48 | GPIO48 | Expansion header | Available as a general-purpose GPIO |
| GPIO49 | RMII TX_EN | Ethernet PHY | Transmit enable, ESP32-P4 -> PHY (output) |
| GPIO50 | RMII REF_CLK | Ethernet PHY | 50 MHz RMII reference clock, PHY -> ESP32-P4 (input) |
| GPIO51 | PHY RESET | Ethernet PHY | Hardware reset control, ESP32-P4 -> PHY (GPIO output) |
| GPIO52 | SMI MDIO | Ethernet PHY | PHY management data, bidirectional |
| GPIO53 | PA_CTRL | Speaker amplifier enable / expansion header | Active high; avoid driving as a plain GPIO while audio playback is in use |
| GPIO54 | C5_RESET | ESP32-C5 reset | Active low reset for the Wi-Fi/BT coprocessor |

## Usage Notes

- GPIO7/GPIO8 form the shared I2C bus for ES8311, GT911, the LCD backlight
  controller, and the camera SCCB interface. Check for address conflicts
  before adding another I2C device.
- GPIO45 controls microSD power, GPIO53 controls the audio power amplifier,
  GPIO54 resets the ESP32-C5, and GPIO6 wakes the ESP32-C5. Avoid driving
  these independently while the corresponding peripheral is active.
- GPIO28–GPIO31, GPIO34–GPIO35, and GPIO49–GPIO52 are used for Ethernet
  RMII/SMI signaling and PHY reset; avoid repurposing them as plain GPIO.
- GPIO34–GPIO38 are ESP32-P4 strapping pins sampled during power-on or reset.
  GPIO34/GPIO35 also carry Ethernet TXD0/TXD1, while GPIO37/GPIO38 carry UART0
  TX/RX for flashing and serial logs. After reset is released, these pins can
  serve their other functions; external circuits must not disturb their levels
  during sampling.
- GPIO0/GPIO1 connect to XTAL_32K_N/XTAL_32K_P of the onboard 32.768 kHz
  crystal, used for the ESP32-P4 RTC slow clock. Check and account for the
  crystal circuit before using either pin for another purpose.
- LCD reset, GT911 reset, and GT911 interrupt signals are not connected;
  touch status is read over I2C by polling.
- Check the board pinout for the physical locations of expansion-header pins.

## Board Dimensions

<!-- image placeholder: docs/_static/esp32-p4-wifi6-db-poe-eth-dimensions.png (mechanical drawing) -->

Board dimensions will be added when the hardware drawing is available.
