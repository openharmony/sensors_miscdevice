# 知识路由

> 遇到什么问题读什么文档。本文件是路由索引，指向 docs/ 子文档和 codewiki 段落。

## 场景路由表

| 场景/问题 | 先读 | 关键概念 |
|-----------|------|----------|
| 目录分层、模块职责、入口文件 | docs/code-map.md | frameworks/services/utils/haptic_decoder |
| 编译命令、产物路径、测试 target、Feature flags | docs/build-test.md | hb build、miscdevice_service_target、SA 3602 |
| 约束、反模式、依赖禁忌 | docs/constraints.md | VIBRATE权限、FreeVibratorPackage、strcpy_s |
| 振动器 JS API（vibrate/startVibration/stop/查询） | docs/api/vibrator-js.md | startVibration、VibrateOptions、VibratorPatternBuilder |
| 振动器 Native/NDK 接口 | docs/api/vibrator-native.md | vibrator_agent.h、OH_Vibrator_PlayVibration、VibratorServiceClient |
| 灯光 API | docs/api/light.md | LightClient、GetLightList、TurnOn/TurnOff |
| IPC 接口与 Proxy/Stub | docs/architecture/ipc-design.md | IMiscdeviceService.idl、Proxy-Stub、COMMAND_VIBRATE |
| 振动控制数据流（应用→IPC→Service→HDI→硬件） | docs/architecture/data-flow.md | VibratorThread、VibrationPriorityManager、CustomVibrationMatcher |
| HDI 连接/重连/容错 | docs/architecture/hdi-connection.md | IVibratorInterface V2.0、ILightInterface、DeathRecipient |
| 振动优先级管理 | docs/architecture/data-flow.md §优先级 | VibrationPriorityManager、勿扰模式、IGNORE_* |
| 权限模型与错误码 | docs/security/permission.md | VIBRATE、SYSTEM_LIGHT_CONTROL、MANAGE_VIBRATOR |
| 振动效果解码（HE/OH 格式） | docs/features/haptic-decoder.md | HapticDecoder、HE/OH JSON、波形调制、SeekTimeOnPackage |
| 设备热插拔监听 | docs/api/vibrator-native.md §插拔 | SubscribeVibratorPlug、VibratorPlugState |
| 会话控制与进程级控制 | docs/api/vibrator-native.md §会话 | PlayPatternBySessionId、DisableVibratorByPid |
| 故障排查/日志/调试 | docs/build-test.md §调试 | hidumper -s 3602、hilog、ohos-vibratorControl |

## 术语表

| 术语 | 含义 |
|------|------|
| SA 3602 | MiscdeviceService 系统能力 ID，运行在 sensors 进程 |
| HDI | Hardware Device Interface，硬件驱动接口 |
| IVibratorInterface V2.0 | 振动器 HDI 接口主版本 |
| ILightInterface V1.0 | 灯光 HDI 接口版本 |
| VibratorIdentifier | 设备标识符 `{deviceId, vibratorId}`，-1 表示默认 |
| VibrateInfo | 振动参数信息结构（type/usage/duration/effectId/count/intensity） |
| VibratePackage | 振动包结构，含 patterns 列表 |
| VibrateEvent | 振动事件，tag=CONTINUOUS/TRANSIENT |
| VibrateCurvePoint | 振动曲线控制点（time/intensity/frequency） |
| VibratorCapacity | 设备能力（高清触感/预设映射/时延控制） |
| VibrationPriorityManager | 优先级管理器，通过 data_share 观察 settings 数据库决策振动是否执行 |
| VibratorThread | 振动执行线程 |
| CustomVibrationMatcher | 抹平算法，底层不支持某特性振动时抹平为时长振动 |
| SessionId | 服务侧分配的会话标识，传到底层判断振动生命周期 |
| miscdevice_common_event_subscriber | 监听公共事件，确认 data_share 是否可用 |
| Haptic Decoder | 振动效果解码器，支持 HE/OH 格式 |
| HE 格式 | 振动效果 JSON 格式之一（HEVibratorDecoder） |
| OH 格式 | 振动效果 JSON 格式之一（DefaultVibratorDecoder） |
| Usage | 使用场景枚举（alarm/ring/notification/touch/media 等） |
| systemUsage | 系统调用标志，绕过部分权限检查 |
| CompatibleConnection | HDI 兼容模拟实现 |
| QOS | Quality of Service，预设振动使用 QOS_USER_INTERACTIVE 优化响应 |
| VibratorPatternBuilder | 振动模式构建器，链式调用构建复杂振动序列 |
| ohos-vibratorControl | CLI 工具，直接调用 innerkit 接口操作振动，预置到系统镜像 |
