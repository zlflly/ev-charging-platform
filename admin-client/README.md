# EV Charging Platform - Admin Client

Linux + Qt 6 管理员桌面客户端。当前已完成 Commit 6：

- 宽屏后台主框架与统一导航；
- 单例式共享 `NetworkClient`，采用 4 字节大端长度前缀 + UTF-8 JSON；
- `requestId` 并发响应匹配、10 秒超时、断线重连与载荷上限；
- 可复用表格加载、空、错误状态以及稳定实体 ID 映射；
- PING mock server 与网络冒烟测试。
- 管理员账号/密码登录、本地非空校验和重复提交保护；
- 集中的 `AdminSession` 与应用级登录/主窗口切换；
- TCP 连接绑定会话、断线失效和统一重新登录；
- 管理员认证 mock 与成功/失败冒烟测试。
- 登录页使用项目专属电动汽车充电科技插画，并通过 Qt 资源系统打包。
- 充电桩状态聚合总览，展示空闲、在用、故障、离线数量与服务端百分比；
- 登录后自动加载和显式刷新，刷新期间阻止重复并发请求；
- 统一校验总数、状态数量、百分比和零数据，网络/数据异常均有可重试反馈。
- 充电桩运维列表，展示所属站点、类型、功率、状态和累计统计字段；
- 支持编号/站名关键词、所属电站、快慢充类型和运行状态组合筛选；
- 状态使用语义色标签，故障设备以红色行背景和“需处理”醒目标记；
- 顶部故障待处理入口显示实时故障数，并可一键切换到故障设备视图；
- 所有可排序列使用独立排序值，功率、次数和时长按数值而非显示文字排序；
- 表格排序后仍通过隐藏的真实 `chargerId` 执行操作；
- 统一风格的远程重启二次确认与结果反馈、在用设备安全拒绝和成功后重新查询。
- 支持空闲、故障、离线之间的受控运维状态调整；在用状态只由订单流程驱动，
  状态变更要求原因和 `expectedStatus` 并由服务端最终校验；
- 选择在用设备时仍可点击“调整状态”查看保护原因，但客户端不会打开编辑表单，
  也不会发送状态变更请求；
- 充电站列表展示站点 ID、站名、地址、经纬度、站点级电价、总桩数和服务端在线率；
- 支持按站点 ID、站名和地址搜索，排序后仍保留真实 `stationId`；
- 选中站点后复用 `station.detail` 按 ID 查询站内桩，空站点显示明确空状态；
- 新增站点表单校验站名、地址、经纬度、站点级电价和 0～100 台初始桩；
- 新增操作等待服务端确认，成功后重查列表与详情，失败不会插入本地假行。
- 支持编辑站名、地址、经纬度和站点级电价，使用服务端 `version` 乐观锁避免并发覆盖；
- 站内设备可直接执行受控状态调整和远程重启，成功后同步刷新详情和在线率；
- 不允许编辑派生的总桩数/在线率，也不提供语义未冻结的级联删除。
- 用户列表采用服务端分页，展示稳定 userId、手机号、昵称、两位小数余额、注册时间
  和明确状态标签；支持手机号模糊查询与正常/冻结状态筛选；
- 搜索只在点击、回车或翻页时发送，空关键字返回全部，无结果显示明确空状态；
- 冻结/解冻携带 `expectedStatus` 和必填原因，服务端决定活跃订单规则；客户端不
  乐观改行，响应后重查并以最新状态更新按钮。
- 账号状态与业务状态分列展示；未完成订单显示已预约/充电中/待支付，以及关联
  站点、充电桩和订单号，并支持服务端“有未完成订单/当前无订单”筛选；
- `WAIT_SETTLEMENT` 与用户端统一显示为“待支付”：充电已停止、账单已生成，但
  扣款与订单完结尚未成功；它仍属于服务端定义的未完成订单；
- 冻结未完成订单用户采用“风险说明 → 原因填写”两阶段确认，服务端拒绝原因使用
  高对比结果弹窗展示；服务端在写入时仍须复查，客户端不绕过订单保护；
- Mock 对已预约、充电中和待支付三类未完成订单统一拒绝冻结，测试同时覆盖充电中
  与待支付，避免按样例用户 ID 写死业务规则；
- 全局视觉改为浅灰背景、白色圆角卡片、深蓝正文和蓝色主按钮，和用户端保持同一
  产品语言，同时保留管理端侧栏、表格和信息密度。
- 运营总览已改为独立聚合页面：复用营收、站点、设备和用户接口，展示今日/本月/
  累计营收、站点数、桩总数、平台用户数、活跃业务人数及前三条业务/设备摘要；
- 首页直接复用 `admin.revenue.trend` 绘制最近 7 个自然日折线，不再显示占位说明；
- “运营关注”从完整 `admin.chargers.list` 响应筛出故障设备，展示设备编号与所属站点；
  该列表只读，不在首页确认或修改故障状态；
- 右侧运营关注使用适配窄卡片的双行摘要列表，完整信息可通过换行、纵向滚动和悬停
  提示查看，不再使用会在小宽度下挤压遮挡的三列表头；
- 营收汇总复用成员 A 已固定的 `admin.revenue.summary`，展示今日、本月和累计
  已结算营收；
- 7/30 日趋势复用 `admin.revenue.trend`，严格校验日桶数量、连续日期、金额、币种
  与时区，缺失日期由服务端显式返回零值；
- 浅色自适应折线图支持全零、单日收入、大额缩放和悬停精确金额，不额外要求
  `Qt6::Charts` 系统组件；
