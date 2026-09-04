#pragma once

#include "model/Order.h"
#include "model/Station.h"

#include <QMainWindow>
#include <QPointer>
#include <QStringList>

class Geocoder;
class NetworkClient;
class RoutePlanner;
class QButtonGroup;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;
class QWebEngineView;
struct RouteResult;

class EnergyMapWidget final : public QWidget
{
public:
    explicit EnergyMapWidget(QWidget* parent = nullptr);
    void setStations(const QList<StationInfo>& stations);
    void setCaption(const QString& caption);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QList<StationInfo> stations_;
    QString caption_ = QStringLiteral("输入位置，查找附近站点");
};

class ChargeGauge final : public QWidget
{
public:
    explicit ChargeGauge(QWidget* parent = nullptr);
    void setValue(double percent, const QString& centerText, const QString& caption);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    double percent_ = 0.0;
    QString centerText_ = QStringLiteral("—");
    QString caption_ = QStringLiteral("等待订单");
};

class AmapWidget final : public QWidget
{
public:
    explicit AmapWidget(QWidget* parent = nullptr);
    void setStations(const QList<StationInfo>& stations);
    void setCenter(double latitude, double longitude, const QString& label);
    void setRoute(const RouteResult& route);
    bool usesLiveMap() const { return liveMap_; }
private:
    void runScript(const QString& script);
    QWebEngineView* webView_ = nullptr;
    EnergyMapWidget* fallback_ = nullptr;
    bool liveMap_ = false;
    bool loaded_ = false;
    QStringList queuedScripts_;
};

class MobileApp final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MobileApp(NetworkClient* network, QWidget* parent = nullptr);

private:
    enum Page { LoginPage, HomePage, StationPage, ChargerPage, NavigationPage,
                ChargingPage, OrdersPage, ProfilePage };

    QWidget* buildLoginPage();
    QWidget* buildHomePage();
    QWidget* buildStationPage();
    QWidget* buildChargerPage();
    QWidget* buildNavigationPage();
    QWidget* buildChargingPage();
    QWidget* buildOrdersPage();
    QWidget* buildProfilePage();
    QWidget* buildBottomNavigation();

    void showPage(Page page);
    void setBusy(QPushButton* button, bool busy, const QString& normalText);
    void setNotice(QLabel* label, const QString& text, bool error = false);
    void attemptLogin();
    void locateAddress();
    void requestNearbyStations();
    void requestStationDetail(qint64 stationId);
    void renderStationList(const QList<StationInfo>& stations);
    void renderStationDetail(const StationDetail& detail);
    void showChargerDetail(const ChargerInfo& charger);
    void showNavigation();
    void requestActiveOrder(Page destination = ChargingPage);
    void renderOrder(const OrderInfo& order);
    void reserveCharger(const ChargerInfo& charger);
    void performOrderAction(const QString& action, QPushButton* source);
    void updateProfile();
    void recharge();
    void refreshSessionViews();
    void updateConnectionState(bool connected);

    NetworkClient* network_ = nullptr;
    Geocoder* geocoder_ = nullptr;
    RoutePlanner* routePlanner_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QWidget* bottomNav_ = nullptr;
    QButtonGroup* navGroup_ = nullptr;

    QLineEdit* phoneInput_ = nullptr;
    QPushButton* loginButton_ = nullptr;
    QLabel* loginNotice_ = nullptr;
    QLabel* connectionLabel_ = nullptr;

    QLabel* locationLabel_ = nullptr;
    QLineEdit* addressInput_ = nullptr;
    QPushButton* locateButton_ = nullptr;
    QLabel* homeNotice_ = nullptr;
    AmapWidget* mapWidget_ = nullptr;
    QWidget* stationListBody_ = nullptr;
    QVBoxLayout* stationListLayout_ = nullptr;

    QLabel* detailName_ = nullptr;
    QLabel* detailMeta_ = nullptr;
    QLabel* detailAddress_ = nullptr;
    QLabel* detailPrice_ = nullptr;
    QLabel* detailAvailability_ = nullptr;
    QLabel* detailNotice_ = nullptr;
    QWidget* chargerListBody_ = nullptr;
    QVBoxLayout* chargerListLayout_ = nullptr;
    StationDetail currentStation_;
    ChargerInfo currentCharger_;

    QLabel* chargerCode_ = nullptr;
    QLabel* chargerStatus_ = nullptr;
    QLabel* chargerPower_ = nullptr;
    QLabel* chargerMeta_ = nullptr;
    QLabel* chargerNotice_ = nullptr;
    QPushButton* reserveButton_ = nullptr;

    AmapWidget* navigationMap_ = nullptr;
    QLabel* navigationTarget_ = nullptr;
    QLabel* navigationDistance_ = nullptr;
    QLabel* navigationDuration_ = nullptr;
    QLabel* navigationNotice_ = nullptr;
    QPushButton* navigationAction_ = nullptr;

    ChargeGauge* gauge_ = nullptr;
    QLabel* chargingStatus_ = nullptr;
    QLabel* chargingStation_ = nullptr;
    QLabel* chargingMetrics_ = nullptr;
    QLabel* chargingAmount_ = nullptr;
    QLabel* chargingNotice_ = nullptr;
    QPushButton* chargingAction_ = nullptr;
    OrderInfo activeOrder_;

    QLabel* orderHeadline_ = nullptr;
    QLabel* orderSummary_ = nullptr;
    QLabel* orderNotice_ = nullptr;

    QLabel* profileName_ = nullptr;
    QLabel* profilePhone_ = nullptr;
    QLabel* profileBalance_ = nullptr;
    QLineEdit* nicknameInput_ = nullptr;
    QPushButton* nicknameButton_ = nullptr;
    QLineEdit* rechargeInput_ = nullptr;
    QPushButton* rechargeButton_ = nullptr;
    QLabel* profileNotice_ = nullptr;
};
