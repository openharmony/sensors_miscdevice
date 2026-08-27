# IPC 设计 — Proxy/Stub

## IPC 接口定义

IPC 契约定义在 `IMiscdeviceService.idl`，服务端为 `MiscdeviceServiceStub`，客户端为 `IMiscdeviceService Proxy`。

详见 codewiki core.md §4.4(IPC 接口定义)。

### 核心接口

| 接口 | IPC Code | 参数 | 说明 |
|------|----------|------|------|
| `Vibrate` | COMMAND_VIBRATE | identifier, timeOut, usage, systemUsage | 定时振动 |
| `PlayVibratorEffect` | COMMAND_PLAY_EFFECT | identifier, effect, loopCount, usage, systemUsage | 预设效果振动 |
| `PlayVibratorCustom` | COMMAND_PLAY_CUSTOM | identifier, package, customInfo | 自定义振动 |
| `PlayPattern` | COMMAND_PLAY_PATTERN | identifier, pattern, customInfo | 模式振动 |
| `StopVibrator` | COMMAND_STOP_VIBRATOR | identifier | 停止振动 |
| `GetVibratorList` | COMMAND_GET_VIBRATOR_LIST | identifier, vibratorInfo[] | 获取振动器列表 |
| `TransferClientRemoteObject` | COMMAND_TRANSFER_CLIENT | remoteObject | 注册客户端回调 |
| `SubscribeVibratorPlugInfo` | COMMAND_SUBSCRIBE_PLUG | remoteObject | 订阅插拔事件 |

### 客户端 Stub

`MiscdeviceClientStub` 接收服务端推送的：
- 振动器插拔事件

服务端通过 `OnRemoteRequest` 分发。

## SA 配置

- SA ID: 3602
- 进程: sensors
- 库: libmiscdevice_service.z.so
- 启动: run-on-create=true
- HDI 代理版本: libvibrator_proxy_2.0.z.so

配置文件: `sa_profile/3602.json`

## 客户端代理

- `VibratorServiceClient`：振动器服务客户端单例，封装 IPC 调用
- `LightClient`：灯光服务客户端单例，封装 IPC 调用

两者通过 `SystemAbilityManager::GetSystemAbility(3602)` 获取服务代理，注册 `DeathRecipient` 监听服务死亡。
