# EV Charging Platform

东软电动汽车充电桩应用管理平台小学期项目。

C++17 + Qt 6 + SQLite。服务端通过 TCP + JSON 帧协议对外提供全部业务能力，客户端只经协议读写业务状态。

## 仓库结构

| 路径 | 说明 | 负责人 |
|---|---|---|
| `server/` | 服务端：TCP + JSON 服务、业务规则、SQLite 数据层，见 `server/README.md` | 成员 1 |
| `user-client/` | 用户 Qt 客户端：定位找站、导航、预约/充电/结算、个人中心 | 成员 2 |
| `docs/` | `协议冻结说明.md`（协议唯一权威来源）与 `对接说明.md`（接口清单、错误码、测试账号） | 成员 1 |

用户端开发约定见 `AGENTS.md`，视觉与产品说明见 `DESIGN.md` / `PRODUCT.md`。

## 团队分支

- `fengshu-server-db` — A：服务端核心、数据库与总体集成
- `zlflly-user-client` — B：用户 Qt 客户端与充电业务流程
- `qwq-admin-client` — C：管理员 Qt 客户端与运营管理流程
- `ran-dashboard-ml` — D：数据可视化大屏与机器学习分析

开发完成后通过 Pull Request 合并到 `main`。

---

# 用户客户端（user-client）

## 环境准备

目标环境是 Linux + Qt 6；当前联调环境为 Ubuntu 24.04 + Qt 6.4.2 + CMake 3.28。

```bash
sudo apt install cmake build-essential qt6-base-dev qt6-webengine-dev \
                 fonts-noto-cjk libgl1-mesa-dri
```

Windows 下通过 WSL2（Ubuntu + WSLg）运行。客户端是 GUI 程序，必须在有 WSLg 或 X 服务的会话中启动。

## 快速开始

```bash
# 1. 先起服务端（构建与参数见 server/README.md）
./server/build/ev-server --database ./ev.db

# 2. 准备本地密钥（local.env 不进仓库）
cd user-client
cp local.env.example local.env      # 填入自己的高德 key

# 3. 一键构建并启动
./run.sh
```

客户端默认连接 `127.0.0.1:9000`，常量在 `user-client/src/protocol/Protocol.h`，服务端不在本机时改这里。
登录页输入手机号即“登录 / 自动注册”，预置账号与测试站点坐标见 `docs/对接说明.md`。

## run.sh：一键构建并启动

`user-client/run.sh` 把“配置 CMake → 增量构建 → 注入联调环境变量 → 启动”封装成一条命令，
需要在 `user-client/` 目录下执行。

| 选项 | 说明 |
|---|---|
| `-p, --preview PAGE` | 界面预览模式，直达某个页面并注入本地登录态（不依赖服务端）。可选：`home` `station` `charger` `navigation` `charging` `profile` |
| `-n, --no-build` | 跳过构建，直接运行已有产物 |
| `-c, --clean` | 删除 `build/` 后重新配置并全量构建 |
| `-m, --map-debug` | 打开 QtWebEngine / 地图诊断日志 |
| `-l, --log FILE` | 运行日志同时写入文件 |
| `-h, --help` | 显示帮助 |
| `-- <args>` | `--` 之后的参数原样传给可执行文件 |

```bash
cd user-client
./run.sh                        # 正常启动，先起服务端
./run.sh -p home                # 只看首页视觉，不连服务端登录
./run.sh -c                     # 改过 CMakeLists 或资源清单后全量重建
./run.sh -m -l /tmp/uc.log      # 排查地图问题，保留完整日志
./run.sh -n                     # 只是重启界面，不重新编译
```

预览模式只是注入本地登录态与定位来单独检查页面视觉，**不代表真实业务状态**；
预约、开始、停止、结算等写操作仍然必须走服务端联调验证。

等价的手动命令：

```bash
cd user-client
cmake -S . -B build
cmake --build build -j$(nproc)
./build/user-client
```

## 目录职责

| 目录 | 职责 |
|---|---|
| `src/net` | `NetworkClient`：唯一网络边界，页面不接触 socket |
| `src/protocol` | 协议 action、错误码、默认服务端地址 |
| `src/session` | `Session`：当前用户与定位的唯一状态来源 |
| `src/model` | `StationInfo` / `ChargerInfo` / `OrderInfo` 等 DTO |
| `src/geo` | 高德地理编码与路线规划 |
| `src/ui` | Qt Widgets 页面、可复用卡片、`src/ui/theme/Theme.h` 主题令牌 |
| `resources` | qrc 打包的地图页面与图片素材 |

## local.env：高德密钥

密钥只从进程环境或 `user-client/local.env` 读取，源码与仓库里不保存密钥（`local.env` 已在 `.gitignore`）。

| 变量 | 用途 |
|---|---|
| `AMAP_JS_API_KEY` | 首页/导航页在线地图（高德 JS API 2.0） |
| `AMAP_JS_API_SECRET` | JS API 安全密钥 |
| `AMAP_WEB_SERVICE_KEY` | 地理编码与路线规划（Web 服务 API） |

## 地图行为与排查

在线地图不可用时客户端不会留一片空白，而是自动切换到本地绘制的简易地图，并在卡片上写明原因。
触发回退的情况：没有 `AMAP_JS_API_KEY`、高德 JS API 未加载、当前环境没有可用 WebGL、
渲染中 WebGL 上下文丢失、或 12 秒内没有收到地图就绪信号。

WSL / 虚拟机里的 OpenGL 驱动会被 Chromium 判定为不可信并把 WebGL 拉黑，而高德 JS API 2.0 只有
WebGL 一条渲染路径——这正是“地图闪一下就消失”的原因。客户端启动时会自动追加
`--ignore-gpu-blocklist`（已显式设置 `QTWEBENGINE_CHROMIUM_FLAGS` 时不覆盖）。

地图页面里的 `console.error` 会转发到 Qt 日志，前缀为 `[map]`，key 无效、域名白名单未配置、
网络不通都能直接看到。`./run.sh -m` 会额外打开 QtWebEngine 诊断日志。

WSL 里访问高德服务默认走宿主机 HTTP 代理（`user-client/src/config/AppConfig.h` 中的
`kHttpProxyHost` / `kHttpProxyPort`）。直连环境把 `kHttpProxyHost` 置空即可。

## 调试开关

| 环境变量 | 说明 |
|---|---|
| `EV_PREVIEW_AUTH` | 注入预览登录态（`run.sh -p` 会自动设置） |
| `EV_PREVIEW_PAGE` | 启动后直达的页面名 |
| `EV_CAPTURE_PATH` | 启动 500ms 后截图到该路径并退出，用于留存界面对比图 |
| `QT_LOGGING_RULES` | Qt 日志过滤，如 `qt.webenginecontext.debug=true` |

## 联调自测清单

改完客户端代码至少走一遍：

- `cd user-client && ./run.sh` 能构建并启动（脚本内已包含 `cmake --build build`）。
- 页面巡检：登录、首页附近站点、站点详情、充电桩、导航、充电页、订单页、个人中心，
  可用 `./run.sh -p <page>` 逐页快速过一遍。
- 订单状态机：`RESERVED → CHARGING → WAIT_SETTLEMENT → FINISHED`，
  并覆盖重复点击、断网重连、余额不足、桩不可用、状态冲突这几条异常路径。
- 地图：有 key 时确认在线地图渲染，去掉 key 后确认回退到简易地图且文案正确。

## 当前进度

登录、附近充电站、站点与充电桩详情、导航、充电与结算流程、个人中心已打通；
在线地图带离线回退；充电中按服务端 `order.status` 轮询刷新电量与金额。
