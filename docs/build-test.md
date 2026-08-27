# 构建和测试方法

## 编译

构建命令从 OpenHarmony 源码根目录执行：

```sh
# 编译 miscdevice 部件（含框架+服务+工具）
./build.sh --product-name rk3568 --build-target miscdevice --ccache

# 单独编译框架层
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 vibrator_target
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 light_target

# 单独编译服务层
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 miscdevice_service_target
```

### 编译产物

| 产物 | 路径 | 来源 |
|------|------|------|
| libmiscdevice_service.z.so | `out/{device}/system/lib64/` | services/miscdevice_service/ |
| libmiscdevice_utils.z.so | `out/{device}/system/lib64/` | utils/common/ |
| libvibrator_decoder.z.so | `out/{device}/system/lib64/` | utils/haptic_decoder/ |
| libvibrator.z.so | `out/{device}/system/lib64/` | frameworks/js/napi/ |
| libvibrator_client.z.so | `out/{device}/system/lib64/` | frameworks/native/vibrator/ |
| liblight_client.z.so | `out/{device}/system/lib64/` | frameworks/native/light/ |

### 构建目标（bundle.json）

| group_type | target |
|------------|--------|
| fwk_group | vibrator_js_target, vibrator_cj_target, vibrator_target, light_target, miscdevice_utils_target, ohvibrator, vibrator_taihe |
| service_group | miscdevice_service_target, sensors_sa_profiles, ohos-vibratorControl |

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
# 1. 检查 Miscdevice Service SA 3602 是否存活
hdc shell "hidumper -s 3602"

# 2. 查看传感器服务进程（miscdevice 运行在 sensors 进程）
hdc shell "ps -ef | grep sensors"

# 3. 检查 SA 配置文件
hdc shell "cat /system/profile/3602.json"
```

## 测试

```sh
# 单元测试 target（来自 bundle.json build.test）
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 unittest
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 fuzztest
```

| 测试类型 | 路径 | target |
|----------|------|--------|
| 振动器 JS 单元测试 | test/unittest/vibrator/js/ | unittest |
| 振动器 Native 单元测试 | test/unittest/vibrator/native/ | unittest |
| 振动器 CAPI 单元测试 | test/unittest/vibrator/capi/ | unittest |
| 灯光单元测试 | test/unittest/light/ | unittest |
| 振动器 Fuzz | test/fuzztest/vibrator/ | fuzztest |
| 灯光 Fuzz | test/fuzztest/light/ | fuzztest |
| 服务层 Fuzz | test/fuzztest/service/ | fuzztest |

## 调试

```sh
# 查看振动器/灯光日志
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

## 部署推送

```sh
# 编译后推送 .so 到设备
hdc file send out/rk3568/system/lib64/libmiscdevice_service.z.so /system/lib64/
hdc file send out/rk3568/system/lib64/libmiscdevice_utils.z.so /system/lib64/
hdc file send out/rk3568/system/lib64/libvibrator_decoder.z.so /system/lib64/

# 重启服务
hdc shell "kill -9 $(pidof sensors)"  # SA 会被自动拉起
```

> 涉及真实振动器/灯光硬件的行为，需要补充板侧证据。提交使用 `git commit -s`。
