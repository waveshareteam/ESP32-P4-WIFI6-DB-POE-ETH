# NVS 读写

[English](README.md) | [简体中文](README_ZH.md)

## 功能

把开机计数器和一个字符串保存到 NVS，计数器在重启后保留。

## 硬件需求

- ESP32-P4-WIFI6-DB-POE-ETH 开发板
- USB 数据线

## 怎么运行

```bash
idf.py set-target esp32p4 build flash monitor
```

## 预期现象

每次启动时 `Boot count:` 的值递增，并打印 `Read string: Hello from NVS!`。

## 常见问题

运行 `idf.py erase-flash` 可以清零计数器。
