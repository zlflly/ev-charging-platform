#pragma once

#include <QList>
#include <QObject>
#include <QPointF>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// 高德 direction API 返回的道路级导航步骤。
struct RouteStep
{
    QString instruction;
    QString roadName;
    QString orientation;
    QString action;
    QString assistantAction;
    double distanceMeters = 0.0;
    qint64 durationSeconds = 0;
    QList<QPointF> path;
};

// 高德路线规划结果。QPointF 的 x 为经度，y 为纬度，方便直接喂给地图页面。
struct RouteResult
{
    double distanceMeters = 0.0;
    qint64 durationSeconds = 0;
    QList<QPointF> path;
    QString firstInstruction;
    QString firstRoadName;
    double firstStepDistanceMeters = 0.0;
    QList<RouteStep> steps;

    bool valid() const { return distanceMeters > 0.0 && !path.isEmpty(); }
};

Q_DECLARE_METATYPE(RouteResult)

// 地址/站点坐标 -> 路线：页面不直接处理高德网络请求。
class RoutePlanner : public QObject
{
    Q_OBJECT

public:
    explicit RoutePlanner(QObject* parent = nullptr);
    ~RoutePlanner() override;

    void plan(double originLatitude, double originLongitude,
              double destinationLatitude, double destinationLongitude,
              bool walking = false);
    static bool hasApiKey();
    bool isBusy() const { return isBusy_; }

signals:
    void routeReady(const RouteResult& result);
    void error(const QString& message);

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager* networkManager_ = nullptr;
    QNetworkReply* activeReply_ = nullptr;
    bool isBusy_ = false;
};
