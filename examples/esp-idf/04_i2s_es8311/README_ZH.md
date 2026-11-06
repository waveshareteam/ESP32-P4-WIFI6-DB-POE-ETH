# I2S ES8311

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 ES8311 编解码器播放内置音乐片段；切换到回声模式后，把麦克风声音回送到扬声器。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- 扬声器

## 怎么运行

```bash
idf.py set-target esp32p4 build flash monitor
```

如需回声模式，在 menuconfig 的 `Example Configuration` 中修改 `Example mode`。

## 预期现象

音乐模式下扬声器播放 `canon.pcm`；回声模式下扬声器播放麦克风拾取的声音。

## 常见问题

使用回声模式时，让麦克风和扬声器保持距离，以减少啸叫。
