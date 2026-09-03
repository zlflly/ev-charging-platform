# EV Charging Platform - Admin Client

Linux + Qt 6 管理员桌面客户端。当前 Commit 0 提供：

- 宽屏后台主框架与统一导航；
- 单例式共享 `NetworkClient`，采用 4 字节大端长度前缀 + UTF-8 JSON；
- `requestId` 并发响应匹配、10 秒超时、断线重连与载荷上限；
- 可复用表格加载、空、错误状态以及稳定实体 ID 映射；
- PING mock server 与网络冒烟测试。

管理员业务 action、字段和错误码仍需成员 1 冻结，当前没有伪造业务数据。

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
