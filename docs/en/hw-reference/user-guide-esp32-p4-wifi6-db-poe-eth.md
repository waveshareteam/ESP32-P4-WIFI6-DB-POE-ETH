# ESP32-P4-WIFI6-DB-POE-ETH

[中文版本](../../zh_CN/hw-reference/user-guide-esp32-p4-wifi6-db-poe-eth.md)

ESP32-P4-WIFI6-DB-POE-ETH is a board built around the ESP32-P4 application
processor (dual-core high-performance RISC-V + a low-power core), paired with
an ESP32-C5 Wi-Fi 6 / Bluetooth 5 (LE) coprocessor over SDIO. The board adds
RMII Ethernet with PoE-capable power input, a selectable MIPI-DSI touch panel,
and a MIPI-CSI camera interface.

<!-- image placeholder: docs/_static/esp32-p4-wifi6-db-poe-eth-board.png (board photo, top view) -->

> **Note**
>
> This page is generated from the board support package (BSP) source in
> [`components/esp32_p4_wifi6_db_poe_eth`](../../../../components/esp32_p4_wifi6_db_poe_eth/).
> It reflects what the BSP configures and drives. Mechanical details not
> exposed by software (connector types, silkscreen labels, indicator LEDs,
> board dimensions, exact PHY/PMIC part numbers) are not yet verified against
> a schematic in this repository and are left as placeholders below.

## Onboard Resources

Confirmed through the BSP's capability flags and pin configuration:

- **Main processor**: ESP32-P4, dual-core RISC-V HP system + low-power core.
- **Flash**: 32 MB (QIO). Note: the example `sdkconfig.defaults` files in this
  repository currently set `CONFIG_ESPTOOLPY_FLASHSIZE_16MB`, which
  under-declares the available flash; update them to 32 MB if you need the
  full capacity.
- **PSRAM**: On-chip PSRAM, 200 MHz (`CONFIG_SPIRAM_SPEED_200M`); exact
  capacity not asserted by the BSP.
- **Wireless coprocessor**: ESP32-C5 (Wi-Fi 6 + Bluetooth 5 LE) over 4-bit
  SDIO (slot 1), driven by the application's `esp_hosted` / `esp_wifi_remote`
  components.
- **Ethernet**: RMII MAC/PHY using the generic IEEE 802.3 PHY driver
  (`esp_eth_phy_new_generic`), PoE-capable power input.
