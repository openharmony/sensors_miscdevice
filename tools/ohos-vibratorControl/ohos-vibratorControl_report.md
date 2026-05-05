# ohos-vibratorControl 封装报告

## 1. 概述

| 项目 | 内容 |
|------|------|
| CLI 名称 | ohos-vibratorControl |
| 功能描述 | 振动器控制工具，用于启动预置震动和查询震动效果支持情况 |
| 仓库类型 | 开源仓（ohos- 前缀） |
| 代码路径 | tools/ohos-vibratorControl/ |

## 2. 接口分析

### 2.1 候选接口列表

| 接口名 | 来源 | 方法签名 | 权限需求 | 推荐封装层级 |
|--------|------|----------|----------|--------------|
| StartVibrator | inner_kits | `int32_t StartVibrator(const char *effectId)` | ohos.permission.VIBRATE | inner_kits |
| IsSupportEffect | inner_kits | `int32_t IsSupportEffect(const char *effectId, bool *state)` | ohos.permission.VIBRATE | inner_kits |

### 2.2 权限需求分析

| 权限标识 | 用途说明 | 涉及命令 |
|----------|----------|----------|
| ohos.permission.VIBRATE | 振动器控制权限 | startVibrator, isSupportEffect |

### 2.3 异步分析结果

| 接口 | 类型 | 判断依据 | 封装决策 |
|------|------|----------|----------|
| StartVibrator | 同步 | 通过 IPC 同步调用服务层 | ✅ 封装 |
| IsSupportEffect | 同步 | 直接返回查询结果 | ✅ 封装 |

## 3. 命令设计

### 3.1 命令列表

| CLI 命令 | 对应接口 | 权限 | 参数 | 说明 |
|----------|----------|------|------|------|
| startVibrator | OHOS::Sensors::StartVibrator() | ohos.permission.VIBRATE | --effectId | 启动预置震动 |
| isSupportEffect | OHOS::Sensors::IsSupportEffect() | ohos.permission.VIBRATE | --effectId | 查询效果是否支持 |

### 3.2 命令依赖关系

| CLI 命令 | 前置依赖命令 | 依赖条件 | 说明 |
|----------|--------------|----------|------|
| startVibrator | 无 | - | 直接调用振动器接口 |
| isSupportEffect | 无 | - | 直接调用振动器接口 |

## 4. 代码实现

### 4.1 文件结构

```
tools/ohos-vibratorControl/
├── src/
│   └── main.cpp              # 主入口，参数解析和命令分发
├── tests/
│   └── TEST.md               # 测试文档
├── docs/
│   └── README.md             # 工具使用说明
├── BUILD.gn                  # 构建配置
└── ohos-vibratorControl.json # 工具描述文件
```

### 4.2 关键代码说明

**命令分发：** 使用 std::unordered_map 静态命令表

```cpp
static std::unordered_map<std::string, Command> gCommands;
```

**错误码对齐：**

```cpp
constexpr int32_t VIBRATOR_SUCCESS = 0;
```

### 4.3 代码修复记录
- 修复 CLI_LOG 宏的表达式未使用错误（使用 fprintf 替代）
- 添加命名空间 OHOS::Sensors:: 到 API 调用
- 移除未使用的常量变量

## 5. 编译验证

### 5.1 编译配置

| 项目 | 值 |
|------|-----|
| ABI 类型 | generic_generic_arm_64only |
| 设备类型 | general_all_phone_standard |
| 编译命令 | `./build_system.sh --abi-type generic_generic_arm_64only --device-type general_all_phone_standard --build-target miscdevice --build-variant root` |

### 5.2 编译过程

**尝试 1-3：**

- **结果:** 失败
- **错误:** 多种编译错误（缺少 import、宏表达式错误、API 命名空间、变量未使用等）
- **修复:**
  - 添加 `import("//build/ohos.gni")` 和 `import("./../../miscdevice.gni")`
  - 添加 `part_name = "miscdevice"`
  - 修复 sanitize 块格式
  - 修复 CLI_LOG 宏
  - 添加命名空间 OHOS::Sensors::
  - 移除未使用的变量

**尝试 4：**

- **结果:** ✅ 成功

### 5.3 最终结果

✅ 编译成功

- **尝试次数:** 4
- **产物路径:** `out/generic_generic_arm_64only/general_all_phone_standard/sensors/miscdevice/ohos-vibratorControl`
- **文件大小:** 43520 bytes

## 6. 使用说明

### 6.1 安装路径
```
/system/bin/cli_tool/executable/ohos-vibratorControl
```

### 6.2 命令示例

**启动预置震动：**

```bash
ohos-vibratorControl startVibrator --effectId haptic.clock.timer
```

**查询效果支持：**

```bash
ohos-vibratorControl isSupportEffect --effectId haptic.long.press.heavy
```

**查看帮助：**

```bash
ohos-vibratorControl --help
ohos-vibratorControl startVibrator --help
```

### 6.3 权限配置
使用前需确保拥有以下权限：

```
ohos.permission.VIBRATE
```