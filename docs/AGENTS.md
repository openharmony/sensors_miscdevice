# sensors_miscdevice 指引

## 项目定位

本仓库对应 OpenHarmony `泛sensor服务/sensors_miscdevice`，管理振动器（Vibrator）和呼吸灯（Light）设备。优先按这些目录定位问题：

- `frameworks/native/vibrator/`：振动器客户端 + IPC Proxy，入口 vibrator_service_client.cpp
- `frameworks/native/light/`：呼吸灯客户端 + C API，入口 light_agent.cpp / light_client.cpp
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

详见 docs/build-test.md。涉及真实振动器/呼吸灯硬件需补充板侧证据，提交使用 `git commit -s`。

## 知识索引

改动前按场景读取对应文档：

| 场景 | 先读 |
|------|------|
| 目录分层、模块职责 | docs/code-map.md |
| 振动器 JS API（startVibration/pattern/查询） | docs/api/vibrator-js.md |
| 振动器 Native/C API | docs/api/vibrator-native.md |
| 呼吸灯 API（GetLightList/TurnOn/TurnOff） | docs/api/light.md |
| IPC 接口与 Proxy/Stub | docs/architecture/ipc-design.md |
| 振动控制数据流（应用→IPC→Service→HDI→马达） | docs/architecture/data-flow.md |
| HDI 连接/重连/容错 | docs/architecture/hdi-connection.md |
| 权限模型与错误码 | docs/security/permission.md |
| 振动效果解码（HE/OH 格式） | docs/features/haptic-decoder.md |
| 约束/反模式/依赖禁忌 | docs/constraints.md |
| 编译/测试/调试命令 | docs/build-test.md |
| 场景→文档完整路由表 + 术语表 | docs/knowledge-routing.md |

## 关键约束

- 振动需 `ohos.permission.VIBRATE`，呼吸灯需 `ohos.permission.SYSTEM_LIGHT_CONTROL`，校验在 Service 端（详见 docs/constraints.md）
- 振动前必须 `SetUsageEnhanced` 设置使用场景；解码后必须 `FreeVibratorPackage` 释放内存
- 使用 `strcpy_s` 而非 `strcpy`；NAPI 回调不可在数据线程直接调用 JS
- `VibrationPriorityManager` 决策振动是否执行，不可绕过
- `frameworks/` 不可直接依赖 `services/` 内部实现，只通过 IPC 通信
