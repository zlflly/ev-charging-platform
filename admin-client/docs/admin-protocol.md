# 管理员客户端协议约定

> 对齐基线：成员 A `fengshu-server-db` 分支的《协议冻结说明》《对接说明》及
> `StationService` 实际响应。公共接口以成员 A 为准；管理员未实现部分在本文标为
> “待服务端冻结扩展”，不得反向修改已冻结的用户端字段。

## 传输与消息外壳

- 长连接 TCP。
- 默认联调地址为 `127.0.0.1:8888`，与成员 A `ServerConfig` 一致；管理员 Mock
  也监听该端口，避免测试通过后切换真实服务端时仍连向旧端口。
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

请求 action：`admin.chargers.list`

```json
{
  "action": "admin.chargers.list",
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

请求 action：`admin.chargers.restart`

```json
{
  "action": "admin.chargers.restart",
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
- 成功后客户端不修改本地状态，而是重新调用 `admin.chargers.list`；失败时保留
  当前列表并显示服务端原因。
- `chargerId` 不存在或格式错误返回 `1001`；未登录返回 `1003`。

> 待成员 1 冻结：累计次数是否只计算 `FINISHED` 订单，以及累计时长取
> `stopTime-startTime` 还是独立计量字段。客户端当前只展示服务端返回值。

## 充电桩运维状态变更

> 待服务端冻结扩展：成员 A 当前权威 action 清单尚未包含此接口。客户端、Mock
> 和测试先按下述契约实现；接入真实数据库前，成员 A 应把 action、字段、事务检查
> 和 `2101/2103` 写入《协议冻结说明》并实现，不能只依赖客户端限制。

请求 action：`admin.chargers.status.update`

```json
{
  "action": "admin.chargers.status.update",
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
        "pricePerKwh": 1.50,
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
| `pricePerKwh` | 正数 | 站点统一充电单价，单位元/度；与公共 `station.detail` 一致 |
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

> action 名使用成员 A 协议框架中的复数资源形式。充电桩列表与重启也统一为
> `admin.chargers.list/restart`，避免客户端和服务端各保留一套名称。

## 编辑充电站资料

> 待服务端冻结扩展：成员 A 当前只列出站点查询与创建，尚未实现编辑接口。
> `version/expectedVersion` 是为避免多个管理员互相覆盖而新增的并发控制字段；
> 服务端落库时需要为 stations 增加版本列或实现等价的原子比较更新。

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
    "longitude": 116.171590,
    "pricePerKwh": 1.50
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

- 可编辑字段只有站名、地址、WGS-84 经纬度和站点级 `pricePerKwh`，长度、
  坐标范围及电价约束和新增接口一致。
- `stationId`、`totalCount`、`onlineRate` 不可编辑；桩数和在线率必须由服务端
  根据设备事实重新聚合。
- 服务端必须在一个事务中比较 `expectedVersion` 并更新；版本不一致返回 `2102`，
  客户端提示冲突并重新加载，不自动重放旧修改。
- 更新站名时，`admin.chargers.list` 中的 `stationName` 必须来自最新站点记录，
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
    "pricePerKwh": 1.50,
    "availableCount": 2,
    "totalCount": 4,
    "chargers": [
      {
        "chargerId": 1001,
        "code": "CP-001",
        "type": 0,
        "status": 0,
        "powerKw": 120.0
      }
    ]
  }
}
```

- `chargers` 必须始终存在；无桩站点返回空数组 `[]`，不返回 `null`。
- 公共详情不增加管理员专用的 `version` 字段；编辑使用
  `admin.stations.list` 返回的版本，避免破坏成员 2 已接入的响应结构。
- 单桩字段严格使用成员 A 已冻结并实现的 `code`、`powerKw`；不要求服务端提供
  `chargerCode/power` 别名。
- `pricePerKwh` 是站点级正数，站内所有桩采用该价格；客户端可在每个桩行重复
  展示，但不能把它误解为每桩独立字段。
- `availableCount` 是 `status=0` 的桩数，`totalCount` 等于 `chargers` 数组长度；
  客户端校验二者，避免展示互相矛盾的统计。
- `type`、`status` 使用本文前述统一整数枚举。
- 客户端校验响应 `stationId` 与请求一致，并用选择 generation 忽略迟到详情响应。

## 新增充电站与初始桩

> 成员 A 已冻结 `admin.stations.create` action 名，但管理员请求/响应字段尚未在
> 权威文档中补齐；下述字段是依据其现有 stations/chargers 数据模型形成的待冻结
> 契约，其中 `pricePerKwh` 必须位于站点级。

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
    "pricePerKwh": 1.50,
    "chargerCount": 4
  }
}
```

输入约束：

- `name` 去除首尾空白后为 1～60 个字符。
- `address` 去除首尾空白后为 1～200 个字符。
- `latitude`、`longitude` 分别在 -90～90、-180～180 范围内。
- `pricePerKwh` 是站点统一充电单价，必须为有限正数；它与成员 A 的 stations
  数据模型及公共 `station.detail` 保持一致。
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
- 初始桩的类型、功率属于服务端配置/种子策略，不由客户端请求指定；所有初始桩
  使用请求中的站点级 `pricePerKwh`。Mock 中交替快慢充仅用于界面验证。
- 站点和全部初始桩必须在一个数据库事务中创建；任意一步失败整体回滚。
- 成功时 `createdChargerCount` 必须等于请求的 `chargerCount`，否则客户端将响应
  视为异常并不刷新为成功状态。
- 客户端不乐观插入本地行。收到成功响应后重新调用 `admin.stations.list`，再按
  新 `stationId` 调用 `station.detail`；失败时保留当前列表。
- 成员 2 的用户端随后通过 `station.nearby`/`station.detail` 查询同一数据库事实，
  作为真实服务端阶段的跨端验收。

## 用户列表与服务端搜索

> 成员 A 已冻结 `admin.users.list` action、用户状态枚举以及 Repository 的用户字段，
> 但管理员请求/响应字段仍等待其 Commit 6 纳入权威文档。以下是客户端先行的最小
> 待冻结契约；Mock 只验证契约，不代表真实数据库。

请求 action：`admin.users.list`

```json
{
  "action": "admin.users.list",
  "requestId": "admin-10",
  "data": {
    "page": 1,
    "pageSize": 20,
    "phoneKeyword": "3800",
    "status": 0,
    "activityFilter": "ACTIVE"
  }
}
```

- `page` 从 1 开始；`pageSize` 范围 1～100。
- `phoneKeyword` 必须是 0～11 位数字。空字符串表示返回全部；查询使用手机号
  `contains`/SQL `LIKE %keyword%` 语义，并且必须由服务端筛选，客户端不在当前页
  上冒充全库查询。
- `status` 可省略；省略表示全部，`0` 正常，`1` 冻结。
- `activityFilter` 必填：`ALL` 全部、`ACTIVE` 存在活跃订单、`IDLE` 无活跃订单。
  活跃订单严格指 A 已冻结的 `RESERVED / CHARGING / WAIT_SETTLEMENT`，不包含
  `FINISHED`；此筛选同样必须在服务端分页前完成。
- 客户端仅在点击“查询”、按回车、翻页或刷新时请求，不随每个输入字符发包。

成功响应：

```json
{
  "requestId": "admin-10",
  "code": 0,
  "message": "ok",
  "data": {
    "users": [
      {
        "userId": 3,
        "phone": "13800138003",
        "nickname": "充电中用户",
        "balance": 150.00,
        "createdAt": 1788421800000,
        "status": 0,
        "activityStatus": "CHARGING",
        "activeOrder": {
          "orderId": 9003,
          "status": "CHARGING",
          "stationId": 2,
          "stationName": "中关村科技园站",
          "chargerId": 1007,
          "chargerCode": "CP-007"
        }
      }
    ],
    "total": 1,
    "page": 1,
    "pageSize": 20
  }
}
```

- `total` 是全部匹配记录数，不是本页数组长度；结果按 `userId` 升序，保证翻页稳定。
- `balance` 是元，客户端统一保留两位小数；数据库写入与扣款精度仍由服务端保证。
- A 的 `UserRepository::User` 当前把 `createdAt` 保存为 epoch 毫秒，但权威文档的
  通用时间规则仍写 ISO 8601。客户端过渡期同时接受 epoch 毫秒和合法 ISO 8601，
  内部统一转 epoch 毫秒后按本地时区显示；成员 A 在 Commit 6 必须冻结最终线格式。
- 空结果返回 `users: []`、`total: 0`，不返回 `null`。
- `activityStatus` 取 `IDLE` 或三个活跃订单状态；`IDLE` 时 `activeOrder` 必须为
  JSON `null`，其他状态必须带完整对象且内部 `status` 与外层一致。
- `WAIT_SETTLEMENT` 是成员 A 订单状态机与用户端共同使用的真实值。其含义是充电
  已停止、最终电量和应付金额已生成，但扣款及 `FINISHED` 状态写入尚未成功；管理端
  与用户端统一显示“待支付”。它仍属于未完成订单，但客户端不得据此推断桩仍在物理充电。
- 截至成员 A 分支当前实现，`order.stop` 只将订单推进到 `WAIT_SETTLEMENT`，而
  `order.settle` 成功后才把桩恢复为空闲；这与早期“停止即释放桩”的文字约定存在
  差异，必须由成员 A 冻结最终规则。管理端只显示服务端返回的关联设备，不自行改桩状态。
- `activeOrder` 字段直接复用 A 已实现的订单和关联实体命名。管理端不调用用户专属
  `order.active`，也不冒充用户会话；应由管理员列表服务通过 Repository 批量查询。
- 服务端应在同一读快照中生成用户行和活跃订单，避免“状态是充电中但设备为空”；
  实现时避免逐用户 N+1 查询。若读取期间订单刚完成，以最终一致的整行结果为准。
- 客户端校验每行确实符合本次手机号、账号状态和使用状态条件；排序后操作仍读取隐藏的真实
  `userId`，绝不使用视图行号。

## 冻结与解冻用户

请求 action：`admin.users.freeze`

```json
{
  "action": "admin.users.freeze",
  "requestId": "admin-11",
  "data": {
    "userId": 2,
    "expectedStatus": 0,
    "targetStatus": 1,
    "reason": "疑似异常充电行为，人工复核"
  }
}
```

- 同一个 action 同时承担冻结与解冻：`targetStatus=1` 冻结，`targetStatus=0` 解冻。
- `expectedStatus` 必须等于管理员刚看到的服务端状态；服务端在同一事务内比较并
  更新，不一致返回待冻结扩展码 `2104`，避免两个管理员相互覆盖。
- `reason` 去除首尾空白后为 2～200 字符，用于服务端审计；不得记录在普通日志中
  的密码、证件等敏感信息。
- 客户端根据列表中的 `activeOrder` 在确认框展示订单号、阶段、站点和桩编号，但
  该信息只用于说明风险，不能作为写入前最终判断。服务端必须在事务内重新查询。
- 对存在未完成订单的用户，客户端先显示独立风险弹窗，再进入原因填写；取消第一步
  不发送请求。服务端拒绝时必须显示其 `message`，并重新查询，绝不插入本地成功状态。
- 推荐的安全策略是：存在 `RESERVED / CHARGING / WAIT_SETTLEMENT` 时拒绝冻结并
  返回 `2003` 及明确 `message`，避免用户被冻结后无法停止或结算；该策略仍需成员 A
  在 Commit 6 权威文档中确认。客户端不绕过服务端结果。

成功响应：

```json
{
  "requestId": "admin-11",
  "code": 0,
  "message": "用户已冻结",
  "data": {
    "userId": 2,
    "previousStatus": 0,
    "status": 1,
    "changedAt": 1788516000000
  }
}
```

- 客户端不乐观修改当前行；成功或失败后都重新调用 `admin.users.list`，按钮文字
  由重新查询所得状态决定。
- 若冻结成功，服务端必须让该用户之后的所有受保护业务请求返回 `1002`，不能只在
  下一次登录时检查。现有连接如何失效由成员 A 冻结；公共无需登录的站点查询不能
  作为冻结验收操作。
- 当前成员 A 的 `UserService` 已在 `user.login` 检查冻结状态，但已登录连接上的
  `user.profile.update`、`user.recharge` 以及订单写操作尚未看到统一冻结校验；因此
  “已有会话跨端立即受限”仍是服务端 Commit 6 的待办，Mock 通过不代表真实服务端已完成。
- 跨端验收至少使用 `user.profile.update`、`user.recharge` 或订单写操作之一确认
  返回 `1002`；解冻后重新登录并验证同一操作恢复。用户端需将 `1002` 明确提示为
  “账号已冻结，请联系管理员”。
- Mock 对所有带 `activeOrder` 的未完成订单样例（包括 `WAIT_SETTLEMENT`）返回 `2003`，
  不得按固定用户 ID 或只按 `CHARGING` 特判。冻结弹窗会先解释订单阶段和当前设备，
  服务端拒绝后再展示真实 `message`。若 A 最终选择允许冻结，必须同时冻结活跃订单
  如何停止/结算及旧连接如何处理，客户端再按权威规则调整。

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
| 1002 | 用户账号被冻结（用户端受保护业务请求） |
| 1003 | 未登录或会话失效 |
| 1101 | 管理员账号或密码错误 |
| 2101 | 充电桩正在服务订单，拒绝运维操作 |
| 2102 | 站点版本冲突，旧资料不得覆盖新版本 |
| 2103 | 充电桩当前状态与 `expectedStatus` 不一致 |
| 2104 | 用户当前状态与 `expectedStatus` 不一致（待成员 A 冻结） |
| 5000 | 服务端内部错误 |
