#include "model/Revenue.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QSet>
#include <QTimeZone>

#include <cmath>

namespace {

constexpr double kMaximumRevenue = 1000000000000.0;

QString formatMoney(double value)
{
    return QStringLiteral("¥ %1").arg(value, 0, 'f', 2);
}

bool readRevenue(const QJsonObject& json, const QString& key,
                 double* result, QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    const double number = value.toDouble(-1.0);
    if (!value.isDouble() || !std::isfinite(number) || number < 0.0
        || number > kMaximumRevenue) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 %1 必须是 0～%2 的有限金额")
                                .arg(key).arg(kMaximumRevenue, 0, 'f', 0);
        }
        return false;
    }
    *result = number;
    return true;
}

QDateTime parseIsoTime(const QString& value)
{
    QDateTime parsed = QDateTime::fromString(value, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(value, Qt::ISODate);
    }
    return parsed;
}

bool readTime(const QJsonObject& json, const QString& key,
              qint64* epochMs, QString* errorMessage)
{
    const QJsonValue value = json.value(key);
    if (value.isString()) {
        const QDateTime parsed = parseIsoTime(value.toString());
        if (parsed.isValid()) {
            *epochMs = parsed.toMSecsSinceEpoch();
            return true;
        }
    } else if (value.isDouble()) {
        const double number = value.toDouble(-1.0);
        if (std::isfinite(number) && number > 0.0
            && number <= 9007199254740991.0 && std::floor(number) == number) {
            *epochMs = static_cast<qint64>(number);
            return true;
        }
    }
    if (errorMessage) {
        *errorMessage = QStringLiteral("字段 %1 必须是 ISO 8601 时间或 epoch 毫秒")
                            .arg(key);
    }
    return false;
}

bool readContractMetadata(const QJsonObject& json,
                          QString* currency,
                          QString* timezone,
                          qint64* generatedAt,
                          bool requireCurrency,
                          QString* errorMessage)
{
    if (requireCurrency) {
        const QJsonValue currencyValue = json.value(QStringLiteral("currency"));
        if (!currencyValue.isString() || currencyValue.toString() != QStringLiteral("CNY")) {
            if (errorMessage) *errorMessage = QStringLiteral("字段 currency 必须为 CNY");
            return false;
        }
        *currency = currencyValue.toString();
    }
    const QJsonValue timezoneValue = json.value(QStringLiteral("timezone"));
    if (!timezoneValue.isString()
        || timezoneValue.toString() != QStringLiteral("Asia/Shanghai")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("字段 timezone 必须为 Asia/Shanghai");
        }
        return false;
    }
    *timezone = timezoneValue.toString();
    return readTime(json, QStringLiteral("generatedAt"), generatedAt, errorMessage);
}

} // namespace

QString RevenueSummary::formattedTodayRevenue() const
{
    return formatMoney(todayRevenue);
}

QString RevenueSummary::formattedMonthRevenue() const
{
    return formatMoney(monthRevenue);
}

QString RevenueSummary::formattedTotalRevenue() const
{
    return formatMoney(totalRevenue);
}

QString RevenueSummary::formattedGeneratedAt() const
{
    return QDateTime::fromMSecsSinceEpoch(generatedAtEpochMs)
        .toTimeZone(QTimeZone(QByteArrayLiteral("Asia/Shanghai")))
        .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

bool RevenueSummary::fromJson(const QJsonObject& json,
                              RevenueSummary* summary,
                              QString* errorMessage)
{
    if (!summary) return false;
    RevenueSummary parsed;
    if (!readRevenue(json, QStringLiteral("todayRevenue"),
                     &parsed.todayRevenue, errorMessage)
        || !readRevenue(json, QStringLiteral("monthRevenue"),
                        &parsed.monthRevenue, errorMessage)
        || !readRevenue(json, QStringLiteral("totalRevenue"),
                        &parsed.totalRevenue, errorMessage)
        || !readContractMetadata(json, &parsed.currency, &parsed.timezone,
                                 &parsed.generatedAtEpochMs, true, errorMessage)) {
        return false;
    }
    constexpr double epsilon = 0.005;
    if (parsed.todayRevenue > parsed.monthRevenue + epsilon
        || parsed.monthRevenue > parsed.totalRevenue + epsilon) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("今日、本月、累计营收的包含关系不一致");
        }
        return false;
    }
    *summary = parsed;
    if (errorMessage) errorMessage->clear();
    return true;
}

QString RevenueTrend::formattedGeneratedAt() const
{
    return QDateTime::fromMSecsSinceEpoch(generatedAtEpochMs)
        .toTimeZone(QTimeZone(QByteArrayLiteral("Asia/Shanghai")))
        .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

bool RevenueTrend::fromJson(const QJsonObject& json,
                            int expectedDays,
                            RevenueTrend* trend,
                            QString* errorMessage)
{
    if (!trend) return false;
    if (expectedDays != 7 && expectedDays != 30) {
        if (errorMessage) *errorMessage = QStringLiteral("趋势天数只能是 7 或 30");
        return false;
    }
    const QJsonValue daysValue = json.value(QStringLiteral("days"));
    const double daysNumber = daysValue.toDouble(-1.0);
    if (!daysValue.isDouble() || std::floor(daysNumber) != daysNumber
        || daysNumber != expectedDays) {
        if (errorMessage) *errorMessage = QStringLiteral("响应 days 与请求不匹配");
        return false;
    }
    RevenueTrend parsed;
    parsed.days = expectedDays;
    QString unusedCurrency;
    if (!readContractMetadata(json, &unusedCurrency, &parsed.timezone,
                              &parsed.generatedAtEpochMs, false, errorMessage)) {
        return false;
    }
    const QJsonValue pointsValue = json.value(QStringLiteral("points"));
    if (!pointsValue.isArray()) {
        if (errorMessage) *errorMessage = QStringLiteral("响应缺少 points 数组");
        return false;
    }
    const QJsonArray points = pointsValue.toArray();
    if (points.size() != expectedDays) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 日趋势必须包含恰好 %1 个日期桶")
                                .arg(expectedDays);
        }
        return false;
    }
    QDate previousDate;
    QSet<QDate> dates;
    for (int index = 0; index < points.size(); ++index) {
        if (!points.at(index).isObject()) {
            if (errorMessage) *errorMessage = QStringLiteral("第 %1 个趋势点不是对象").arg(index + 1);
            return false;
        }
        const QJsonObject pointJson = points.at(index).toObject();
        const QJsonValue dateValue = pointJson.value(QStringLiteral("date"));
        const QDate date = QDate::fromString(dateValue.toString(), QStringLiteral("yyyy-MM-dd"));
        double revenue = 0.0;
        if (!dateValue.isString() || !date.isValid()) {
            if (errorMessage) *errorMessage = QStringLiteral("第 %1 个趋势点日期无效").arg(index + 1);
            return false;
        }
        if (!readRevenue(pointJson, QStringLiteral("revenue"), &revenue, errorMessage)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("第 %1 个趋势点：%2")
                                    .arg(index + 1).arg(*errorMessage);
            }
            return false;
        }
        if (dates.contains(date) || (previousDate.isValid()
                                     && previousDate.daysTo(date) != 1)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("趋势日期必须升序、唯一且连续");
            }
            return false;
        }
        dates.insert(date);
        previousDate = date;
        parsed.points.append({date, revenue});
    }
    *trend = parsed;
    if (errorMessage) errorMessage->clear();
    return true;
}
