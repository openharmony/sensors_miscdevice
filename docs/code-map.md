# 代码地图

> sensors_miscdevice 仓库目录分层与模块职责。

## 目录结构

```
miscdevice/
├── frameworks/                      # 接口框架层
│   ├── js/napi/                    # JS/ArkTS NAPI 绑定层（振动器）
│   ├── native/
│   │   ├── vibrator/               # 振动器 Native 客户端 + IPC Proxy
│   │   └── light/                  # 灯光 Native 客户端 + NDK 接口
│   ├── capi/                       # C API 封装层
│   ├── cj/                         # Cangjie 语言绑定层
│   └── ets/taihe/                  # Taihe/ETS 绑定层
├── services/
│   └── miscdevice_service/         # Miscdevice Service（SA 3602）
│       ├── include/                # 服务头文件
│       ├── src/                    # 服务实现
│       ├── hdi_connection/         # HDI 驱动连接层
│       │   ├── adapter/            # HDI 兼容适配
│       │   └── interface/          # HDI 连接接口
│       └── haptic_matcher/         # 触觉效果匹配器
├── utils/
│   ├── common/                     # 通用工具：JSON/权限/IPC数据结构/日志
│   ├── haptic_decoder/             # 振动效果解码器（HE/OH 格式）
│   │   ├── he_json/               # HE 格式解码
│   │   ├── oh_json/               # OH 格式解码
│   │   └── interface/             # 解码器接口
│   └── tools/                     # 工具（音频转触觉/格式转换）
├── interfaces/
│   ├── inner_api/
│   │   ├── vibrator/              # 振动器内部 API（vibrator_agent.h）
│   │   └── light/                 # 灯光内部 API（light_agent.h）
│   └── kits/c/                    # NDK 公共 API（vibrator.h）
├── sa_profile/                    # 系统能力配置（SA 3602）
├── tools/
│   └── ohos-vibratorControl/      # 振动器控制命令行工具
├── test/                           # 测试
│   ├── unittest/                  # 单元测试（vibrator/light/common）
│   └── fuzztest/                  # Fuzz 测试（vibrator/light/service）
├── bundle.json                     # 部件配置
└── hisysevent.yaml                 # 事件埋点配置
```

## 模块职责

| 模块 | 路径 | 职责 | 入口文件 |
|------|------|------|----------|
| JS NAPI Binding | `frameworks/js/napi/` | JS/ArkTS 振动器接口绑定，vibrate/startVibration/stop/查询 | `src/vibrator_js.cpp` |
| Native Vibrator Client | `frameworks/native/vibrator/` | 振动器客户端 + IPC Proxy + 插拔订阅 + 自定义振动 | `src/vibrator_service_client.cpp` |
| Native Light Client | `frameworks/native/light/` | 灯光客户端 + NDK 接口 + IPC Proxy | `light_agent.cpp`、`src/light_client.cpp` |
| C API | `frameworks/capi/` | C API 封装层，封装 `interfaces/kits/c/`，新增 C 接口两者都需同步 | - |
| Cangjie 绑定 | `frameworks/cj/` | Cangjie 语言振动器 FFI 绑定 | `src/` |
| Taihe/ETS 绑定 | `frameworks/ets/taihe/` | Taihe 声明式振动器接口 | - |
| Miscdevice Service | `services/miscdevice_service/` | SA 3602 服务端，振动调度/优先级/灯光控制 | `src/miscdevice_service.cpp` |
| VibrationPriorityManager | `services/miscdevice_service/` | 通过 data_share 观察 settings 数据库，决策振动是否执行 | `src/vibration_priority_manager.cpp` |
| VibratorThread | `services/miscdevice_service/` | 振动执行线程 | `src/vibrator_thread.cpp` |
| HDI Connection | `services/miscdevice_service/hdi_connection/` | HDI 驱动连接/重连/容错 | `interface/` |
| Haptic Matcher | `services/miscdevice_service/haptic_matcher/` | 抹平算法，底层不支持某特性振动时抹平为时长振动 | - |
| Common Utils | `utils/common/` | 权限校验/JSON解析/IPC数据结构/日志 | `include/` |
| Haptic Decoder | `utils/haptic_decoder/` | HE/OH 格式振动效果解码，波形调制 | `interface/` |
| VibratorControl 工具 | `tools/ohos-vibratorControl/` | CLI 工具，直接调用 innerkit 接口操作振动，预置系统镜像 | `src/` |

## 模块依赖关系

```
Level 1 (基础):  utils/common ← utils/haptic_decoder
Level 2 (核心):  frameworks/native/vibrator ← services/miscdevice_service ← services/miscdevice_service/hdi_connection
                 frameworks/native/light ← services/miscdevice_service
Level 3 (接口):  frameworks/js/napi ← frameworks/native/vibrator
                 frameworks/capi ← frameworks/native/vibrator
                 frameworks/cj ← frameworks/native/vibrator
                 frameworks/ets/taihe ← frameworks/native/vibrator
```

依赖方向：上层 → 下层，不可反向。`frameworks/` 不直接依赖 `services/` 内部实现，只通过 IPC 接口（`IMiscdeviceService.idl`）通信。
