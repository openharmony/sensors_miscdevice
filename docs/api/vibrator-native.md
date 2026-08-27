# 振动器 Native / NDK 接口

> C/C++ 振动器接口，位于 `frameworks/native/vibrator/` 和 `interfaces/`。

## Inner API（内部 API）

头文件：`interfaces/inner_api/vibrator/vibrator_agent.h`、`vibrator_agent_type.h`

### 基础振动

| 函数 | 说明 |
|------|------|
| `StartVibrator(identifier, effect)` | 预设效果振动 |
| `StartVibratorOnce(identifier, duration)` | 定时振动 |
| `StopVibrator(identifier)` | 停止振动 |
| `Cancel(identifier)` | 取消振动 |

### 增强型振动

| 函数 | 说明 |
|------|------|
| `StartVibratorOnceEnhanced(identifier, duration)` | 增强版定时振动 |
| `StartVibratorEnhanced(identifier, effect)` | 增强版预设效果 |
| `PlayVibratorCustomEnhanced(identifier, fd, offset, length)` | 自定义振动 |
| `PlayPatternEnhanced(identifier, pattern)` | 模式振动 |
| `PlayPrimitiveEffectEnhanced(identifier, effect)` | 预设效果增强 |
| `StopVibratorEnhanced(identifier)` | 增强版停止 |
| `CancelEnhanced(identifier)` | 取消振动 |

### 自定义振动包

| 函数 | 说明 |
|------|------|
| `PlayVibratorCustom(identifier, package, customInfo)` | 播放自定义振动 |
| `PreProcess(identifier, package, customInfo)` | 预处理振动包 |
| `SeekTimeOnPackage(identifier, package, time)` | 时间跳转 |
| `ModulatePackage(identifier, package, curve)` | 曲线调制 |
| `FreeVibratorPackage(package)` | 释放振动包内存 |

### 参数配置

| 函数 | 说明 |
|------|------|
| `SetUsageEnhanced(identifier, usage, systemUsage)` | 设置使用场景 |
| `SetLoopCountEnhanced(identifier, count)` | 设置循环次数 |
| `SetParameters(identifier, params)` | 设置效果参数 |

### 查询

| 函数 | 说明 |
|------|------|
| `IsHdHapticSupported(identifier)` | 查询高清振动支持 |
| `IsSupportEffect(identifier, effectId)` | 查询效果支持 |
| `GetVibratorList(identifier, info[])` | 获取振动器列表 |
| `GetEffectInfo(identifier, effectType)` | 获取效果信息 |

### 插拔订阅

| 函数 | 说明 |
|------|------|
| `SubscribeVibratorPlug(callback)` | 订阅振动器插拔事件 |
| `UnSubscribeVibratorPlug(callback)` | 取消订阅 |

### 会话控制

| 函数 | 说明 |
|------|------|
| `PlayPatternBySessionId(identifier, pattern, sessionId)` | 按会话 ID 播放模式 |
| `PlayPackageBySessionId(identifier, package, sessionId)` | 按会话 ID 播放包 |
| `StopVibrateBySessionId(identifier, sessionId)` | 按会话 ID 停止 |

### 进程级控制

| 函数 | 说明 |
|------|------|
| `DisableVibratorByPid(pid)` | 按 PID 禁用振动 |
| `EnableVibratorByPid(pid)` | 按 PID 恢复振动 |

详见 codewiki core.md §4.3(Inner API 接口)、modules.md §3(Native Vibrator Client)。

## NDK 接口（kits/c）

头文件：`interfaces/kits/c/vibrator.h`、`vibrator_type.h`

| 函数 | 说明 |
|------|------|
| `OH_Vibrator_PlayVibration(duration, attribute)` | 定时振动 |
| `OH_Vibrator_PlayVibrationCustom(fileDescription, attribute)` | 自定义振动 |
| `OH_Vibrator_Cancel()` | 停止振动 |

### 数据类型

```c
typedef struct Vibrator_Attribute {
    int32_t vibratorId;        // -1 表示默认
    Vibrator_Usage usage;      // 使用场景枚举
} Vibrator_Attribute;

typedef struct Vibrator_FileDescription {
    int32_t fd;
    int64_t offset;
    int64_t length;
} Vibrator_FileDescription;
```

详见 codewiki core.md §4.2(C API 接口)。

## 客户端架构

`VibratorServiceClient`（单例）封装所有 IPC 调用：
- 通过 `SystemAbilityManager` 获取 SA 3602 代理
- 注册 `DeathRecipient` 监听服务死亡
- 死亡后自动重连

详见 codewiki modules.md §3(Native Vibrator Client)、§3.2(客户端架构)。
