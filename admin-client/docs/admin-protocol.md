# 管理员客户端协议约定

> 当前状态：客户端与 mock server 已按本约定实现；成员 A 的真实服务端应按此对接。

## 传输与消息外壳

- 长连接 TCP。
- 每帧为 4 字节大端 payload 长度，加 UTF-8 JSON payload。
- 请求包含 `action`、`requestId`、`data`；服务端必须原样返回 `requestId`。
- `code=0` 表示成功。

## 管理员登录

请求 action：`admin.login`

```json
{
  "action": "admin.login",
  "requestId": "admin-1",
  "data": {
    "account": "admin",
    "password": "123456"
  }
}
```

成功响应：

```json
{
  "requestId": "admin-1",
  "code": 0,
  "message": "ok",
  "data": {
    "adminId": 1,
    "account": "admin",
    "displayName": "系统管理员"
  }
}
```

`admin/123456` 是数据库与 mock server 的初始种子账号，不是客户端校验条件。

## 充电桩状态总览

请求 action：`admin.charger.overview`

```json
{
  "action": "admin.charger.overview",
  "requestId": "admin-2",
  "data": {}
}
```

成功响应：

```json
{
  "requestId": "admin-2",
  "code": 0,
  "message": "ok",
  "data": {
    "total": 12,
    "idle": 6,
    "charging": 3,
    "fault": 2,
    "offline": 1,
    "idlePercent": 50.0,
    "chargingPercent": 25.0,
    "faultPercent": 16.7,
    "offlinePercent": 8.3,
    "updatedAt": 1788480000000
  }
}
```

### 统计口径

- 状态值与用户端保持一致：`0=空闲`、`1=充电中/在用`、`2=故障`、
  `3=离线`。管理员接口返回具名字段，不直接暴露枚举数字。
- 用户端当前行为是预约成功即占用桩，因此 `RESERVED` 与 `CHARGING`
  对应的桩都计入 `charging`；订单结算完成后释放为 `idle`。
- `total = idle + charging + fault + offline`。离线必须纳入全集，否则总数
  无法与各状态数量之和一致。
- `*Percent` 是百分数而非 0~1 比例，范围为 0~100；非零数据四项之和允许
  因四舍五入存在不超过 0.25 的误差。
- 当 `total=0` 时，四个数量和四个百分比都必须为 0，客户端直接显示
  `0.0%`，不会自行除法。
- `updatedAt` 为可选的 epoch 毫秒时间戳。真实数量、百分比和时间戳均由
  服务端一次性聚合返回；客户端不下载全部充电桩重复统计。
- 此接口要求管理员连接已经登录，未登录返回 `code=1003`。

### 跨客户端验收

1. 记录管理员总览中的四类数量。
2. 用户端执行预约；刷新总览后，`idle` 应减少、`charging` 应增加。
3. 用户端开始充电；状态仍属于 `charging`，总数不应变化。
4. 用户端停止并完成结算；刷新后 `charging` 应减少、`idle` 应恢复。
5. 若数字不一致，应检查服务端订单到桩状态的映射与数据库事务，不在管理端
   UI 层增减或补偿数字。

> 对齐状态：用户端 `zlflly-user-client` 已采用上述四状态枚举和“预约即占用、
> 结算后释放”的行为；成员 A 的 `fengshu-server-db` 分支尚未实现聚合接口。
> 当前管理员 mock 的固定数据只用于客户端联调，不能作为生产统计来源。

## 会话规则

- 当前版本不使用 token，服务端在登录成功后把 `adminId` 绑定到该 TCP 连接。
- 后续所有管理员写操作必须由服务端检查当前连接是否已经认证。
- TCP 断开即视为会话失效；客户端清空 `AdminSession` 并返回登录窗口。
- 客户端不持久化管理员密码，也不自动重放登录请求。
- 未来若改为 token，只在 `AdminApiClient` 中统一附加认证信息。

## 错误码

| code | 含义 |
|---:|---|
| 0 | 成功 |
| 1001 | 请求参数错误 |
| 1003 | 未登录或会话失效 |
| 1101 | 管理员账号或密码错误 |
| 5000 | 服务端内部错误 |
