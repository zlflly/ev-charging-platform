# ev-server（成员 1：服务端与数据库）

东软电动汽车充电桩应用管理平台的服务端。Linux + Qt6（Core / Network / Sql），
对用户端、管理员端提供统一的 TCP + JSON 服务，SQLite 为唯一数据事实来源。

协议约定见仓库根目录 `docs/协议冻结说明.md`——那是全队唯一权威来源，本文只讲怎么跑起来。

## 目录分层

| 目录 | 职责 |
|---|---|
| `src/config` | 启动配置：监听地址、端口、数据库路径 |
| `src/net` | 网络层：framing 编解码、TCP 监听、连接生命周期、请求路由 |
| `src/protocol` | 协议层：消息外壳、错误码、枚举、action 常量 |
| `src/service` | 业务层：各 action 的处理逻辑与业务规则校验（Commit 1 起） |
| `src/repository` | 数据层：SQLite 访问封装（建表、CRUD、事务）（Commit 1 起） |
| `tests` | 单元测试 |
| `tools` | 联调辅助工具（冒烟客户端等） |

## 构建

Ubuntu 22.04：

```bash
sudo apt install qt6-base-dev cmake build-essential
cmake -S server -B server/build
cmake --build server/build -j$(nproc)
```

产物：`ev-server`（服务端）、`FrameCodecTest`（单元测试）、`ev-smoke-test`（冒烟客户端）。

Windows/CLion 下的 Qt6 路径配置见 `CLION_SETUP.md`。

## 运行

```bash
./server/build/ev-server --host 0.0.0.0 --port 9000 --database ./ev.db
```

可用参数：`--host` / `--port` / `--database` / `--max-connections` / `--threads` / `--verbose`，
`--help` 可查看完整说明。Ctrl+C 优雅退出。

## 验收（Commit 0）

```bash
# 单元测试：framing 的粘包 / 半包 / 超长包 / 空包 / 连续 100 帧
./server/build/FrameCodecTest

# 冒烟测试：一次性灌 100 个 PING，校验 requestId 全部匹配且不丢帧
./server/build/ev-server &
./server/build/ev-smoke-test 127.0.0.1 9000
```

冒烟通过时输出 `Test PASSED`，任何一个 requestId 对不上或响应缺失都会以非零码退出。

## 当前进度

Commit 0 已完成：工程骨架、启动配置、协议定义、framing 编解码与单元测试、
TCP 监听与连接生命周期、请求路由与 `PING`、冒烟测试工具。

Commit 1 起接入 SQLite 与业务 action（用户登录、站点、订单、结算），
Commit 6 补齐管理员端 action，Commit 7 补齐机器学习数据接口。
