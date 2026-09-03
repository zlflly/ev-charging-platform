# 电动汽车充电桩用户端：开发约定

## 项目定位

这是“东软电动汽车充电桩应用管理平台”的 **Qt 用户客户端**。面向电动车车主，覆盖：

`登录 → 定位找站 → 查看站点与充电桩 → 导航 → 预约/开始充电 → 停止/结算 → 个人中心`

界面语言使用简体中文；当前桌面端优先，整体视觉为深色座舱风格（深海蓝底色、电光蓝主操作、绿色成功、琥珀色注意、红色错误）。

项目位置：\\wsl.localhost\Ubuntu\home\zlflly\ev-charging-platform

## 技术栈与构建

- C++17、Qt 6 Widgets、CMake。
- 网络：`QTcpSocket` + 自定义 JSON 帧协议；页面不得直接操作 socket。
- 地图：`QWebEngineView` 承载地图/路线页面；地理编码与路线规划走独立模块。
- 构建目录应放在 `user-client/build/`，不要把生成文件提交到源码目录。

常用命令：

```bash
cd user-client
cmake -S . -B build
cmake --build build -j$(nproc)
./build/mock-server       # 本地联调时先启动
./build/user-client
```

## 目录职责

```text
user-client/
  src/net/          NetworkClient：唯一网络边界
  src/protocol/     协议 action、错误码、JSON 约定
  src/session/      Session：当前用户与定位的唯一状态来源
  src/model/        Station / Charger / Order 等 DTO
  src/geo/          地理编码与路线规划
  src/ui/           Qt Widgets 页面与可复用卡片
  src/ui/theme/     Theme.h：颜色、尺寸、全局 QSS 令牌
  resources/        qrc 打包的地图页面和图片素材
  tests/            Network / Geocoder / RoutePlanner 冒烟测试
```

## 业务与状态原则

1. **服务端是业务状态的唯一真相。** 前端不能因点击按钮就本地假设预约、开始、停止或结算成功。
2. **Session 是用户状态唯一来源。** 用户 id、手机号、昵称、头像、余额、当前定位必须通过 `Session` 读写；更新后发出统一变更信号。
3. **页面只通过 `NetworkClient::sendRequest(...)` 调用服务端。** 禁止在 UI 页中自行 new socket、拼帧或解析底层网络缓冲。
4. **订单按状态机渲染。** 合法流转：`RESERVED → CHARGING → WAIT_SETTLEMENT → FINISHED`。进入充电页必须先检查未完成订单；余额不足、桩不可用、状态冲突和断网必须保留可恢复路径。
5. **任何写操作以服务端响应为准。** 昵称、头像、充值、预约、开始、停止、结算成功后才更新 UI/Session；请求在途时禁用重复提交。
6. **不调用未经协议定义的接口。** 例如服务端未提供订单历史接口时，订单页面只展示 `order.active` 的真实状态和清晰空态，不伪造历史订单数据。

## UI 与资源规范

- 所有主题颜色、全局控件风格、核心尺寸优先来自 `src/ui/theme/Theme.h`；避免在各页面散落十六进制颜色。
- 使用卡片、细描边、圆角与电光蓝主按钮；状态必须同时使用颜色和文字。
- 登录页是独立沉浸式首屏：登录态下隐藏业务侧栏，背景资源通过 Qt Resource System（`:/resources/...`）引用。
- 图片必须放入 `resources/` 并加入 `qt_add_resources`；不可引用本机绝对路径。
- 保持桌面布局可读，避免让表单遮挡主视觉或让长文本溢出；窗口窄时必须保留基本可操作性。
- UI 文案使用产品语言，例如“登录 / 自动注册”“附近充电站”“停止充电”；避免技术错误码直接暴露给用户。

## 修改与验证要求

- 修改 C++/CMake/资源清单后至少运行一次 `cmake --build build`。
- 修改网络行为时，运行对应 smoke test；修改订单流时检查重复点击、断网、余额不足和状态冲突。
- 修改 UI 后检查登录、主页、站点详情、充电页、订单页和个人中心至少一遍。
- 保留用户已有的未提交修改；不要使用 `git reset --hard`、`git checkout --` 等会覆盖工作区的命令。
- 新增第三方依赖前先确认必要性；若确需安装，优先安装到 D 盘，避免无故占用 C 盘。

## 不应做的事

- 不直接修改数据库，不绕过服务端写余额、订单或桩状态。
- 不把 API Key、账号、机器本地绝对路径写入源码或提交到仓库。
- 不用 emoji 或不一致的 Unicode 字符替代正式图标；使用 Qt 绘制或受控 SVG/资源。
- 不删除或重写与当前任务无关的设计、地图、协议或文档文件。
