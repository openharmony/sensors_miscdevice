# ohos-vibratorControl

## 概述
振动器控制工具，用于启动预置震动效果和查询震动效果支持情况。

## 功能列表
- 启动预置震动效果
- 查询预置震动效果是否支持

## 依赖
- 系统能力：SystemCapability.Sensors.MiscDevice
- 权限：ohos.permission.VIBRATE

## 基本用法
```
ohos-vibratorControl <command> [options]
```

## 命令列表

| 命令 | 说明 | 参数 | 权限 | 前置依赖 |
|------|------|------|------|----------|
| startVibrator | 启动预置震动 | --effectId | ohos.permission.VIBRATE | 无 |
| isSupportEffect | 查询震动效果是否支持 | --effectId | ohos.permission.VIBRATE | 无 |

### 前置依赖说明：
- **无**：该命令可直接执行，无需前置条件
- **必须**：必须先执行前置命令才能成功执行
- **可选**：建议先执行前置命令以确保状态正确

## 示例
```bash
# 查看帮助信息
ohos-vibratorControl --help

# 查看子命令帮助
ohos-vibratorControl startVibrator --help
ohos-vibratorControl isSupportEffect --help

# 启动预置震动（无前置依赖）
ohos-vibratorControl startVibrator --effectId haptic.clock.timer

# 查询震动效果是否支持（无前置依赖）
ohos-vibratorControl isSupportEffect --effectId haptic.clock.timer
```

## 常用 effectId 列表

| 效果 ID | 描述 |
|---------|------|
| haptic.clock.timer | 计时器振动 |
| haptic.notice.fail | 失败提醒振动 |
| haptic.calib.charge | 充电振动 |
| haptic.long_press_light | 轻触长按振动 |
| haptic.long_press_medium | 中等长按振动 |
| haptic.long_press_heavy | 重度长按振动 |
| haptic.slide | 滑动振动 |
| haptic.common.threshold | 阈值振动 |
| haptic.effect.hard | 硬振动 |
| haptic.effect.soft | 软振动 |
| haptic.effect.sharp | 锐利振动 |
| haptic.notice.success | 成功提醒振动 |
| haptic.notice.warning | 警告提醒振动 |