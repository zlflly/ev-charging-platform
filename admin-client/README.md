# EV Charging Platform - Admin Client

Linux + Qt 6 管理员桌面客户端。当前已完成 Commit 3：

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

## Structure

```text
src/app/ApplicationController    管理依赖和登录/主窗口切换
src/api/AdminApiClient           登录及后续认证请求的统一入口
src/model/ChargerStatusOverview  聚合响应 DTO 与一致性校验
src/model/Charger                桩列表 DTO、字段校验和状态/类型显示映射
src/session/AdminSession         管理员登录态唯一来源
src/net/NetworkClient            TCP framing、超时和 requestId 匹配
src/ui/LoginWindow               登录表单和交互状态
src/ui/MainWindow                认证后的运营后台
src/ui/ChargerManagementPage     列表、选择、二次确认和重启反馈
src/ui/widgets/ChargerStatusOverviewWidget  状态卡片、进度条与刷新反馈
```
