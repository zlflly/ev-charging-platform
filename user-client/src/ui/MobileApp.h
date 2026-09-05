#pragma once

#include "geo/RoutePlanner.h"
#include "model/Order.h"
#include "model/Station.h"

#include <QMainWindow>
#include <QPointer>
#include <QStringList>
#include <QVector>
#include <QColor>

class Geocoder;
class NetworkClient;
class QButtonGroup;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;
class QWebEngineView;

class EnergyMapWidget final : public QWidget
{
public:
    explicit EnergyMapWidget(QWidget* parent = nullptr);
    void setStations(const QList<StationInfo>& stations);
    void setCaption(const QString& caption);
    void setRoute(const RouteResult& route);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QList<StationInfo> stations_;
    QList<QPointF> routePath_;
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

class SparklineWidget final : public QWidget
{
public:
    explicit SparklineWidget(const QColor& color, QWidget* parent = nullptr);
    void setSamples(const QList<double>& samples);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QColor color_;
    QVector<double> samples_;
};

class MetricGlyphWidget final : public QWidget
{
public:
    enum Kind { Battery, Bolt, Clock, Coin };
    MetricGlyphWidget(Kind kind, const QColor& color, const QColor& background,
                      QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    Kind kind_;
    QColor color_;
    QColor background_;
};

class RouteTurnGlyph final : public QWidget
{
public:
    explicit RouteTurnGlyph(bool compact = false, QWidget* parent = nullptr);
    void setManeuver(const QString& maneuver, bool active = true);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString maneuver_ = QStringLiteral("straight");
    bool active_ = true;
    bool compact_ = false;
};

class AmapWidget final : public QWidget
{
public:
    explicit AmapWidget(QWidget* parent = nullptr);
    void setStations(const QList<StationInfo>& stations);
    void setCenter(double latitude, double longitude, const QString& label);
    void setRoute(const RouteResult& route, const QString& destinationLabel = QString());
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
    void requestStationDetail(qint64 stationId, bool navigateAfter = false);
    void renderStationList(const QList<StationInfo>& stations);
    void applyHomeFilter();
    void renderStationDetail(const StationDetail& detail);
    void showChargerDetail(const ChargerInfo& charger);
    void showNavigation();
    void applyNavigationRoute(const RouteResult& route);
    void updateNavigationStep(int index);
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
    QLabel* stationCountLabel_ = nullptr;
    AmapWidget* mapWidget_ = nullptr;
    QWidget* stationListBody_ = nullptr;
    QVBoxLayout* stationListLayout_ = nullptr;
    QList<StationInfo> nearbyStations_;
    int homeFilter_ = 0;
    bool navigateAfterDetail_ = false;

    QLabel* detailName_ = nullptr;
    QLabel* detailMeta_ = nullptr;
    QLabel* detailAddress_ = nullptr;
    QLabel* detailPrice_ = nullptr;
    QLabel* detailAvailability_ = nullptr;
    QLabel* detailTotal_ = nullptr;
    QLabel* detailFast_ = nullptr;
    QLabel* detailSlow_ = nullptr;
    QLabel* detailNotice_ = nullptr;
    QWidget* chargerListBody_ = nullptr;
    QVBoxLayout* chargerListLayout_ = nullptr;
    StationDetail currentStation_;
    ChargerInfo currentCharger_;
    ChargerInfo preferredCharger_;

    QLabel* chargerCode_ = nullptr;
    QLabel* chargerStationLabel_ = nullptr;
    QLabel* chargerStatus_ = nullptr;
    QLabel* chargerHint_ = nullptr;
    QLabel* chargerPower_ = nullptr;
    QLabel* chargerMeta_ = nullptr;
    QLabel* chargerPrice_ = nullptr;
    QLabel* chargerLocation_ = nullptr;
    QLabel* chargerMethod_ = nullptr;
    QLabel* chargerStateSpec_ = nullptr;
    QLabel* chargerNotice_ = nullptr;
    QPushButton* reserveButton_ = nullptr;

    AmapWidget* navigationMap_ = nullptr;
    RouteTurnGlyph* navigationTurnGlyph_ = nullptr;
    QLabel* navigationStepLead_ = nullptr;
    QLabel* navigationRoad_ = nullptr;
    QLabel* navigationTarget_ = nullptr;
    QLabel* navigationStationMeta_ = nullptr;
    QLabel* navigationStatus_ = nullptr;
    QLabel* navigationAvailability_ = nullptr;
    QLabel* navigationPrice_ = nullptr;
    QLabel* navigationMapEta_ = nullptr;
    QLabel* navigationEta_ = nullptr;
    QLabel* navigationEtaHint_ = nullptr;
    QLabel* navigationDistance_ = nullptr;
    QLabel* navigationDuration_ = nullptr;
    QLabel* navigationNotice_ = nullptr;
    QPushButton* navigationEndAction_ = nullptr;
    QPushButton* navigationAction_ = nullptr;
    RouteResult navigationRoute_;
    int navigationStepIndex_ = 0;

    ChargeGauge* gauge_ = nullptr;
    QLabel* chargingStatus_ = nullptr;
    QLabel* chargingStation_ = nullptr;
    QLabel* chargingMetrics_ = nullptr;
    QLabel* chargingStart_ = nullptr;
    QLabel* chargingMode_ = nullptr;
    QLabel* chargingStateText_ = nullptr;
    QLabel* chargingAmount_ = nullptr;
    QLabel* chargingPowerLive_ = nullptr;
    QLabel* chargingEnergyLive_ = nullptr;
    QLabel* chargingDuration_ = nullptr;
    QLabel* chargingRemaining_ = nullptr;
    QLabel* chargingFeeSummary_ = nullptr;
    QLabel* chargingEnergyTile_ = nullptr;
    QLabel* chargingPowerTile_ = nullptr;
    QLabel* chargingDurationTile_ = nullptr;
    QLabel* chargingFeeTile_ = nullptr;
    QLabel* chargingElectrical_ = nullptr;
    SparklineWidget* powerWave_ = nullptr;
    SparklineWidget* energyWave_ = nullptr;
    QPushButton* chargingOrderButton_ = nullptr;
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
