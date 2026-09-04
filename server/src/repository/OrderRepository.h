#ifndef ORDER_REPOSITORY_H
#define ORDER_REPOSITORY_H

#include <QString>
#include <QVector>
#include <optional>

namespace repository {

struct Order {
    int orderId;
    int userId;
    int chargerId;
    int stationId;
    QString status;  // "RESERVED", "CHARGING", "WAIT_SETTLEMENT", "FINISHED"
    qint64 startTime;
    qint64 stopTime;
    qint64 settleTime;
    int duration;        // 秒
    double energyKwh;
    double amount;
    qint64 createdAt;
};

/**
 * 订单数据访问层
 */
class OrderRepository {
public:
    // 根据 orderId 查找订单
    static std::optional<Order> findById(int orderId);

    // 查询用户的未完成订单（RESERVED/CHARGING/WAIT_SETTLEMENT）
    static std::optional<Order> findActiveByUser(int userId);

    // 查询用户的所有订单（按创建时间倒序）
    static QVector<Order> findByUser(int userId);

    // 查询所有已完成订单（用于统计和ML）
    static QVector<Order> findFinished();

    // 查询指定时间范围内的已完成订单
    static QVector<Order> findFinishedInRange(qint64 startTime, qint64 endTime);

    // 创建预约订单
    static std::optional<Order> createReservation(int userId, int chargerId, int stationId);

    // 更新订单状态
    static bool setStatus(int orderId, const QString& status);

    // 开始充电（记录开始时间）
    static bool startCharging(int orderId, qint64 startTime);

    // 停止充电（记录停止时间、时长、电量、金额）
    static bool stopCharging(int orderId, qint64 stopTime, int duration, double energyKwh, double amount);

    // 结算订单（记录结算时间）
    static bool settle(int orderId, qint64 settleTime);
};

} // namespace repository

#endif // ORDER_REPOSITORY_H
