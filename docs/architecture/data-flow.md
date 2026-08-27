# 振动控制数据流与架构

## 整体架构

四层结构：应用层（JS/ArkTS/C/C++）→ 接口层（NAPI/C API/Inner API）→ 框架层（VibratorServiceClient/LightClient）→ 服务层（MiscdeviceService SA 3602 → HDI → 驱动）

详见 codewiki core.md §3.1(整体架构图)、§3.5(技术分层)。

## 振动控制数据流

```
JS 应用 startVibration()
  → NAPI 接口层 (vibrator_js.cpp)
  → VibratorServiceClient (单例封装)
  → IMiscdeviceService Proxy (IPC Binder)
  → MiscdeviceServiceStub (OnRemoteRequest)
  → MiscdeviceService (核心服务)
    → CheckAuthAndParam() 权限校验
    → VibrationPriorityManager.ShouldIgnoreVibrate() 优先级判断
    → 若不忽略：
      → VibratorThread.UpdateVibratorEffect()
      → CustomVibrationMatcher 效果转换（抹平算法：底层不支持某特性时抹平为时长振动）
      → VibratorHdiConnection
      → IVibratorInterface (HDI)
      → Vibrator HAL → Kernel Driver → 振动马达
```

详见 codewiki core.md §3.2(振动控制数据流)、§3.4(组件交互时序)。

### 关键数据结构

| 结构 | 说明 |
|------|------|
| VibratorIdentifier | `{deviceId, vibratorId}`，-1 表示默认 |
| VibrateInfo | 振动参数：mode/packageName/pid/uid/usage/duration/effect/count/intensity/package/sessionId |
| VibratePackage | 振动包：packageDuration + patterns 列表 |
| VibratePattern | 模式：startTime + patternDuration + events 列表 |
| VibrateEvent | 事件：tag(CONTINUOUS/TRANSIENT)/time/duration/intensity/frequency/index/points |
| VibrateCurvePoint | 曲线点：time/intensity/frequency |

数据结构定义详见 core.md §5.2(振动器核心数据结构)。

## 优先级管理

`VibrationPriorityManager` 基于以下条件决策振动是否执行：

- 勿扰模式状态（`miscdevice_feature_do_not_disturb_enable`）
- 铃声设置
- 使用场景（Usage）
- 系统调用标志（systemUsage）

通过 `data_share` 观察 settings 数据库变化获取上述状态。`miscdevice_common_event_subscriber` 监听公共事件，主要用来确认 data_share 是否可用，从而获取数据库数据。

返回 `VIBRATION`（执行）或 `IGNORE_*`（忽略）。

详见 codewiki core.md §3.1(优先级管理器)、§3.4(组件交互时序)。

## 振动并发与打断

- 多个应用同时请求振动时，**后者打断前者**
- 相同级别 usage 可以互相打断
- 不同级别 usage 需根据 usage 优先级判断是否能被打断
- SessionId 由服务侧分配，传到底层后由底层判断振动生命周期

## 模块依赖关系

```
Level 1: utils/common ← utils/haptic_decoder (基础)
Level 2: frameworks/native/vibrator ← services/miscdevice_service ← hdi_connection (核心)
         frameworks/native/light ← services/miscdevice_service
Level 3: frameworks/js/napi ← frameworks/native/vibrator (接口)
```

详见 codewiki core.md §3.3(模块依赖关系)。

## 灯光控制

灯光控制通过 `LightClient` 单例 → IPC → `MiscdeviceService` → `LightHdiConnection` → `ILightInterface` → LED 驱动。

详见 codewiki modules.md §2(Native Light Client)。
