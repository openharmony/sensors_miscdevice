# HDI 连接 — 驱动连接/重连/容错

## HDI 连接架构

```
MiscdeviceService (services/miscdevice_service/)
  → VibratorHdiConnection (振动器 HDI 单例)
    → HdiConnection (真实 HDI 实现类，位于 hdi_connection/interface/)
    → CompatibleConnection (兼容模拟实现类，位于 hdi_connection/adapter/)
  → LightHdiConnection (呼吸灯 HDI 单例)
    → ILightInterface (HDI)
```

> 注意：`hdi_connection` 既是目录名也是架构层名，`HdiConnection`（首字母大写）是其中的真实 HDI 实现类，需与泛指的"HDI 连接"概念区分。

详见 codewiki core.md §3.1(整体架构)、modules.md §7(HDI Connection Layer)。

## 振动器 HDI 接口

IVibratorHdiConnection 接口（`services/miscdevice_service/hdi_connection/interface/`）：

| 方法 | 说明 |
|------|------|
| `ConnectHdi()` | 连接 HDI 服务 |
| `StartOnce(identifier, duration)` | 单次振动 |
| `Start(identifier, effectType)` | 预设效果振动 |
| `Stop(identifier, mode)` | 停止振动 |
| `EnableCompositeEffect(identifier, effect)` | 复合效果 |
| `IsVibratorRunning(identifier)` | 运行状态查询 |
| `GetVibratorInfo(vibratorInfo[])` | 设备信息 |
| `RegisterVibratorPlugCallback(callback)` | 注册插拔回调 |

详见 codewiki core.md §4.5(HDI 接口抽象)。

## 重连容错机制

1. HDI 服务死亡时 `DeathRecipient` / `ProcessDeathObserver` 触发
2. 清空代理引用
3. 重新调用 `InitLightClient()` / 等效方法重建连接
4. 重连后恢复服务代理

详见 codewiki modules.md §2 §7.3(服务死亡处理)、core.md §7.6(生产环境注意事项)。

## 兼容模式

- `CompatibleConnection`（`hdi_connection/adapter/`）提供兼容模拟实现
- ENG 版本可使用兼容模式进行无硬件环境测试

详见 codewiki core.md §3.1(CompatibleConnection)。

## 呼吸灯 HDI

`LightHdiConnection` 通过 `ILightInterface` V1.0 与呼吸灯驱动交互，控制呼吸灯开关、颜色和动画。
