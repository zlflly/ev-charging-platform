#ifndef STATION_SERVICE_H
#define STATION_SERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QTcpSocket>

namespace service {

/**
 * 充电站业务服务层
 * 处理 station.* 相关的 action
 */
class StationService : public QObject {
    Q_OBJECT

public:
    explicit StationService(QObject* parent = nullptr);

    // station.nearby: 附近站点查询（服务端计算距离并排序）
    QJsonObject handleNearby(const QJsonObject& data, QTcpSocket* socket);

    // station.detail: 站点详情+桩列表
    QJsonObject handleDetail(const QJsonObject& data, QTcpSocket* socket);

private:
    // 计算两点间距离（Haversine公式，单位：米）
    double calculateDistance(double lat1, double lon1, double lat2, double lon2) const;
};

} // namespace service

#endif // STATION_SERVICE_H
