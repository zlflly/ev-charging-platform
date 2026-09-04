#pragma once

#include <QtGlobal>

namespace config {

inline constexpr const char* kDefaultServerHost = "127.0.0.1";
// 与成员 A 服务端 ServerConfig 的默认监听端口保持一致。
inline constexpr quint16 kDefaultServerPort = 8888;

} // namespace config
