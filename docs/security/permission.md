# 权限模型与错误码

## 权限映射表

| 权限名 | 敏感级别 | 适用接口 |
|--------|---------|---------|
| `ohos.permission.VIBRATE` | system_grant | 所有振动相关接口 |
| `ohos.permission.SYSTEM_LIGHT_CONTROL` | system_grant | 灯光控制接口 |
| `ohos.permission.MANAGE_VIBRATOR` | system_grant | 进程级控制接口 |

详见 codewiki core.md §7.1.1(权限清单)。

## 权限校验流程

1. 应用调用 API
2. 检查权限是否声明 → 否则返回 201
3. 检查 TokenID 是否有效 → 否则返回 201
4. 检查权限是否已授予 → 否则返回 201
5. 通过后执行业务逻辑

详见 codewiki core.md §7.1.2(权限校验流程)。

## 系统调用豁免

`systemUsage=true` 的系统服务调用可绕过部分权限检查：

```cpp
bool VibrationPriorityManager::IsSystemCalling() {
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto flag = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (flag == ATokenTypeEnum::TOKEN_NATIVE) return true;  // Native 进程视为系统调用
    return TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetCallingFullTokenID());
}
```

详见 codewiki core.md §7.1.3(系统调用豁免)。

## 错误码

| 错误码 | 含义 | 处理建议 |
|--------|------|----------|
| 0 | 成功 | - |
| 201 | 权限不足 | 检查应用 `module.json5` 权限声明 |
| 401 | 参数错误 | 检查 API 调用参数 |
| 801 | 能力不支持 | 设备不具备相关 SysCap |
| 14600101+ | 设备错误 | 硬件层面操作失败，检查设备状态 |

详见 codewiki core.md §7.4(错误码体系)、modules.md §1 §9.3(错误处理)。

## 权限声明

应用在 `module.json5` 中声明：

```json
{
  "module": {
    "requestPermissions": [
      { "name": "ohos.permission.VIBRATE", "reason": "$string:reason_vibrate" }
    ]
  }
}
```

详见 codewiki core.md §6.2(权限声明)。
