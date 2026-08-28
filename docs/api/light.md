# 灯光 API

> 灯光控制接口，位于 `frameworks/native/light/`。

## C API

头文件：`interfaces/inner_api/light/light_agent.h`、`light_agent_type.h`

| 函数 | 说明 |
|------|------|
| `GetLightList(LightInfo **info, int32_t &count)` | 获取灯光设备列表 |
| `TurnOn(lightId, color, animation)` | 点亮指定灯光 |
| `TurnOff(lightId)` | 关闭指定灯光 |

详见 codewiki core.md §4.2(C API)、modules.md §2(Native Light Client)。

## 数据结构

```cpp
struct LightInfo {
    int32_t lightId;           // 灯光设备 ID
    int32_t lightType;         // 灯光类型
    int32_t lightNumber;       // 灯光数量
    char lightName[128];       // 灯光名称
};

struct LightColor {
    uint32_t singleColor;      // ARGB 格式颜色值
};

struct LightAnimation {
    int32_t mode;              // 动画模式
    int32_t onTime;            // 亮起持续时间(ms)
    int32_t offTime;           // 熄灭持续时间(ms)
};
```

详见 codewiki core.md §5.3(灯光数据结构)、modules.md §2 §6.1(核心数据结构)。

## 客户端架构

`LightClient` 单例（`frameworks/native/light/`）：
- 懒加载：第一次调用 `GetLightList` 时从服务端获取并缓存
- IPC：通过 `IMiscdeviceService` Proxy 调用 SA 3602
- 死亡监听：注册 `DeathRecipient`，服务死亡时自动重连
- 参数校验：`IsLightIdValid` 校验 lightId，`IsLightAnimationValid` 校验动画参数
- 内存管理：`malloc` 分配灯光列表，`ClearLightInfos` 释放

详见 codewiki modules.md §2 §7(设备管理)、§8(HDI交互)。
