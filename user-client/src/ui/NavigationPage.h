#pragma once

#include "geo/RoutePlanner.h"

#include <QList>
#include <QPointF>
#include <QWidget>

class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;
class QWebEngineView;
class NetworkClient;
class Geocoder;

// 移动端导航流程：路线规划 -> 导航中 -> 导航结束。
// 地图与路线数据来自高德；未配置 key 时明确提示，不伪造路线。
class NavigationPage : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationPage(NetworkClient* networkClient, QWidget* parent = nullptr);

    void openRoute(double destinationLatitude,
                   double destinationLongitude,
                   const QString& destinationName,
                   bool walking = false);

    // 由定位/模拟器推送当前位置后，页面会同步高亮当前道路级步骤、更新顶部指引
    // 和高德地图上的当前位置标记。坐标顺序为纬度、经度。
    void updateUserPosition(double latitude, double longitude);

signals:
    void backRequested();
    void backToStationRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTopBackClicked();
    void onShareClicked();
    void onOriginReturnPressed();
    void onDestinationReturnPressed();
    void onSwapLocationsClicked();
    void onRouteOptionClicked();
    void onStartNavigationClicked();
    void onStopNavigationClicked();
    void onRecenterClicked();
    void onZoomInClicked();
    void onZoomOutClicked();
    void onLayersClicked();
    void onNavigationTick();
    void onRouteReady(const RouteResult& result);
    void onRouteError(const QString& message);
    void onGeocoded(double latitude, double longitude,
                    const QString& formattedAddress);
    void onGeocoderError(const QString& message);

private:
    enum ViewState { Planning, Navigating, Finished, Error };

    void buildUi();
    void layoutUi();
    void renderState();
    void renderPlanning();
    void renderNavigating();
    void renderFinished();
    void renderError(const QString& message);
    void requestRoute();
    void resolveLocation(bool originField);
    void replanFromCurrentInputs();
    void loadMap();
    void runMapJavaScript(const QString& script);
    QString buildNavigationMapDocument() const;
    void updatePlanningMetrics();
    void updateNavigationReadout();
    void updateGuidanceForProgress(double progress, int preferredStep = -1);
    double routeProgressForPosition(const QPointF& position) const;
    void setMapVisible(bool visible);
    void setAllStateWidgetsVisible(bool planVisible,
                                   bool navigationVisible,
                                   bool finishedVisible);
    QString formatDistance(double meters) const;
    QString formatDuration(qint64 seconds) const;
    QString formatEta(qint64 secondsFromNow) const;
    QString currentLocationLabel() const;

    NetworkClient* networkClient_ = nullptr;
    RoutePlanner* routePlanner_ = nullptr;
    Geocoder* geocoder_ = nullptr;

    QPushButton* topBackButton_ = nullptr;
    QPushButton* closeButton_ = nullptr;
    QPushButton* shareButton_ = nullptr;
    QLabel* pageTitle_ = nullptr;

    QFrame* routeSummaryCard_ = nullptr;
    QLabel* originCaptionLabel_ = nullptr;
    QLabel* destinationCaptionLabel_ = nullptr;
    QLineEdit* originEdit_ = nullptr;
    QLineEdit* destinationEdit_ = nullptr;
    QLabel* routeSummaryMetaLabel_ = nullptr;
    QPushButton* swapRouteButton_ = nullptr;

    QFrame* mapFrame_ = nullptr;
    QWebEngineView* mapView_ = nullptr;
    QLabel* mapStateLabel_ = nullptr;
    QPushButton* recenterButton_ = nullptr;
    QPushButton* zoomInButton_ = nullptr;
    QPushButton* zoomOutButton_ = nullptr;
    QPushButton* layersButton_ = nullptr;

    QFrame* routeOptionsCard_ = nullptr;
    QList<QPushButton*> routeOptionButtons_;
    QList<QLabel*> routeOptionTimeLabels_;
    QList<QLabel*> routeOptionMetaLabels_;
    QLabel* routeOptionsHintLabel_ = nullptr;
    QPushButton* returnStationButton_ = nullptr;
    QPushButton* startNavigationButton_ = nullptr;
    QLabel* planningHintLabel_ = nullptr;

    QFrame* guidanceCard_ = nullptr;
    QFrame* turnArrowWidget_ = nullptr;
    QLabel* guidanceDistanceLabel_ = nullptr;
    QLabel* guidanceRoadLabel_ = nullptr;

    QFrame* navigationBottomCard_ = nullptr;
    QLabel* remainingDistanceLabel_ = nullptr;
    QLabel* remainingEtaLabel_ = nullptr;
    QLabel* arrivalBatteryLabel_ = nullptr;
    QPushButton* stopNavigationButton_ = nullptr;

    QFrame* finishedCard_ = nullptr;
    QFrame* finishedStatusIcon_ = nullptr;
    QLabel* finishedStatusLabel_ = nullptr;
    QLabel* finishedStationLabel_ = nullptr;
    QLabel* finishedParkingLabel_ = nullptr;
    QLabel* finishedThanksLabel_ = nullptr;
    QLabel* finishedDurationLabel_ = nullptr;
    QLabel* finishedDistanceLabel_ = nullptr;
    QLabel* finishedParkingMetaLabel_ = nullptr;
    QFrame* finishNextStepsCard_ = nullptr;
    QLabel* nextStepsTitleLabel_ = nullptr;
    QList<QFrame*> finishStepCards_;
    QList<QLabel*> finishStepTitles_;
    QList<QLabel*> finishStepDetails_;
    QPushButton* finishBackDetailButton_ = nullptr;
    QPushButton* finishChargerListButton_ = nullptr;
    QPushButton* chooseChargerButton_ = nullptr;
    QLabel* finishedHintLabel_ = nullptr;

    QTimer* navigationTimer_ = nullptr;
    ViewState state_ = Planning;
    bool routeLoading_ = false;
    enum class LocationField { None, Origin, Destination };
    LocationField geocodingField_ = LocationField::None;
    int selectedRouteOption_ = 0;
    qint64 navigationStartedAtMs_ = 0;
    qint64 simulatedElapsedSeconds_ = 0;
    int activeStepIndex_ = 0;
    double navigationProgress_ = 0.0;
    QPointF currentPosition_;
    bool positionDriven_ = false;

    double destinationLatitude_ = 0.0;
    double destinationLongitude_ = 0.0;
    double originLatitude_ = 0.0;
    double originLongitude_ = 0.0;
    QString originName_;
    QString destinationName_;
    RouteResult route_;
};
