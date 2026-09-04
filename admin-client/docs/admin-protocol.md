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
| 5000 | 服务端内部错误 |
