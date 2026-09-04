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

## 充电桩运维状态变更

请求 action：`admin.charger.status.update`

```json
{
  "action": "admin.charger.status.update",
  "requestId": "admin-5",
  "data": {
    "chargerId": 1001,
    "expectedStatus": 0,
    "targetStatus": 3,
    "reason": "计划检修下线"
  }
}
```

成功响应：

```json
{
  "requestId": "admin-5",
  "code": 0,
  "message": "设备状态已更新",
  "data": {
    "chargerId": 1001,
    "previousStatus": 0,
    "status": 3,
    "changedAt": 1788480000000
  }
}
```

业务约束：

- 这是实训项目的“模拟运维状态”接口，不是客户端直接控制真实硬件。
- `targetStatus` 只允许 `0=空闲`、`2=故障`、`3=离线`；管理员不能把设备
  手工设置为 `1=在用`，`1` 只能由预约/充电订单状态机产生。
- 若当前状态为 `1`，或服务端查询到关联的活动订单，必须拒绝并返回 `2101`。
- `expectedStatus` 必须与服务端事务内读取到的当前状态相同；不一致返回 `2103`，
  防止管理员依据旧页面覆盖刚发生的订单或设备变化。
- `reason` 去除首尾空白后为 2～200 个字符。真实服务端应记录管理员 ID、原因、
  前后状态和操作时间，作为审计记录。
- 相同状态的无效变更返回 `1001`。客户端写成功后必须重新查询，不在本地直接
  改行；站点页还要重查站点列表，使 `onlineRate` 与详情同步。

## 充电站管理列表

请求 action：`admin.stations.list`

```json
{
  "action": "admin.stations.list",
  "requestId": "admin-5",
  "data": {}
}
```

成功响应：

```json
{
  "requestId": "admin-5",
  "code": 0,
  "message": "ok",
  "data": {
    "stations": [
      {
        "stationId": 1,
        "name": "良乡大学城站",
        "address": "北京市房山区良乡大学城北路",
        "latitude": 39.731320,
        "longitude": 116.171590,
        "totalCount": 4,
        "onlineRate": 100.0,
        "version": 1
      }
    ]
  }
}
```

字段口径：

| 字段 | 类型 | 含义 |
|---|---|---|
| `stationId` | 正整数 | 站点数据库主键；选择、排序和详情查询的唯一依据 |
| `name` | 1～60 字符 | 站点显示名称 |
| `address` | 1～200 字符 | 站点详细地址 |
| `latitude` | 数字 | WGS-84 纬度，范围 -90～90 |
| `longitude` | 数字 | WGS-84 经度，范围 -180～180 |
| `totalCount` | 非负整数 | 该站全部充电桩数量，包含空闲、在用、故障和离线 |
| `onlineRate` | 数字 | 服务端聚合的百分数，范围 0～100，不是 0～1 比例 |
| `version` | 正整数 | 站点资料版本；编辑时用于防止并发覆盖 |

在线率在本项目中的统一定义：

- `onlineCount = idle + charging + fault`，仅 `offline` 不计在线；故障表示设备
  已上报故障，仍属于在线设备。
- `onlineRate = onlineCount / totalCount * 100`，由服务端基于同一次数据快照聚合。
- `totalCount=0` 时固定返回 `onlineRate=0.0`。
- 客户端只校验并显示 `totalCount`、`onlineRate`，不下载桩列表重复计算在线率。
- 此接口要求当前 TCP 连接已完成管理员登录，未登录返回 `1003`。

> action 名使用成员 1 协议框架中已有的复数形式 `admin.stations.list`。现有
> Commit 3 的 `admin.charger.list/restart` 暂不在本节点改名，避免破坏已验证功能；
> 真实服务端接入前需单独统一这组历史名称。

## 编辑充电站资料

请求 action：`admin.stations.update`

```json
{
  "action": "admin.stations.update",
  "requestId": "admin-7",
  "data": {
    "stationId": 1,
    "expectedVersion": 1,
    "name": "良乡大学城智慧站",
    "address": "北京市房山区良乡大学城北路",
    "latitude": 39.731320,
    "longitude": 116.171590
  }
}
```

成功响应：

```json
{
  "requestId": "admin-7",
  "code": 0,
  "message": "充电站资料已更新",
  "data": {
    "stationId": 1,
    "version": 2,
    "updatedAt": 1788480000000
  }
}
```

