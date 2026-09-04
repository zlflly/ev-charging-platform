#ifndef CHARGER_REPOSITORY_H
#define CHARGER_REPOSITORY_H

#include <QString>
#include <QVector>
#include <optional>

namespace repository {

struct Charger {
    int chargerId;
    int stationId;
    QString code;
    int type;   // 0=快充, 1=慢充
    int status; // 0=空闲, 1=充电中, 2=故障, 3=离线
    double powerKw;
    int totalChargeCount;
    int totalChargeDurationSeconds;
    qint64 createdAt;
};

struct ChargerWithStation {
    Charger charger;
    QString stationName;
};

/**
 * 充电桩数据访问层
 */
class ChargerRepository {
public:
    // 根据 chargerId 查找充电桩
    static std::optional<Charger> findById(int chargerId);

    // 查询站点下的所有充电桩
    static QVector<Charger> findByStation(int stationId);

    // 查询所有充电桩（含站点名）
    static QVector<ChargerWithStation> findAllWithStation();

    // 更新充电桩状态
    static bool setStatus(int chargerId, int status);

    // 增加累计次数和时长（原子操作）
    static bool incrementStats(int chargerId, int durationSeconds);

    // 统计各状态充电桩数量
    struct StatusCount {
        int total;
        int idle;      // status = 0
        int charging;  // status = 1
        int fault;     // status = 2
        int offline;   // status = 3
    };
    static StatusCount countByStatus();
};

} // namespace repository

#endif // CHARGER_REPOSITORY_H
