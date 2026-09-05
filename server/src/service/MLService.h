#ifndef ML_SERVICE_H
#define ML_SERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QTcpSocket>

namespace service {

/**
 * 机器学习数据导出服务层
 * 处理 ml.* 相关的 action
 * 为成员4提供训练数据
 */
class MLService : public QObject {
    Q_OBJECT

public:
    explicit MLService(QObject* parent = nullptr);

    // ml.orders.export: 导出指定时间范围内的已完成订单数据
    QJsonObject handleOrdersExport(const QJsonObject& data, QTcpSocket* socket);

private:
    // 验证日期格式（YYYY-MM-DD）
    bool isValidDateFormat(const QString& date) const;

    // 解析日期字符串为时间戳（毫秒）
    qint64 parseDateToTimestamp(const QString& date) const;
};

} // namespace service

#endif // ML_SERVICE_H
