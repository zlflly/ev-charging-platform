#include "StationService.h"
#include "repository/StationRepository.h"
#include "repository/ChargerRepository.h"
#include "protocol/ProtocolHelper.h"
#include <QJsonArray>
#include <QDebug>
#include <QtMath>

namespace service {

StationService::StationService(QObject* parent)
    : QObject(parent)
{
}

QJsonObject StationService::handleNearby(const QJsonObject& data, QTcpSocket* socket) {
    Q_UNUSED(socket)

    // 提取用户经纬度
    double userLat = data.value("latitude").toDouble();
    double userLon = data.value("longitude").toDouble();
    int radius = data.value("radius").toInt(5000);  // 默认5000米

    // 校验参数
    if (userLat < -90 || userLat > 90 || userLon < -180 || userLon > 180) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "经纬度参数非法");
    }

    if (radius < 100 || radius > 50000) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "搜索半径应在100-50000米之间");
    }

    // 查询所有活跃站点
    QVector<repository::Station> stations = repository::StationRepository::findActive();

    // 计算距离并过滤
    struct StationWithDistance {
        repository::Station station;
        double distance;
        int availableCount;
        int totalCount;
    };

    QVector<StationWithDistance> candidates;

    for (const auto& station : stations) {
        double distance = calculateDistance(userLat, userLon, station.latitude, station.longitude);

        if (distance > radius) {
            continue;  // 超出半径，跳过
        }

        // 查询站点内的充电桩
        QVector<repository::Charger> chargers = repository::ChargerRepository::findByStationId(station.stationId);

        int totalCount = chargers.size();
        int availableCount = 0;

        for (const auto& charger : chargers) {
            if (charger.status == protocol::ChargerStatusIdle) {
                availableCount++;
            }
        }

        StationWithDistance swd;
        swd.station = station;
        swd.distance = distance;
        swd.availableCount = availableCount;
        swd.totalCount = totalCount;
        candidates.append(swd);
    }

    // 按距离排序（从近到远）
    std::sort(candidates.begin(), candidates.end(), [](const StationWithDistance& a, const StationWithDistance& b) {
        return a.distance < b.distance;
    });

    // 构造响应
    QJsonArray stationsArray;
    for (const auto& swd : candidates) {
        QJsonObject stationObj;
        stationObj["stationId"] = swd.station.stationId;
        stationObj["name"] = swd.station.name;
        stationObj["address"] = swd.station.address;
        stationObj["latitude"] = swd.station.latitude;
        stationObj["longitude"] = swd.station.longitude;
        stationObj["distance"] = qRound(swd.distance);  // 四舍五入到整数米
        stationObj["availableCount"] = swd.availableCount;
        stationObj["totalCount"] = swd.totalCount;
        stationsArray.append(stationObj);
    }

    QJsonObject result;
    result["stations"] = stationsArray;

    qInfo() << "[StationService] Nearby query: userPos=(" << userLat << "," << userLon
            << ") radius=" << radius << "found=" << stationsArray.size() << "stations";

    return protocol::makeSuccessResponse(result);
}

QJsonObject StationService::handleDetail(const QJsonObject& data, QTcpSocket* socket) {
    Q_UNUSED(socket)

    // 提取站点ID
    int stationId = data.value("stationId").toInt(0);

    if (stationId <= 0) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "站点ID非法");
    }

    // 查询站点信息
    auto stationOpt = repository::StationRepository::findById(stationId);
    if (!stationOpt.has_value()) {
        return protocol::makeErrorResponse(protocol::CodeBadRequest, "站点不存在");
    }

    const auto& station = stationOpt.value();

    // 查询站点内的充电桩
    QVector<repository::Charger> chargers = repository::ChargerRepository::findByStationId(stationId);

    // 构造充电桩列表
    QJsonArray chargersArray;
    int availableCount = 0;

    for (const auto& charger : chargers) {
        QJsonObject chargerObj;
        chargerObj["chargerId"] = charger.chargerId;
        chargerObj["code"] = charger.code;
        chargerObj["type"] = charger.type;
        chargerObj["status"] = charger.status;
        chargerObj["powerKw"] = charger.powerKw;
        chargersArray.append(chargerObj);

        if (charger.status == protocol::ChargerStatusIdle) {
            availableCount++;
        }
    }

    // 构造响应
    QJsonObject result;
    result["stationId"] = station.stationId;
    result["name"] = station.name;
    result["address"] = station.address;
    result["latitude"] = station.latitude;
    result["longitude"] = station.longitude;
    result["pricePerKwh"] = station.pricePerKwh;
    result["availableCount"] = availableCount;
    result["totalCount"] = chargers.size();
    result["chargers"] = chargersArray;

    qInfo() << "[StationService] Station detail: stationId=" << stationId
            << "chargers=" << chargers.size();

    return protocol::makeSuccessResponse(result);
}

double StationService::calculateDistance(double lat1, double lon1, double lat2, double lon2) const {
    // Haversine公式计算球面距离
    const double R = 6371000.0;  // 地球半径（米）

    double dLat = qDegreesToRadians(lat2 - lat1);
    double dLon = qDegreesToRadians(lon2 - lon1);

    double a = qSin(dLat / 2) * qSin(dLat / 2)
               + qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2))
               * qSin(dLon / 2) * qSin(dLon / 2);

    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));

    return R * c;  // 返回米
}

} // namespace service
