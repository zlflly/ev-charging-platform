#include "geo/Geocoder.h"

#include "config/AppConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

QString describeAmapError(const QString& info)
{
    const QString upper = info.toUpper();
    if (upper.contains(QStringLiteral("QPS")) ||
        upper.contains(QStringLiteral("EXCEEDED_THE_LIMIT"))) {
        return QStringLiteral("定位请求太频繁，请稍等几秒再试。");
    }
    if (upper.contains(QStringLiteral("INVALID_USER_KEY")) ||
        upper.contains(QStringLiteral("KEY")) ||
        upper.contains(QStringLiteral("INVALID")) ||
        upper.contains(QStringLiteral("DAILY_QUERY_OVER_LIMIT"))) {
        return QStringLiteral("高德地图 key 无效或配额已用完，请检查后重试。");
    }
    return info.isEmpty() ? QStringLiteral("定位失败，请稍后重试。") : info;
}

} // namespace

Geocoder::Geocoder(QObject* parent)
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

Geocoder::~Geocoder()
{
    if (activeReply_) {
        activeReply_->abort();
    }
}

bool Geocoder::hasApiKey()
{
    return !readAmapApiKey().isEmpty();
}

void Geocoder::geocode(const QString& address)
{
    if (isBusy_) {
        return;
    }

    const QString trimmedAddress = address.trimmed();
    if (trimmedAddress.isEmpty()) {
        emit error(QStringLiteral("请输入要定位的地址。"));
        return;
    }

    const QByteArray apiKey = readAmapApiKey();
    if (apiKey.isEmpty()) {
        emit error(QStringLiteral(
            "高德地图 API key 未配置，请设置 AMAP_WEB_SERVICE_KEY 后重试。"));
        return;
    }

    QUrl url(QString::fromLatin1(appConfig::kAmapGeocoderUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("address"), trimmedAddress);
    query.addQueryItem(QStringLiteral("key"), QString::fromLatin1(apiKey));
    url.setQuery(query);

    activeReply_ = networkManager_->get(QNetworkRequest(url));
    connect(activeReply_, &QNetworkReply::finished,
            this, &Geocoder::onReplyFinished);
    isBusy_ = true;
}

void Geocoder::onReplyFinished()
{
    QNetworkReply* const reply = activeReply_;
    activeReply_ = nullptr;
    isBusy_ = false;
    if (!reply) {
        return;
    }
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit error(QStringLiteral("定位失败：无法连接高德地图服务（%1）。")
                       .arg(reply->errorString()));
        return;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit error(QStringLiteral("定位失败：高德地图返回数据无法解析。"));
        return;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("status")).toString() != QStringLiteral("1")) {
        emit error(describeAmapError(root.value(QStringLiteral("info")).toString()));
        return;
    }

    const QJsonArray geocodes = root.value(QStringLiteral("geocodes")).toArray();
    if (geocodes.isEmpty()) {
        emit error(QStringLiteral("定位失败：未解析到该地址的坐标。"));
        return;
    }

    const QJsonObject geocode = geocodes.first().toObject();
    const QStringList coordinates = geocode.value(QStringLiteral("location"))
                                        .toString().split(QChar(','));
    if (coordinates.size() != 2) {
        emit error(QStringLiteral("定位失败：高德地图返回坐标格式异常。"));
        return;
    }

    const double longitude = coordinates.at(0).toDouble();
    const double latitude = coordinates.at(1).toDouble();
    if (latitude == 0.0 && longitude == 0.0) {
        emit error(QStringLiteral("定位失败：未解析到该地址的坐标。"));
        return;
    }

    emit geocoded(latitude, longitude,
                  geocode.value(QStringLiteral("formatted_address")).toString());
}
