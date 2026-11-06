# USB 扩展屏

[English](README.md) | [简体中文](README_ZH.md)

## 功能

让开发板在 Windows 电脑上作为 USB 扩展显示器使用，并支持触摸和音频。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 支持的 MIPI-DSI 屏幕（5、7、8 或 10.1 英寸）
- Windows 电脑，以及连接高速 USB 口的数据线
- 扬声器

## 怎么运行

在 `Component config > Board Support Package (ESP32-P4-WIFI6-DB-POE-ETH) > Display > Select LCD type` 中选择实际连接的屏幕，默认是 10.1 英寸屏。按 [windows_driver](windows_driver/README_ZH.md) 安装电脑端驱动。首次编译需要联网。

```bash
idf.py set-target esp32p4 menuconfig
idf.py build flash monitor
```

## 预期现象

安装驱动并连接 USB 后，Windows 出现新的显示器，LCD 显示扩展桌面。

## 常见问题

触摸控制了错误的屏幕时，在平板电脑设置中重新选择显示器。USB 音频卡顿时，调低 `CONFIG_USB_EXTEND_SCREEN_MAX_FPS`。更换屏幕后需要重新编译并重新连接 USB。
