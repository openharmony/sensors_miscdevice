# 振动效果解码器

> HE/OH 格式振动效果解码，位于 `utils/haptic_decoder/`。

## 模块结构

```
utils/haptic_decoder/
├── interface/          # 解码器接口定义
├── he_json/            # HE 格式解码器
└── oh_json/            # OH 格式解码器
```

## 解码流程

```
振动媒体文件 (JSON)
  → VibratorDecoderCreator (解码器工厂)
    → HEVibratorDecoder (HE 格式)
    或 DefaultVibratorDecoder (OH 格式)
  → 解析 JSON 振动效果
  → 生成内部波形数据 (VibratePackage)
  → 波形调制 (ModulatePackage)
  → 时间跳转 (SeekTimeOnPackage)
  → 触觉事件输出
```

详见 codewiki core.md §3.1(解码器层)、modules.md §9(Haptic Decoder)。

## 支持的格式

| 格式 | 解码器 | 说明 |
|------|--------|------|
| HE | HEVibratorDecoder | HE 格式振动效果 JSON |
| OH | DefaultVibratorDecoder | OH 格式振动效果 JSON |

## 核心功能

| 功能 | 接口 | 说明 |
|------|------|------|
| 效果预处理 | `PreProcess` | 解码振动媒体文件生成内部波形数据 |
| 波形调制 | `ModulatePackage` | 使用调制曲线调整强度/频率 |
| 时间跳转 | `SeekTimeOnPackage` | 基于时间戳截取波形片段，支持音振同步 |
| 资源释放 | `FreeVibratorPackage` | 释放解码后的内存 |

详见 codewiki core.md §2(功能列表 - 效果预处理/波形调制/时间跳转)。

## 动态库加载

解码器通过 `dlopen`/`dlsym` 动态加载，支持运行时切换解码库。

详见 codewiki modules.md §3 §1.2(技术选型)。

## 依赖

`haptic_decoder` 是独立解码模块，只依赖 `utils/common`（基础工具）和 `cJSON`（JSON 解析），不依赖 `services/`。

## 相关工具

- `utils/tools/audio2haptic/`：音频转触觉工具
- `utils/tools/haptic_format_converter/`：格式转换工具
- `tools/ohos-vibratorControl/`：命令行振动控制工具
