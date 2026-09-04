#ifndef STATION_REPOSITORY_H
#define STATION_REPOSITORY_H

#include <QString>
#include <QVector>
#include <optional>

namespace repository {

struct Station {
    int stationId;
    QString name;
    QString address;
    double latitude;
    double longitude;
    double pricePerKwh;
    int status;  // 0=active, 1=inactive
    qint64 createdAt;
};

/**
 * 充电站数据访问层
 */
class StationRepository {
public:
    // 根据 stationId 查找站点
    static std::optional<Station> findById(int stationId);

    // 查询所有站点
    static QVector<Station> findAll();

    // 查询活跃站点
    static QVector<Station> findActive();

    // 创建站点
    static std::optional<Station> create(const QString& name, const QString& address,
                                         double latitude, double longitude, double pricePerKwh);

    // 更新站点状态
    static bool setStatus(int stationId, int status);
};

} // namespace repository

#endif // STATION_REPOSITORY_H
