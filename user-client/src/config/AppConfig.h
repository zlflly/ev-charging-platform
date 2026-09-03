#pragma once

#include <QtGlobal>

// 本地运行配置：密钥只从进程环境或 user-client/local.env 读取，源码不保存密钥。
namespace appConfig {

inline constexpr const char* kAmapWebServiceKeyEnvironment = "AMAP_WEB_SERVICE_KEY";
inline constexpr const char* kAmapGeocoderUrl = "https://restapi.amap.com/v3/geocode/geo";
inline constexpr const char* kAmapJsApiKeyEnvironment = "AMAP_JS_API_KEY";
inline constexpr const char* kAmapJsApiSecretEnvironment = "AMAP_JS_API_SECRET";

// WSL 联调环境通过宿主机代理访问高德服务；为空时可改为直连。
inline constexpr const char* kHttpProxyHost = "172.19.80.1";
inline constexpr quint16 kHttpProxyPort = 7890;

} // namespace appConfig
