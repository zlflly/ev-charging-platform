#pragma once

#include "model/Station.h"

#include <QList>
#include <QWidget>

class Geocoder;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QWebEngineView;
class NetworkClient;

// 登录后的手机端首页：地址搜索、高德暗色地图、排序和真实附近站点列表。
// 顶部只保留产品标题和搜索栏；城市徽标、扫码入口不属于本移动端版本。
class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(NetworkClient* networkClient, QWidget* parent = nullptr);
    ~HomePage() override;

    void refreshStations();

signals:
    void stationSelected(qint64 stationId);
    void stationsRequested();
    void chargingRequested();
    void ordersRequested();
    void profileRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onSearchClicked();
    void onSortClicked();
    void onLocateClicked();
    void onGeocoded(double latitude, double longitude, const QString& address);
    void onGeocodeError(const QString& message);

private:
    void buildUi();
    void applyDefaultLocation();
    void refreshMap();
    void requestNearbyStations();
    void showLoadingState();
    void showErrorState(const QString& message);
    void showEmptyState();
    void renderStations();
    void clearList();
    void layoutStationItems();
    void applySort(int sortMode);
    void updateSortButtons();
    QString buildHomeMapDocument(double longitude, double latitude) const;

    NetworkClient* networkClient_ = nullptr;
    Geocoder* geocoder_ = nullptr;

    QLabel* titleLabel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QPushButton* searchButton_ = nullptr;
    QFrame* mapFrame_ = nullptr;
    QWebEngineView* mapView_ = nullptr;
    QPushButton* recenterButton_ = nullptr;
    QScrollArea* stationScroll_ = nullptr;
    QWidget* stationContainer_ = nullptr;
    QList<QPushButton*> sortButtons_;
    QList<QWidget*> stationItems_;
    QList<StationInfo> stations_;
    QLabel* footerLabel_ = nullptr;
    QFrame* bottomBar_ = nullptr;
    QList<QPushButton*> navButtons_;
    int sortMode_ = 0;
    quint64 stationQueryVersion_ = 0;
    bool firstShowHandled_ = false;
    bool previewMode_ = false;
};
