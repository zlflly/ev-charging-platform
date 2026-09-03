#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// 地址 -> 经纬度：页面只调用 geocode，不直接处理高德网络请求。
class Geocoder : public QObject
{
    Q_OBJECT

public:
    explicit Geocoder(QObject* parent = nullptr);
    ~Geocoder() override;

    void geocode(const QString& address);
    static bool hasApiKey();
    bool isBusy() const { return isBusy_; }

signals:
    void geocoded(double latitude, double longitude, const QString& formattedAddress);
    void error(const QString& message);

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager* networkManager_ = nullptr;
    QNetworkReply* activeReply_ = nullptr;
    bool isBusy_ = false;
};