- 可编辑字段只有站名、地址和 WGS-84 经纬度，长度与坐标范围和新增接口一致。
- `stationId`、`totalCount`、`onlineRate` 不可编辑；桩数和在线率必须由服务端
  根据设备事实重新聚合。
- 服务端必须在一个事务中比较 `expectedVersion` 并更新；版本不一致返回 `2102`，
  客户端提示冲突并重新加载，不自动重放旧修改。
- 更新站名时，`admin.charger.list` 中的 `stationName` 必须来自最新站点记录，
  不能在充电桩表中保存一份无法同步的冗余真值。
- 同名同址的重复站点返回 `1001`。本节点不提供硬删除：存在充电桩、订单或统计
  记录时删除语义需要成员 A 单独设计，不能由客户端直接级联删除。

## 站内充电桩详情

管理员端复用用户端已经冻结的 action：`station.detail`，不再设计第二套重复接口。

请求：

```json
{
  "action": "station.detail",
  "requestId": "admin-6",
  "data": { "stationId": 1 }
}
```

成功响应：

```json
{
  "requestId": "admin-6",
  "code": 0,
  "message": "ok",
  "data": {
    "stationId": 1,
    "version": 1,
    "name": "良乡大学城站",
    "address": "北京市房山区良乡大学城北路",
    "latitude": 39.731320,
    "longitude": 116.171590,
    "chargers": [
      {
        "chargerId": 1001,
        "chargerCode": "CP-001",
        "type": 0,
        "power": 120.0,
        "status": 0,
        "pricePerKwh": 1.20
      }
    ]
  }
}
```

- `chargers` 必须始终存在；无桩站点返回空数组 `[]`，不返回 `null`。
- `version` 与站点列表一致；成功编辑后列表和详情都返回递增的新版本。
- `chargerCode`、`power` 沿用用户端已冻结字段，管理员端不要求服务端为详情再做
  `code/powerKw` 别名。
- `type`、`status` 使用本文前述统一整数枚举；`pricePerKwh` 为非负元/度价格。
- 客户端校验响应 `stationId` 与请求一致，并用选择 generation 忽略迟到详情响应。

## 新增充电站与初始桩

请求 action：`admin.stations.create`

```json
{
  "action": "admin.stations.create",
  "requestId": "admin-7",
  "data": {
    "name": "通州智慧能源站",
    "address": "北京市通州区运河东大街",
    "latitude": 39.902500,
    "longitude": 116.656300,
    "chargerCount": 4
  }
}
```

输入约束：

- `name` 去除首尾空白后为 1～60 个字符。
- `address` 去除首尾空白后为 1～200 个字符。
- `latitude`、`longitude` 分别在 -90～90、-180～180 范围内。
- `chargerCount` 是 0～100 的整数；允许创建尚未投运的无桩站点。
- 此接口要求管理员登录；无效字段返回 `1001`，会话失效返回 `1003`，内部或
  事务失败返回 `5000`，具体原因放在 `message`。
- 服务端拒绝同名同址的重复站点并返回 `1001`；客户端不依赖本地列表做唯一性
  判断，避免分页或并发情况下漏判。

成功响应：

```json
{
  "requestId": "admin-7",
  "code": 0,
  "message": "充电站创建成功",
  "data": {
    "stationId": 5,
    "createdChargerCount": 4
  }
}
```

一致性规则：

- 服务端负责生成 `stationId`、初始 `chargerId` 和唯一可展示的桩编号。
- 初始桩的类型、功率和电价属于服务端配置/种子策略，不由客户端请求指定；Mock
  中的交替快慢充数据仅用于界面验证，不是数据库实现的业务默认值。
- 站点和全部初始桩必须在一个数据库事务中创建；任意一步失败整体回滚。
- 成功时 `createdChargerCount` 必须等于请求的 `chargerCount`，否则客户端将响应
  视为异常并不刷新为成功状态。
- 客户端不乐观插入本地行。收到成功响应后重新调用 `admin.stations.list`，再按
  新 `stationId` 调用 `station.detail`；失败时保留当前列表。
- 成员 2 的用户端随后通过 `station.nearby`/`station.detail` 查询同一数据库事实，
  作为真实服务端阶段的跨端验收。

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
| 2102 | 站点版本冲突，旧资料不得覆盖新版本 |
| 2103 | 充电桩当前状态与 `expectedStatus` 不一致 |
| 5000 | 服务端内部错误 |
