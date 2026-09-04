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

## 充电桩管理列表

请求 action：`admin.charger.list`

```json
{
  "action": "admin.charger.list",
  "requestId": "admin-3",
  "data": {}
}
```

成功响应：

```json
{
  "requestId": "admin-3",
  "code": 0,
  "message": "ok",
  "data": {
    "chargers": [
      {
        "chargerId": 1001,
        "code": "CP-001",
        "stationId": 1,
        "stationName": "良乡大学城站",
        "type": 0,
        "powerKw": 120.0,
        "status": 0,
        "totalChargeCount": 36,
        "totalChargeDurationSeconds": 5400
      }
    ]
  }
}
```

字段口径：

| 字段 | 类型 | 含义 |
|---|---|---|
| `chargerId` | 正整数 | 数据库主键；客户端选中、排序和操作的唯一依据 |
| `code` | 字符串 | 面向运营人员展示的充电桩编号，不作为操作主键 |
| `stationId` | 正整数 | 所属站点主键 |
| `stationName` | 字符串 | 所属站点显示名称，由服务端关联查询返回 |
| `type` | 整数 | `0=快充`、`1=慢充`，与用户端一致 |
| `powerKw` | 正数 | 额定功率，单位 kW |
| `status` | 整数 | `0=空闲`、`1=在用`、`2=故障`、`3=离线` |
| `totalChargeCount` | 非负整数 | 该桩累计完成/计入次数；具体订单计入口径由成员 1 冻结 |
| `totalChargeDurationSeconds` | 非负整数 | 该桩累计充电时长，单位秒；由服务端聚合 |

- 当前 Commit 3 获取全量列表并提供显式刷新；没有在客户端伪造累计字段。
- 当前数据量下，编号/站名关键词、`stationId`、类型和状态在完整响应上进行
  客户端组合筛选。电站下拉选项由响应中的 `stationId + stationName` 动态生成，
  不使用写死站点；筛选只改变视图，不改变服务端事实。
- “故障待处理”数量来自本次列表响应中 `status=2` 的项目，只作为快捷筛选提示；
  它不是新的服务端统计口径，也不替代 `admin.charger.overview` 聚合接口。
- 若真实数据量需要分页，成员 1 应在此 action 的 `data` 中增加筛选和分页参数，
  并明确总记录数；届时筛选必须由服务端覆盖完整数据集，而不是只过滤当前页。
- 客户端把 `chargerId` 存在表格模型的自定义 role 中。即使点击表头改变排序，
  操作仍通过代理模型映射回源模型取得真实 ID，不使用视图行号或编号文本。
- 功率、累计次数和累计时长另存原始数值作为排序键，显示单位不参与排序。

## 充电桩远程重启

请求 action：`admin.charger.restart`

```json
{
  "action": "admin.charger.restart",
  "requestId": "admin-4",
  "data": { "chargerId": 1001 }
}
```

成功响应只表示服务端接受本次模拟重启指令：

```json
{
  "requestId": "admin-4",
  "code": 0,
  "message": "服务器已接受重启指令",
  "data": {
    "chargerId": 1001,
    "restartedAt": 1788480000000
  }
}
```

安全规则：

- 客户端发送前必须展示电桩编号、真实 ID、所属站点和当前状态，并二次确认。
- `status=1` 表示该桩已预约或正在充电，服务端必须拒绝重启，返回
  `code=2101` 和明确原因；这样不会破坏成员 2 的进行中订单。
- 空闲、故障、离线桩可以接受本项目中的“远程重启模拟”。实际硬件结果不由
  客户端假定。
- 成功后客户端不修改本地状态，而是重新调用 `admin.charger.list`；失败时保留
  当前列表并显示服务端原因。
- `chargerId` 不存在或格式错误返回 `1001`；未登录返回 `1003`。

> 待成员 1 冻结：累计次数是否只计算 `FINISHED` 订单，以及累计时长取
> `stopTime-startTime` 还是独立计量字段。客户端当前只展示服务端返回值。

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
| 2101 | 充电桩正在服务订单，拒绝运维操作 |
| 5000 | 服务端内部错误 |