- 周期切换允许并发请求，并以 generation 丢弃迟到旧响应；加载失败会清空旧曲线，
  不把缓存数据伪装成最新统计；
- `WAIT_SETTLEMENT`（待支付）不计营收，只有 `FINISHED` 订单按 `settleTime` 计入；
  字段尚未被成员 A 冻结的部分已在正式协议中明确标注为联调契约。

管理员协议见 `docs/admin-protocol.md`。默认 `admin/123456` 仅由数据库或 mock
server 提供，客户端不在本地判断账号密码。
管理员端架构与目录职责见 `DESIGN.md`。

## Build

```bash
cmake -S . -B build
cmake --build build -j
./build/admin-client
```

## Network smoke test

打开两个终端：

```bash
./build/admin-mock-server
```

```bash
./build/admin-network-smoke-test
```

看到 `SMOKE TEST PASSED` 表示基础网络封装通过本地验证。

## Administrator authentication smoke test

保持 mock server 运行，在另一终端执行：

```bash
./build/admin-auth-smoke-test
```

测试会先验证错误密码被拒绝，再验证种子账号登录成功；看到
`AUTH SMOKE TEST PASSED` 表示认证协议闭环通过。

## Charger status overview smoke test

保持 mock server 运行，在另一终端执行：

```bash
./build/admin-charger-overview-smoke-test
```

测试覆盖管理员登录、聚合响应、零数据、数量不一致拒绝和重复刷新保护；看到
`CHARGER OVERVIEW SMOKE TEST PASSED` 表示 Commit 2 的数据闭环通过。

## Charger management smoke test

保持 mock server 运行，在另一终端执行：

```bash
./build/admin-charger-management-smoke-test
```

测试覆盖列表解析、组合筛选规则、重复加载保护、在用桩重启拒绝、允许状态重启，
以及重复操作保护；看到 `CHARGER MANAGEMENT SMOKE TEST PASSED` 表示 Commit 3
闭环通过。

## Station management smoke test

保持 mock server 运行，在另一终端执行：

```bash
./build/admin-station-management-smoke-test
```

测试覆盖站点列表与字段解析、空站点详情、搜索规则、非法新增不污染列表、重复提交
保护，以及新增成功后重新查询列表和初始桩详情；看到
`STATION MANAGEMENT SMOKE TEST PASSED` 表示 Commit 4 闭环通过。

## Station operations smoke test

保持 mock server 运行，在另一终端执行：

```bash
./build/admin-station-operations-smoke-test
```

测试覆盖站点版本化编辑、旧版本写入拒绝、设备状态修改、旧状态写入拒绝、在用设备
保护，以及状态变化后站点在线率同步；看到 `STATION OPERATIONS SMOKE TEST PASSED`
表示增强运维链路通过。

## User management smoke test

保持 mock server 运行，在另一终端执行：

```bash
./build/admin-user-management-smoke-test
```

测试覆盖分页元数据、服务端手机号/使用状态筛选、活跃订单及设备字段、空结果、
冻结后重查、旧状态并发拒绝、解冻，以及活跃订单拒绝原因透传；看到
`USER MANAGEMENT SMOKE TEST PASSED` 表示 Commit 5 客户端闭环通过。

## Revenue statistics smoke test

保持 mock server 运行，在另一终端执行：

```bash
./build/admin-revenue-smoke-test
```

测试覆盖汇总包含关系、金额/时间字段校验、7/30 个连续日期桶、缺失桶拒绝、汇总
重复请求保护，以及 7/30 日乱序响应仍按各自 requestId 正确匹配；看到
`REVENUE SMOKE TEST PASSED` 表示 Commit 6 客户端闭环通过。

## Operations overview smoke test

保持 mock server 运行，在另一终端执行：

```bash
./build/admin-operations-overview-smoke-test
```

测试会在同一轮首页刷新中并发请求营收汇总、7 日趋势、桩状态、站点和设备列表，
再串行请求用户总数与活跃业务摘要；看到 `OPERATIONS OVERVIEW SMOKE TEST PASSED`
表示首页所有数据来源均可完成协议解析。

## Structure

```text
src/app/ApplicationController    管理依赖和登录/主窗口切换
src/api/AdminApiClient           登录及后续认证请求的统一入口
src/model/ChargerStatusOverview  聚合响应 DTO 与一致性校验
src/model/Charger                桩列表 DTO、字段校验和状态/类型显示映射
src/model/Station                站点、详情、新增和版本化编辑请求的协议校验
src/model/User                   用户分页、金额时间与冻结状态协议校验
src/model/Revenue                营收汇总、日趋势与统计口径一致性校验
src/session/AdminSession         管理员登录态唯一来源
src/net/NetworkClient            TCP framing、超时和 requestId 匹配
src/ui/LoginWindow               登录表单和交互状态
src/ui/MainWindow                认证后的运营后台
src/ui/OperationsOverviewPage    多接口聚合 KPI、7 日趋势与运营关注
src/ui/ChargerManagementPage     列表、选择、二次确认和重启反馈
src/ui/StationManagementPage     站点列表、详情、新增、编辑与站内设备运维
src/ui/UserManagementPage        服务端搜索、分页和冻结/解冻风险控制
src/ui/RevenueStatisticsPage     已结算 KPI、7/30 日切换和迟到响应保护
src/ui/widgets/ChargerStatusOverviewWidget  状态卡片、进度条与刷新反馈
src/ui/widgets/RevenueTrendChart            自适应浅色折线图和悬停金额
```
