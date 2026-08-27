# 专家经验 — 约束与反模式

## 约束

### 权限
- 振动器接口需 `ohos.permission.VIBRATE`（system_grant），在 Service 端校验（来源：core.md §7.1.1）
- 灯光控制接口需 `ohos.permission.SYSTEM_LIGHT_CONTROL`（system_grant）（来源：core.md §7.1.1）
- 进程级控制接口需 `ohos.permission.MANAGE_VIBRATOR`（来源：core.md §7.1.1）
- `systemUsage=true` 的系统服务调用（TOKEN_NATIVE）可绕过部分权限检查（来源：core.md §7.1.3）
- 敏感权限调用后通过 `AddPermissionUsedRecord()` 记录

### 线程安全
- `LightClient` 使用 `std::mutex`（`clientMutex_`、`lightInfosMutex_`）保护并发访问（来源：modules.md §2）
- `VibratorServiceClient` 单例模式，通过 `DelayedSpSingleton` 管理生命周期
- NAPI 回调通过 `EmitUvEventLoop` + `napi_send_event` 跨线程投递到 JS 线程，不可直接在回调线程调用 JS 函数（来源：modules.md §1 §8.3）
- `AsyncCallbackInfo` 继承 `RefBase`，使用引用计数管理生命周期

### 内存安全
- 灯光列表使用 `malloc` 分配，`ClearLightInfos()` 通过 `free` 释放，懒加载策略（来源：modules.md §2 §2.2）
- 自定义振动包解码后必须调用 `FreeVibratorPackage` 释放内存（来源：core.md §7.6）
- 使用 `strcpy_s` 而非 `strcpy`（来源：modules.md §2 `ConvertLightInfos`）
- IPC 数据结构实现 `Parcelable` 接口，通过 `Marshalling`/`Unmarshalling` 安全序列化（来源：core.md §5.5）

### 振动控制
- 振动前必须通过 `SetUsageEnhanced` 设置使用场景（来源：modules.md §1 §9.2）
- 振动模式四种类型：`time`/`preset`/`file`/`pattern`，参数必须匹配类型要求（来源：core.md §4.1）
- 振动参数校验：强度 [0-100]、频率 [0-100]、连续振动最大 5000ms、事件开始时间最大 1800000ms、最大事件数 128（来源：modules.md §1 §6.3）
- 曲线控制点数量 [4-16]（来源：modules.md §1 §6.3）

### 优先级管理
- `VibrationPriorityManager` 基于勿扰模式、铃声设置等条件决策振动是否执行（来源：core.md §3.1、§3.4）
- 优先级判断结果：`VIBRATION`（执行）或 `IGNORE_*`（忽略）（来源：core.md §3.4）
- 进程级控制：`DisableVibratorByPid` / `EnableVibratorByPid` 可按 PID 禁用/恢复（来源：core.md §4.3）

### HDI 连接
- HDI 服务死亡时通过 `DeathRecipient` 自动重连（来源：core.md §7.6 §6）
- 重连后自动恢复服务代理引用
- HDI 接口版本：IVibratorInterface V2.0、ILightInterface V1.0
- `min_hdi_proxy_version`: libvibrator_proxy_2.0.z.so（来源：sa_profile/3602.json）

### 安全编译
- PAC 签名（`branch_protector_ret = "pac_ret"`）
- CFI 检查（`cfi = true`）
- 栈保护（`-fstack-protector-all`）
- 边界检查（`boundary_sanitize = true`）
- 整数溢出检测（`integer_overflow = true`）
- UBSan（`ubsan = true`）（来源：core.md §7.3）

## 反模式

| 反模式 | 正确做法 |
|--------|----------|
| 振动前不设置 `SetUsageEnhanced` | 必须先设置使用场景再执行振动 |
| 解码振动包后不调用 `FreeVibratorPackage` | 使用完毕后立即释放内存 |
| 在振动回调线程直接调用 JS 函数 | 通过 `EmitUvEventLoop` 投递到 JS 线程 |
| 绕过 `VibrationPriorityManager` 直接调用 HDI | 必须经过优先级判断后再执行振动 |
| 使用 `strcpy` 复制字符串 | 使用 `strcpy_s` 安全拷贝 |
| 高频振动请求不做节流 | 系统会节流高频振动，防止硬件损坏 |
| 应用退出前不取消插拔事件订阅 | 退出前调用 `UnSubscribeVibratorPlug` 避免野指针 |
| 灯光列表使用后不释放 | `LightClient` 内部管理，但不要持有外部引用 |
| 修改 IPC 数据结构字段顺序而不更新 IDL | `IMiscdeviceService.idl` 是 IPC 契约，必须同步 |
| 跳过 `IsLightIdValid` 直接控制灯光 | 必须先校验 lightId 在设备列表中 |

## 依赖禁忌

- `frameworks/` **不可**直接依赖 `services/` 的内部头文件，只通过 IPC 接口通信
- `frameworks/native/light/` 和 `frameworks/native/vibrator/` **不可**互相依赖
- `utils/haptic_decoder/` **不可**依赖 `services/`，是独立解码模块
- `utils/` **不可**依赖 `frameworks/` 或 `services/`，是最底层基础库
- `hdi_connection/adapter/`（兼容）和 `interface/`（主路径）**不可**互相依赖
