# 构建和测试方法

## 构建目标

编译配置见 `bundle.json`，构建目标从 OpenHarmony 源码根目录执行：

| group_type | target |
|------------|--------|
| fwk_group | vibrator_js_target, vibrator_cj_target, vibrator_target, light_target, miscdevice_utils_target, ohvibrator, vibrator_taihe |
| service_group | miscdevice_service_target, sensors_sa_profiles, ohos-vibratorControl |

### 编译产物

| 产物 | 来源 |
|------|------|
| libmiscdevice_service.z.so | services/miscdevice_service/ |
| libmiscdevice_utils.z.so | utils/common/ |
| libvibrator_decoder.z.so | utils/haptic_decoder/ |
| libvibrator.z.so | frameworks/js/napi/ |
| libvibrator_client.z.so | frameworks/native/vibrator/ |
| liblight_client.z.so | frameworks/native/light/ |

### Feature flags

| flag | 说明 |
|------|------|
| `miscdevice_feature_vibrator_custom` | 自定义振动支持 |
| `miscdevice_feature_vibrator_input_method_enable` | 输入法振动支持 |
| `miscdevice_feature_crown_vibrator_enable` | 表冠振动器支持 |
| `miscdevice_feature_do_not_disturb_enable` | 勿扰模式支持 |
| `miscdevice_feature_phone_lite_qos_enable` | Lite 设备 QoS 支持 |

## 最小验证

```sh
# 检查 Miscdevice Service SA 3602 是否存活
hdc shell "hidumper -s 3602"

# 查看传感器服务进程（miscdevice 运行在 sensors 进程）
hdc shell "ps -ef | grep sensors"
```

## 测试

测试目标定义在 `bundle.json` 的 `build.test` 中：

| 测试类型 | 路径 | target |
|----------|------|--------|
| 振动器 JS 单元测试 | test/unittest/vibrator/js/ | unittest |
| 振动器 Native 单元测试 | test/unittest/vibrator/native/ | unittest |
| 振动器 CAPI 单元测试 | test/unittest/vibrator/capi/ | unittest |
| 呼吸灯单元测试 | test/unittest/light/ | unittest |
| 振动器 Fuzz | test/fuzztest/vibrator/ | fuzztest |
| 呼吸灯 Fuzz | test/fuzztest/light/ | fuzztest |
| 服务层 Fuzz | test/fuzztest/service/ | fuzztest |

## 调试

```sh
# 查看振动器/呼吸灯日志
hilog -x | grep -i -E "miscdevice|vibrator|light"

# 开启调试日志（需 root）
hdc shell "lshilog -s miscdevice -l D"

# 查看 SA 3602 状态
hdc shell "hidumper -s 3602"

# 查看振动器设备节点
hdc shell "ls -la /dev/vibrator*"

# 查看 HiSysEvent 振动器事件
hdc shell "hisysevent -l"

# 使用命令行振动控制工具
hdc shell "ohos-vibratorControl --help"
```

> 涉及真实振动器/呼吸灯硬件的行为，需要补充板侧证据。提交使用 `git commit -s`。
