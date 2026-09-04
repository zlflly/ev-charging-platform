#pragma once

#include <QtGlobal>

// 本地运行配置：密钥只从进程环境或 user-client/local.env 读取，源码不保存密钥。
namespace appConfig {

// 与 mock server 的初始站点数据保持一致；主页、站点预览和导航预览共用这组坐标。
inline constexpr double kDefaultLocationLatitude = 39.735678;
inline constexpr double kDefaultLocationLongitude = 116.171271;
inline constexpr const char* kDefaultLocationLabel = "北京理工大学良乡校区";
inline constexpr double kPreviewStationLatitude = 39.7328;
inline constexpr double kPreviewStationLongitude = 116.1735;
inline constexpr const char* kPreviewStationName = "北理良乡南门充电站";

inline constexpr const char* kAmapWebServiceKeyEnvironment = "AMAP_WEB_SERVICE_KEY";
inline constexpr const char* kAmapGeocoderUrl = "https://restapi.amap.com/v3/geocode/geo";
inline constexpr const char* kAmapDrivingUrl = "https://restapi.amap.com/v3/direction/driving";
inline constexpr const char* kAmapWalkingUrl = "https://restapi.amap.com/v3/direction/walking";
inline constexpr const char* kAmapJsApiKeyEnvironment = "AMAP_JS_API_KEY";
inline constexpr const char* kAmapJsApiSecretEnvironment = "AMAP_JS_API_SECRET";

// WSL 联调环境通过宿主机代理访问高德服务；为空时可改为直连。
inline constexpr const char* kHttpProxyHost = "172.19.80.1";
inline constexpr quint16 kHttpProxyPort = 7890;

} // namespace appConfig
