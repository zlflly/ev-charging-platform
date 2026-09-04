#include "geo/RoutePlanner.h"

#include "config/AppConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QStringList>

namespace {

QByteArray readAmapApiKey()
{
    return qgetenv(appConfig::kAmapWebServiceKeyEnvironment).trimmed();
}

QString describeRouteError(const QString& info)
{
    const QString upper = info.toUpper();
    if (upper.contains(QStringLiteral("QPS")) ||
        upper.contains(QStringLiteral("EXCEEDED_THE_LIMIT"))) {
        return QStringLiteral("路线请求太频繁，请稍等几秒再试。");
    }
    if (upper.contains(QStringLiteral("INVALID_USER_KEY")) ||
        upper.contains(QStringLiteral("KEY")) ||
        upper.contains(QStringLiteral("INVALID")) ||
        upper.contains(QStringLiteral("DAILY_QUERY_OVER_LIMIT"))) {
        return QStringLiteral("高德地图 key 无效或配额已用完，请检查后重试。");
    }
    return info.isEmpty() ? QStringLiteral("路线规划失败，请稍后重试。") : info;
}

bool appendPolyline(const QString& encoded, QList<QPointF>* output)
{
    bool appended = false;
    const QStringList points = encoded.split(QChar(';'), Qt::SkipEmptyParts);
    for (const QString& point : points) {
        const QStringList coordinates = point.split(QChar(','));
        if (coordinates.size() != 2) continue;
        bool lngOk = false;
        bool latOk = false;
        const double longitude = coordinates.at(0).toDouble(&lngOk);
        const double latitude = coordinates.at(1).toDouble(&latOk);
        if (!lngOk || !latOk || (longitude == 0.0 && latitude == 0.0)) continue;
        const QPointF value(longitude, latitude);
        if (output->isEmpty() || output->last() != value) {
            output->append(value);
        }
        appended = true;
    }
    return appended;
}

double jsonNumber(const QJsonValue& value)
{
    if (value.isDouble()) {
        return value.toDouble();
    }
    return value.toString().toDouble();
}

qint64 jsonInteger(const QJsonValue& value)
{
    if (value.isDouble()) {
        return static_cast<qint64>(value.toDouble());
    }
    return value.toString().toLongLong();
}

} // namespace

RoutePlanner::RoutePlanner(QObject* parent)
    : QObject(parent)
    , networkManager_(new QNetworkAccessManager(this))
{
    if (appConfig::kHttpProxyHost[0] != '\0') {
        networkManager_->setProxy(QNetworkProxy(
            QNetworkProxy::HttpProxy,
            QString::fromLatin1(appConfig::kHttpProxyHost),
            appConfig::kHttpProxyPort));
    }
}

RoutePlanner::~RoutePlanner()
{
    if (activeReply_) activeReply_->abort();
}

bool RoutePlanner::hasApiKey()
{
    return !readAmapApiKey().isEmpty();
}

void RoutePlanner::plan(double originLatitude, double originLongitude,
                        double destinationLatitude, double destinationLongitude,
                        bool walking)
{
    if (isBusy_) return;
    if ((originLatitude == 0.0 && originLongitude == 0.0) ||
        (destinationLatitude == 0.0 && destinationLongitude == 0.0)) {
        emit error(QStringLiteral("路线规划失败：定位坐标无效。"));
        return;
    }

    const QByteArray apiKey = readAmapApiKey();
    if (apiKey.isEmpty()) {
        emit error(QStringLiteral(
            "高德地图 API key 未配置，请设置 AMAP_WEB_SERVICE_KEY 后重试。"));
        return;
    }

    const char* endpoint = walking ? appConfig::kAmapWalkingUrl
                                   : appConfig::kAmapDrivingUrl;
    QUrl url(QString::fromLatin1(endpoint));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("origin"),
                       QStringLiteral("%1,%2").arg(originLongitude, 0, 'f', 6)
                           .arg(originLatitude, 0, 'f', 6));
    query.addQueryItem(QStringLiteral("destination"),
                       QStringLiteral("%1,%2").arg(destinationLongitude, 0, 'f', 6)
                           .arg(destinationLatitude, 0, 'f', 6));
    if (!walking) query.addQueryItem(QStringLiteral("strategy"), QStringLiteral("0"));
    query.addQueryItem(QStringLiteral("extensions"), QStringLiteral("all"));
    query.addQueryItem(QStringLiteral("key"), QString::fromLatin1(apiKey));
    url.setQuery(query);

    activeReply_ = networkManager_->get(QNetworkRequest(url));
    connect(activeReply_, &QNetworkReply::finished,
            this, &RoutePlanner::onReplyFinished);
    isBusy_ = true;
}

void RoutePlanner::onReplyFinished()
{
    QNetworkReply* const reply = activeReply_;
    activeReply_ = nullptr;
    isBusy_ = false;
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit error(QStringLiteral("路线规划失败：无法连接高德地图服务（%1）。")
                       .arg(reply->errorString()));
        return;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit error(QStringLiteral("路线规划失败：高德地图返回数据无法解析。"));
        return;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("status")).toString() != QStringLiteral("1")) {
        emit error(describeRouteError(root.value(QStringLiteral("info")).toString()));
        return;
    }

    const QJsonObject route = root.value(QStringLiteral("route")).toObject();
    const QJsonArray paths = route.value(QStringLiteral("paths")).toArray();
    if (paths.isEmpty()) {
        emit error(QStringLiteral("路线规划失败：没有找到可行路线。"));
        return;
    }

    const QJsonObject path = paths.first().toObject();
    RouteResult result;
    result.distanceMeters = jsonNumber(path.value(QStringLiteral("distance")));
    result.durationSeconds = jsonInteger(path.value(QStringLiteral("duration")));
    const QJsonArray steps = path.value(QStringLiteral("steps")).toArray();
    for (int index = 0; index < steps.size(); ++index) {
        const QJsonObject step = steps.at(index).toObject();
        RouteStep routeStep;
        routeStep.instruction = step.value(QStringLiteral("instruction")).toString();
        routeStep.roadName = step.value(QStringLiteral("road")).toString();
        routeStep.distanceMeters = jsonNumber(step.value(QStringLiteral("distance")));
        routeStep.durationSeconds = jsonInteger(step.value(QStringLiteral("duration")));
        appendPolyline(step.value(QStringLiteral("polyline")).toString(),
                       &routeStep.path);
        result.steps.append(routeStep);
        if (index == 0) {
            result.firstInstruction = routeStep.instruction;
            result.firstRoadName = routeStep.roadName;
            result.firstStepDistanceMeters = routeStep.distanceMeters;
        }
        for (const QPointF& point : routeStep.path) {
            if (result.path.isEmpty() || result.path.last() != point) {
                result.path.append(point);
            }
        }
    }

    if (!result.valid()) {
        emit error(QStringLiteral("路线规划失败：路线数据不完整。"));
        return;
    }
    emit routeReady(result);
}
