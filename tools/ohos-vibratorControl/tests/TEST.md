# 测试文档

## 测试覆盖
本测试文档覆盖 ohos-vibratorControl CLI 工具的所有命令和参数组合。

## 命令列表

| 命令 | 参数 | 必填 | 描述 |
|------|------|------|------|
| startVibrator | --effectId | 是 | 预置震动效果ID |
| isSupportEffect | --effectId | 是 | 预置震动效果ID |

## 测试用例

### 1. help 命令测试

| 测试用例 | 命令 | 预期结果 |
|----------|------|----------|
| 全量帮助 | `ohos-vibratorControl --help` | 显示所有命令列表 |
| 子命令帮助 | `ohos-vibratorControl help` | 显示所有命令列表 |
| startVibrator帮助 | `ohos-vibratorControl startVibrator --help` | 显示startVibrator详细帮助 |
| isSupportEffect帮助 | `ohos-vibratorControl isSupportEffect --help` | 显示isSupportEffect详细帮助 |

### 2. startVibrator 命令测试

| 测试用例 | 命令 | 预期结果 |
|----------|------|----------|
| 启动计时器震动 | `ohos-vibratorControl startVibrator --effectId haptic.clock.timer` | 返回成功JSON，status为vibrating |
| 启动长按轻震动 | `ohos-vibratorControl startVibrator --effectId haptic.long.press.light` | 返回成功JSON，status为vibrating |
| 启动长按中等震动 | `ohos-vibratorControl startVibrator --effectId haptic.long.press.medium` | 返回成功JSON，status为vibrating |
| 启动长按重震动 | `ohos-vibratorControl startVibrator --effectId haptic.long.press.heavy` | 返回成功JSON，status为vibrating |
| 启动点击震动 | `ohos-vibratorControl startVibrator --effectId haptic.click` | 返回成功JSON，status为vibrating |
| 启动滑动轻震动 | `ohos-vibratorControl startVibrator --effectId haptic.slide.light` | 返回成功JSON，status为vibrating |
| 启动滑动中等震动 | `ohos-vibratorControl startVibrator --effectId haptic.slide.medium` | 返回成功JSON，status为vibrating |
| 启动滑动重震动 | `ohos-vibratorControl startVibrator --effectId haptic.slide.heavy` | 返回成功JSON，status为vibrating |
| 缺少effect-id参数 | `ohos-vibratorControl startVibrator` | 返回失败JSON，errCode为ERR_VIB_ARG_MISSING |
| 无效effect-id | `ohos-vibratorControl startVibrator --effectId invalid.effect` | 返回失败JSON，errCode为ERR_VIB_START_FAILED |

### 3. isSupportEffect 命令测试

| 测试用例 | 命令 | 预期结果 |
|----------|------|----------|
| 查询计时器震动支持 | `ohos-vibratorControl isSupportEffect --effectId haptic.clock.timer` | 返回成功JSON，包含isSupported字段 |
| 查询长按震动支持 | `ohos-vibratorControl isSupportEffect --effectId haptic.long.press.heavy` | 返回成功JSON，包含isSupported字段 |
| 查询点击震动支持 | `ohos-vibratorControl isSupportEffect --effectId haptic.click` | 返回成功JSON，包含isSupported字段 |
| 查询滑动震动支持 | `ohos-vibratorControl isSupportEffect --effectId haptic.slide.light` | 返回成功JSON，包含isSupported字段 |
| 查询无效效果 | `ohos-vibratorControl isSupportEffect --effectId invalid.effect.xxx` | 返回成功JSON，isSupported为false |
| 缺少effect-id参数 | `ohos-vibratorControl isSupportEffect` | 返回失败JSON，errCode为ERR_VIB_ARG_MISSING |

### 4. 错误处理测试

| 测试用例 | 命令 | 预期结果 |
|----------|------|----------|
| 未知命令 | `ohos-vibratorControl unknownCmd` | 返回失败JSON，errCode为ERR_VIB_INVALID_COMMAND |
| 空命令 | `ohos-vibratorControl` | 显示用法错误信息 |

## 测试矩阵

| 命令 | 参数组合 | 测试数量 |
|------|----------|----------|
| help | --help, help, help startVibrator, help isSupportEffect | 4 |
| startVibrator | --effectId (有效), --effectId (无效), 缺少参数 | 10 |
| isSupportEffect | --effectId (有效), --effectId (无效), 缺少参数 | 6 |
| 错误处理 | unknownCmd, 空命令 | 2 |
| **总计** | | **22** |