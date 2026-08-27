# sensors_miscdevice 指引

## 项目定位

本仓库对应 OpenHarmony `泛sensor服务/sensors_miscdevice`，管理振动器（Vibrator）和灯光（Light）设备。优先按这些目录定位问题：

- `frameworks/native/vibrator/`：振动器客户端 + IPC Proxy，入口 vibrator_service_client.cpp
- `frameworks/native/light/`：灯光客户端 + NDK 接口，入口 light_agent.cpp / light_client.cpp
- `frameworks/js/napi/`：JS/ArkTS NAPI 绑定层，入口 vibrator_js.cpp
- `services/miscdevice_service/`：Miscdevice Service（SA 3602），入口 miscdevice_service.cpp
- `services/miscdevice_service/hdi_connection/`：HDI 驱动连接层
- `utils/common/`：权限/JSON/IPC数据结构/日志
- `utils/haptic_decoder/`：HE/OH 格式振动效果解码器

## 构建和验证

构建命令从 OpenHarmony 源码根目录执行：

```sh
./build.sh --product-name rk3568 --build-target miscdevice --ccache
hdc shell "hidumper -s 3602"    # 验证 SA 存活
```

详见 docs/build-test.md。涉及真实振动器/灯光硬件需补充板侧证据，提交使用 `git commit -s`。

## 知识索引

改动前按场景读取对应文档：

| 场景 | 先读 |
|------|------|
| 目录分层、模块职责 | docs/code-map.md |
| 振动器 JS API（startVibration/pattern/查询） | docs/api/vibrator-js.md |
| 振动器 Native/NDK 接口 | docs/api/vibrator-native.md |
| 灯光 API（GetLightList/TurnOn/TurnOff） | docs/api/light.md |
| IPC 接口与 Proxy/Stub | docs/architecture/ipc-design.md |
| 振动控制数据流（应用→IPC→Service→HDI→马达） | docs/architecture/data-flow.md |
| HDI 连接/重连/容错 | docs/architecture/hdi-connection.md |
| 权限模型与错误码 | docs/security/permission.md |
| 振动效果解码（HE/OH 格式） | docs/features/haptic-decoder.md |
| 约束/反模式/依赖禁忌 | docs/constraints.md |
| 编译/测试/调试命令 | docs/build-test.md |
| 场景→文档完整路由表 + 术语表 | docs/knowledge-routing.md |

## 关键约束

- 振动需 `ohos.permission.VIBRATE`，灯光需 `ohos.permission.SYSTEM_LIGHT_CONTROL`，校验在 Service 端（详见 docs/constraints.md）
- 振动前必须 `SetUsageEnhanced` 设置使用场景；解码后必须 `FreeVibratorPackage` 释放内存
- 使用 `strcpy_s` 而非 `strcpy`；NAPI 回调不可在数据线程直接调用 JS
- `VibrationPriorityManager` 决策振动是否执行，不可绕过
- `frameworks/` 不可直接依赖 `services/` 内部实现，只通过 IPC 通信

## 参考资料

> 仅当涉及**接口签名、参数定义、枚举值、错误码含义、HDI IDL 定义**等接口相关问题时，才读取以下参考资料。做代码修改、架构分析、编译调试时不要读这些文档，避免跑偏。

### ArkTS API 参考

- [@ohos.vibrator（振动）](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/js-apis-vibrator.md) - 振动控制 API（startVibration/stop/查询）
- [@system.vibrator（振动）](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/js-apis-system-vibrate.md) - 旧版振动 API（兼容）
- [振动错误码](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/errorcode-vibrator.md) - 错误码参考

### C API 参考

- [vibrator.h](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-vibrator-h.md) - 振动控制 C API
- [vibrator_type.h](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-vibrator-type-h.md) - 振动类型定义 C API
- [Vibrator_Attribute](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-vibrator-vibrator-attribute.md) - 振动属性结构体
- [Vibrator_FileDescription](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-sensor-service-kit/capi-vibrator-vibrator-filedescription.md) - 振动文件描述符结构体

### HDI 接口参考

- [IVibratorInterface V1.2](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/interface_i_vibrator_interface_v12.md) - 振动器 HDI 接口 V1.2
- [IVibratorInterface V1.1](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/interface_i_vibrator_interface_v11.md) - 振动器 HDI 接口 V1.1
- [IVibratorInterface V1.0](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/interface_i_vibrator_interface_v10.md) - 振动器 HDI 接口 V1.0
- [VibratorTypes V1.2](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/_vibrator_types_8idl_v12.md) - HDI 振动器类型定义 V1.2
- [HapticEvent](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/_haptic_event_v12.md) - 触觉事件结构
- [HapticPackage](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/_haptic_paket_v12.md) - 触觉振动包结构
- [HapticCapacity](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/_haptic_capacity_v12.md) - 触觉能力描述
- [CurvePoint](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/_curve_point_v12.md) - 振动曲线控制点
- [CompositeEffect](https://gitcode.com/openharmony/docs/blob/master/zh-cn/device-dev/reference/hdi-apis/vibrator/union_composite_effect_v11.md) - 复合振动效果

### 开发指南

- [Sensor Service Kit 简介](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/device/sensor/sensorservice-kit-intro.md) - 传感器服务 Kit 介绍（含振动器）
- [振动开发指导](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/device/sensor/vibrator-guidelines.md) - ArkTS 振动开发指南
- [振动开发指导（C/C++）](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/device/sensor/vibrator-guidelines-capi.md) - C API 振动开发指南
- [振动开发概述](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/device/sensor/vibrator-overview.md) - 振动开发概述