- **Display**: Two-lane MIPI-DSI, Kconfig-selectable panel — HX8394 (5",
  720x1280), ILI9881C (7", 720x1280), or JD9365 (8"/10.1", 800x1280, default).
  No hardware LCD reset pin; backlight is controlled over I2C.
- **Touch**: GT911 capacitive touch controller on the shared I2C bus, polled
  (reset/interrupt not wired).
- **Audio**: ES8311 codec, speaker output + one analog microphone input.
- **Storage**: microSD over 4-bit SDMMC, with power-enable control.
- **Camera**: MIPI-CSI, 2-lane, via `esp_video`; supports OV5647 and SC2336
  sensors. Camera XCLK/RESET are not driven by the BSP (`GPIO_NUM_NC`).
- **USB**: Wired directly to the ESP32-P4's internal USB PHY (DM/DP), with
  over-current protection. Driven as USB Host (`usb/usb_host.h`).
- **Expansion header**: GPIO header enumerated by `bsp_get_header_gpios()`
  (see below). No RTC and no user buttons/LEDs are configured by the BSP
  (`BSP_CAPS_RTC = 0`, `BSP_CAPS_BUTTONS = 0`).
- **ESP-IDF target**: `esp32p4`, requires ESP-IDF >= 5.5.

<!-- image placeholder: docs/_static/esp32-p4-wifi6-db-poe-eth-connectors.png (connector/port labels) -->

## Peripheral Quick Reference

| Module | Device / Function | Interface | Address / Parameters | GPIO / Signal |
| --- | --- | --- | --- | --- |
| LCD | MIPI-DSI panel | MIPI-DSI | 2-lane; HX8394/ILI9881C/JD9365, Kconfig-selectable | No LCD reset pin (`GPIO_NUM_NC`); backlight over shared I2C |
| Touch | GT911 capacitive touch | I2C | 7-bit address `0x5D` or `0x14` (both probed) | SDA=GPIO7, SCL=GPIO8; RST/INT not wired |
| LCD backlight | Backlight controller | I2C | 7-bit address `0x45`; brightness register `0x96` | SDA=GPIO7, SCL=GPIO8 |
| Camera | MIPI-CSI camera connector | MIPI-CSI + SCCB | 2-lane; sensor model/address per module (OV5647, SC2336 supported) | SCCB shares GPIO7/GPIO8; XCLK/RESET not driven by BSP |
| Audio | ES8311 codec | I2C + I2S | I2C 7-bit `0x18` (`ES8311_CODEC_DEFAULT_ADDR`), 8-bit write `0x30`; 1x analog mic | I2C: SDA=GPIO7, SCL=GPIO8; MCLK=GPIO13, SCLK=GPIO12, WS=GPIO10, DOUT=GPIO9, DIN=GPIO11 |
| Power amp | Onboard speaker amp enable | GPIO | Active high | GPIO53 |
| microSD | SDMMC 4-bit | SDMMC | 4-bit mode, slot 0 | CLK=GPIO43, CMD=GPIO44, D0=GPIO39, D1=GPIO40, D2=GPIO41, D3=GPIO42; power enable=GPIO45 (active low) |
| Ethernet | RMII PHY (generic IEEE 802.3 driver) | RMII + SMI | PHY address 1; 50 MHz REF_CLK, external input | CRS_DV=GPIO28, RXD0=GPIO29, RXD1=GPIO30, MDC=GPIO31, TXD0=GPIO34, TXD1=GPIO35, TX_EN=GPIO49, REF_CLK=GPIO50, PHY RESET=GPIO51, MDIO=GPIO52 |
| Wi-Fi/BT coprocessor | ESP32-C5, ESP-Hosted link | SDIO | 4-bit, slot 1, 40 MHz | CLK=GPIO18, CMD=GPIO19, D0=GPIO14, D1=GPIO15, D2=GPIO16, D3=GPIO17, RESET=GPIO54 (active low) |
| USB | USB Host | USB 2.0 | Internal USB PHY, with over-current protection | D+/D- wired directly to ESP32-P4 DM/DP |

## Pin Definition

### Expansion Header

`bsp_get_header_gpios()` returns the BSP-owned, read-only GPIO array below.
Two of these pins are shared with onboard peripherals — see Usage Notes.

| Type | Signal |
| --- | --- |
| GPIO | GPIO2, GPIO3, GPIO4, GPIO5, GPIO20, GPIO21, GPIO22, GPIO23, GPIO24, GPIO25, GPIO26, GPIO27, GPIO32, GPIO33, GPIO36, GPIO45\*, GPIO46, GPIO47, GPIO48, GPIO53\* |

\* GPIO45 is also the microSD power-enable pin; GPIO53 is also the audio
power-amplifier enable pin. Do not drive them independently while the
corresponding peripheral is active.

GPIO6 is also routed to the expansion header but is not part of the
`bsp_get_header_gpios()` software enumeration; it also wakes the ESP32-C5.
Confirm it will not disturb the ESP32-C5 wake timing before using it as a
plain GPIO.

<!-- image placeholder: docs/_static/esp32-p4-wifi6-db-poe-eth-pinout.png (full pinout diagram) -->

## Full GPIO Allocation

The table below lists every GPIO (0–54) and what the BSP source configures it
for. "Not used by BSP" means the pin is not referenced anywhere in
`components/esp32_p4_wifi6_db_poe_eth`; its physical routing (if any) is not
verified in this repository — refer to the hardware schematic once available.

| GPIO | Signal | Connected To | Notes |
| --- | --- | --- | --- |
| GPIO0 | XTAL_32K_N | 32.768 kHz crystal | Not configured by the BSP; disable the onboard XTAL_32K circuit (crystal/matching resistors) before repurposing as a plain GPIO |
| GPIO1 | XTAL_32K_P | 32.768 kHz crystal | Not configured by the BSP; disable the onboard XTAL_32K circuit (crystal/matching resistors) before repurposing as a plain GPIO |
| GPIO2 | GPIO2 | Expansion header | `bsp_get_header_gpios()` |
| GPIO3 | GPIO3 | Expansion header | `bsp_get_header_gpios()` |
| GPIO4 | GPIO4 | Expansion header | `bsp_get_header_gpios()` |
| GPIO5 | GPIO5 | Expansion header | `bsp_get_header_gpios()` |
| GPIO6 | GPIO6 | ESP32-C5 wake / expansion header | Wakes the ESP32-C5; not part of the `bsp_get_header_gpios()` software enumeration |
| GPIO7 | I2C SDA | ES8311, GT911, LCD backlight controller, camera SCCB | Shared I2C data line; not in the header GPIO array |
| GPIO8 | I2C SCL | ES8311, GT911, LCD backlight controller, camera SCCB | Shared I2C clock line; not in the header GPIO array |
| GPIO9 | I2S DOUT | ES8311 DSDIN | Audio playback data, ESP32-P4 -> codec |
| GPIO10 | I2S WS | ES8311 LRCK | Audio frame sync |
| GPIO11 | I2S DIN | ES8311 ASDOUT | Audio capture data, codec -> ESP32-P4 |
| GPIO12 | I2S SCLK | ES8311 SCLK | Audio bit clock |
| GPIO13 | I2S MCLK | ES8311 MCLK | Audio master clock |
| GPIO14 | SDIO D0 | ESP32-C5 | Hosted Wi-Fi/BT link |
| GPIO15 | SDIO D1 | ESP32-C5 | Hosted Wi-Fi/BT link |
| GPIO16 | SDIO D2 | ESP32-C5 | Hosted Wi-Fi/BT link |
| GPIO17 | SDIO D3 | ESP32-C5 | Hosted Wi-Fi/BT link |
| GPIO18 | SDIO CLK | ESP32-C5 | Hosted Wi-Fi/BT link |
| GPIO19 | SDIO CMD | ESP32-C5 | Hosted Wi-Fi/BT link |
| GPIO20 | GPIO20 | Expansion header | `bsp_get_header_gpios()` |
| GPIO21 | GPIO21 | Expansion header | `bsp_get_header_gpios()` |
| GPIO22 | GPIO22 | Expansion header | `bsp_get_header_gpios()` |
| GPIO23 | GPIO23 | Expansion header | `bsp_get_header_gpios()` |
| GPIO24 | GPIO24 | Expansion header | `bsp_get_header_gpios()` |
| GPIO25 | GPIO25 | Expansion header | `bsp_get_header_gpios()` |
| GPIO26 | GPIO26 | Expansion header | `bsp_get_header_gpios()` |
| GPIO27 | GPIO27 | Expansion header | `bsp_get_header_gpios()` |
| GPIO28 | RMII CRS_DV | Ethernet PHY | Carrier sense/RX data valid, PHY -> ESP32-P4 (input) |
| GPIO29 | RMII RXD0 | Ethernet PHY | Receive data bit 0, PHY -> ESP32-P4 (input) |
| GPIO30 | RMII RXD1 | Ethernet PHY | Receive data bit 1, PHY -> ESP32-P4 (input) |
| GPIO31 | SMI MDC | Ethernet PHY | PHY management clock, ESP32-P4 -> PHY (output) |
| GPIO32 | GPIO32 | Expansion header | `bsp_get_header_gpios()` |
| GPIO33 | GPIO33 | Expansion header | `bsp_get_header_gpios()` |
| GPIO34 | RMII TXD0 | Ethernet PHY | Transmit data bit 0, ESP32-P4 -> PHY (output) |
| GPIO35 | RMII TXD1 | Ethernet PHY | Transmit data bit 1, ESP32-P4 -> PHY (output); ESP32-P4 strapping pin, see Usage Notes |
| GPIO36 | GPIO36 | Expansion header | `bsp_get_header_gpios()`; ESP32-P4 strapping pin, see Usage Notes |
| GPIO37 | UART0_TXD | Download/debug UART | Used for flashing and the serial monitor; avoid using as a plain GPIO |
| GPIO38 | UART0_RXD | Download/debug UART | Used for flashing and the serial monitor; avoid using as a plain GPIO |
| GPIO39 | SD D0 | microSD card | SDMMC 4-bit data line |
| GPIO40 | SD D1 | microSD card | SDMMC 4-bit data line |
| GPIO41 | SD D2 | microSD card | SDMMC 4-bit data line |
| GPIO42 | SD D3 | microSD card | SDMMC 4-bit data line |
| GPIO43 | SD CLK | microSD card | SDMMC clock |
| GPIO44 | SD CMD | microSD card | SDMMC command |
| GPIO45 | SD_VDD_EN | microSD power control / expansion header | Active low; avoid driving as a plain GPIO while the SD card is in use |
| GPIO46 | GPIO46 | Expansion header | `bsp_get_header_gpios()` |
| GPIO47 | GPIO47 | Expansion header | `bsp_get_header_gpios()` |
| GPIO48 | GPIO48 | Expansion header | `bsp_get_header_gpios()` |
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
- GPIO37/GPIO38 are the UART0 download port (TXD/RXD), used for flashing
  firmware and the serial monitor; avoid using them as plain GPIO.
- GPIO28–GPIO31, GPIO34–GPIO35, and GPIO49–GPIO52 are used for Ethernet
  RMII/SMI signaling and PHY reset; avoid repurposing them as plain GPIO.
- GPIO34–GPIO37 are ESP32-P4 strapping pins at the SoC level. GPIO34/GPIO35
  are already committed to Ethernet TXD0/TXD1 on this board. Check the
  ESP32-P4 datasheet before changing their state during power-up or reset.
- GPIO0/GPIO1 connect to XTAL_32K_N/XTAL_32K_P of the onboard 32.768 kHz
  crystal, used for the ESP32-P4 RTC slow clock. The BSP does not currently
  configure this. Repurposing either pin as a plain GPIO requires disabling
  the crystal circuit first to avoid conflicting with the crystal signal.
- LCD reset, GT911 reset, and GT911 interrupt are not wired; the display and
  touch drivers handle this in software (polling for touch).
- The header GPIO array (`bsp_get_header_gpios()`) is a software enumeration
  used by examples such as `12_generic_gpio`; it is not necessarily an
  exhaustive list of every pin physically present on the header connector.

## Board Dimensions

<!-- image placeholder: docs/_static/esp32-p4-wifi6-db-poe-eth-dimensions.png (mechanical drawing) -->

Pending hardware documentation.
