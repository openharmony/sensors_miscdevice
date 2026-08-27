# 振动器 JS API

> 振动器 JS/ArkTS 接口，位于 `frameworks/js/napi/`。

## 振动接口

### startVibration — 高级振动

```typescript
vibrator.startVibration(options: VibrateOptions, usage: UsageOptions, callback?): Promise<void>
```

四种振动类型：

| type | 必填参数 | 说明 |
|------|----------|------|
| `'time'` | `duration` | 定时振动 |
| `'preset'` | `effectId` | 预设效果，可选 count/intensity |
| `'file'` | `hapticFd: {fd, offset?, length?}` | 从 Haptic 文件读取 |
| `'pattern'` | `pattern: VibratorPattern` | 自定义序列 |

详见 codewiki modules.md §1 §2.1(高级振动控制)、§5.2.2(startVibration 接口)。

### vibrate — 基础振动（兼容）

```typescript
vibrator.vibrate(duration: number, callback?)
vibrator.vibrate(effectId: string, callback?)
vibrator.vibrate(options: {mode: 'long'|'short'}, callback?)
```

- `mode: 'long'` = 1000ms，`mode: 'short'` = 35ms

详见 codewiki modules.md §1 §5.2.1(vibrate 接口)。

### stop / stopVibration / cancel

```typescript
vibrator.stop(mode?, callback?)
vibrator.stopVibration(mode?, callback?)
vibrator.stopVibrationSync()
vibrator.cancel(identifier?, callback?)
```

详见 codewiki core.md §4.1(JavaScript NAPI 接口)。

## 查询接口

| 函数 | 说明 |
|------|------|
| `getVibratorInfoSync(identifier?)` | 获取振动器列表 |
| `getEffectInfoSync(effectType, identifier?)` | 获取效果信息 |
| `isSupportEffect(effectId, callback?)` | 查询效果支持（异步） |
| `isSupportEffectSync(effectId)` | 查询效果支持（同步） |
| `isHdHapticSupported()` | 查询高清振动支持 |

详见 codewiki core.md §4.1、modules.md §1 §2.1.4(设备查询功能)。

## 设备热插拔监听

```typescript
vibrator.on('vibratorStateChange', (event: VibratorPlugEvent) => {})
vibrator.off('vibratorStateChange', callback?)
```

`VibratorPlugEvent`: `isVibratorOnline`、`deviceId`、`timestamp`、`vibratorCount`

详见 codewiki modules.md §1 §2.1.5(设备状态监听)、§7.3(设备热插拔监听)。

## 振动模式构建器

```typescript
const builder = new vibrator.VibratorPatternBuilder()
builder.addContinuousEvent(0, 500, {intensity: 80, frequency: 50})
builder.addTransientEvent(100, {intensity: 100, index: 1})
const pattern = builder.build()
```

支持连续事件（CONTINUOUS）和瞬态事件（TRANSIENT）。

详见 codewiki modules.md §1 §2.1.3(振动器模式构建器)、§4.4(模式构建器使用流程)。

## Usage 枚举

| usage 值 | 枚举 | 说明 |
|----------|------|------|
| `'unknown'` | USAGE_UNKNOWN | 未知 |
| `'alarm'` | USAGE_ALARM | 闹钟 |
| `'ring'` | USAGE_RING | 铃声 |
| `'notification'` | USAGE_NOTIFICATION | 通知 |
| `'communication'` | USAGE_COMMUNICATION | 通讯 |
| `'touch'` | USAGE_TOUCH | 触摸 |
| `'media'` | USAGE_MEDIA | 媒体 |
| `'physicalFeedback'` | USAGE_PHYSICAL_FEEDBACK | 物理反馈 |
| `'simulateReality'` | USAGE_SIMULATE_REALITY | 模拟现实 |

详见 codewiki modules.md §1 §9.1(权限模型)。

## 参数校验常量

| 常量 | 值 | 说明 |
|------|-----|------|
| VIBRATE_SHORT_DURATION | 35 | 短振动时长(ms) |
| VIBRATE_LONG_DURATION | 1000 | 长振动时长(ms) |
| INTENSITY_MIN/MAX | 0/100 | 强度范围 |
| FREQUENCY_MIN/MAX | 0/100 | 频率范围 |
| CONTINUOUS_DURATION_MAX | 5000 | 连续振动最大时长(ms) |
| EVENT_START_TIME_MAX | 1800000 | 事件开始时间最大值(ms) |
| EVENT_NUM_MAX | 128 | 最大事件数 |
| CURVE_POINT_NUM_MIN/MAX | 4/16 | 曲线点数范围 |

详见 codewiki modules.md §1 §6.3(参数校验常量)。
