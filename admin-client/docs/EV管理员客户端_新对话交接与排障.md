# EV 管理员客户端：交接、成功流程与排障

更新日期：2026-09-04

## 1. 项目边界

- 技术栈：Linux、C++17、Qt 6、CMake、TCP Socket、JSON。
- 固定分支：`qwq-admin-client`。
- 成员 C 只负责管理员 Qt 客户端，不写 SQL、不直接访问数据库、不实现用户端、
  不决定订单状态机、不实现机器学习。
- 所有业务数据和写操作都必须经过成员 A 的服务端接口。
- `admin-mock-server` 只验证客户端协议和交互，不能当成真实数据库。

## 2. 已完成节点

| 节点 | Commit | 内容 |
|---|---|---|
| Commit 0 | `ff2590a` | 工程骨架、共享网络层、统一导航和表格状态 |
| Commit 1 | `f915776` | 管理员登录、认证和会话 |
| Commit 2 | `cb1fbbf` | 充电桩状态总览 |
| Commit 3 | `16a9bb9` | 充电桩列表、筛选和远程重启 |

Commit 4 在提交前应完整包含：

- 站点列表、搜索、在线率与稳定 `stationId`；
- 站内桩详情和空站点状态；
- 新增站点与初始桩，失败不插入本地假行；
- 编辑站名、地址、经纬度，使用 `version` 防止并发覆盖；
- 站内设备远程重启；
- 空闲、故障、离线之间的受控状态调整；
- 在用状态只由订单流程产生，活动订单设备禁止强制修改；
- 协议、Mock、自动测试、README 和设计文档同步更新。

协议的唯一书面来源是 [admin-protocol.md](admin-protocol.md)。

## 3. 固定开发流程

```text
确认需求和协议
→ 核对分支与基线
→ 应用补丁
→ 编译
→ 自动测试
→ 界面和失败路径验收
→ status/diff 检查
→ 精确 git add
→ commit
→ 检查远程连接
→ push
→ 再核对本地与远端
```

禁止使用：

- `git add .`
- `git push --force`
- `git reset --hard`
- 在其他成员分支提交客户端代码

## 4. 补丁传入 Ubuntu 的成功经验

VMware 拖拽文件通常先落在：

```text
/home/bit/.cache/vmware/drag_and_drop/<随机目录>/
```

不要假定浏览器或拖拽文件一定在 `~/Downloads`。先定位：

```bash
find /home/bit -maxdepth 5 -type f -iname '*.patch' -print 2>/dev/null
```

找到后复制到稳定路径，例如 `/home/bit/commit4-station-management.patch`，再按顺序：

```bash
cd ~/projects/ev-charging-platform
git apply --check /home/bit/commit4-station-management.patch
git apply /home/bit/commit4-station-management.patch
```

`git apply --check` 没有输出表示通过。应用后不要再次应用同一补丁。

## 5. 编译与测试

```bash
cd ~/projects/ev-charging-platform

cmake -S admin-client \
      -B admin-client/build \
      -DCMAKE_BUILD_TYPE=Debug

cmake --build admin-client/build -j2
```

自动测试统一使用同一个 Mock：

```bash
fuser -k 9000/tcp 2>/dev/null || true

(
    ./admin-client/build/admin-mock-server \
        >/tmp/admin-smoke.log 2>&1 &
    admin_mock_pid=$!
    trap 'kill "$admin_mock_pid" 2>/dev/null || true; wait "$admin_mock_pid" 2>/dev/null || true' EXIT
    sleep 1

    ./admin-client/build/admin-network-smoke-test &&
    ./admin-client/build/admin-auth-smoke-test &&
    ./admin-client/build/admin-charger-overview-smoke-test &&
    ./admin-client/build/admin-charger-management-smoke-test &&
    ./admin-client/build/admin-station-management-smoke-test &&
    ./admin-client/build/admin-station-operations-smoke-test
)
```

六项都出现 `PASSED` 才进入界面验收。

Mock 登录账号：`admin / 123456`。账号只存在于 Mock/数据库种子，不由客户端
写死判断。

## 6. Commit 4 人工验收

1. 站点列表字段完整，搜索和排序后仍操作正确站点。
2. 选择“亦庄待投运站”，无桩详情显示空状态且不崩溃。
3. 新增非法站点被拒绝，列表不出现假行。
4. 新增合法站点后重新查询，初始桩立即出现在详情。
5. 编辑站名/地址/经纬度后重新查询仍存在；总桩数和在线率不可编辑。
6. 选择空闲桩，填写原因并改为离线；详情和站点在线率都随服务端结果刷新。
7. 在用桩点击“调整状态”只显示订单保护说明，不进入表单、不发送请求；服务端
   测试也必须拒绝绕过客户端的请求。
8. 站内远程重启显示目标 ID 和状态；在用桩由服务端拒绝。
9. 关闭再进入页面或显式刷新后，显示结果仍来自服务端而非本地缓存。

## 7. 常见问题

### Mock 端口被占用

错误：`failed to listen on 127.0.0.1:9000`

```bash
fuser -k 9000/tcp 2>/dev/null || true
ss -lntp | grep 9000
```

不要同时启动多个 Mock。

### 连接被拒绝

通常是 Mock/真实服务端未运行，或客户端连接了错误地址和端口。先检查：

```bash
ss -lntp | grep 9000
```

### Wayland 警告

```text
Warning: Ignoring WAYLAND_DISPLAY on Gnome...
```

当前 Qt 程序通过 X11 兼容层正常显示时可以忽略，它不是业务失败。

### 编译失败只看到 `gmake: Error`

应回到输出中的第一处 `error:`，提供其上下约 10～20 行。最后一行通常只是连锁结果。

### 客户端和服务端显示不一致

依次检查：响应字段是否符合协议、操作是否使用真实 ID、成功后是否重新查询、旧响应
是否覆盖新响应、服务端事务与订单状态映射是否正确。不要在 UI 层手工补数据。

### `git diff --stat` 看不到新文件

未跟踪文件不进入普通 diff。精确暂存后检查：

```bash
git diff --cached --name-status
git diff --cached --stat
```

## 8. GitHub 与 Clash 网络排障

此前成功链路：Ubuntu 虚拟机经 Windows Clash Mixed Port `7897` 访问 GitHub。
Windows WLAN IPv4 会在换网后变化，不能永久照抄旧地址。

Windows 端需确认 Clash 正在运行、允许局域网、规则可访问 GitHub。Ubuntu 先测试：

```bash
timeout 3 bash -c 'echo >/dev/tcp/<Windows当前IPv4>/7897' && \
echo '[PASS] Clash 端口可访问' || echo '[FAIL] Clash 端口不可访问'
```

只为当前仓库配置代理：

```bash
git config --local http.proxy http://<Windows当前IPv4>:7897
git config --local https.proxy http://<Windows当前IPv4>:7897
git config --local http.version HTTP/1.1
```

推送前先验证远程：

```bash
timeout 45 git ls-remote --heads origin qwq-admin-client
```

若 push 遇到 TLS/代理中断，不要重新 commit。先执行 `git log` 和 `git ls-remote`
确认本地提交与远端位置，再重试同一条 push。HTTPS 认证使用 GitHub 用户名和
Personal Access Token，不使用登录密码。

## 9. 提交前检查

```bash
git branch --show-current
git status --short --branch
git diff --check
git diff --stat
```

暂存时列出本次文件，不使用 `git add .`。提交后再检查：

```bash
git status --short --branch
git log -3 --oneline --decorate
```
