#pragma once

#include "model/Station.h"

#include <QList>
#include <QWidget>

class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class NetworkClient;

// 移动端站点详情：返回、站点摘要、计费统计、充电桩筛选和底部操作。
// 右上角不放分享入口，导航由底部主按钮触发 navigationRequested 信号。
class StationDetailPage : public QWidget
{
    Q_OBJECT

public:
    explicit StationDetailPage(NetworkClient* networkClient, QWidget* parent = nullptr);

    void openStation(qint64 stationId);
    void refresh();

signals:
    void chargerSelected(const ChargerInfo& charger,
                         const QString& stationName,
                         double pricePerKwh);
    void backRequested();
    void navigationRequested(double destinationLatitude,
                             double destinationLongitude,
                             const QString& destinationName,
                             bool walking);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onNavigateClicked();
    void onFavoriteClicked();
    void onFilterClicked();

private:
    enum FilterMode { FilterAll = 0, FilterFast = 1, FilterSlow = 2, FilterIdle = 3 };

    void buildUi();
    void requestStationDetail();
    void renderDetail(const StationDetail& detail);
    void renderChargerList();
    void updateStats(const StationDetail& detail);
    void updateFavoriteButton();
    void showLoadingState();
    void showErrorState(const QString& message);
    void showEmptyChargersState();
    void clearChargerList();
    void updateFilterButtons();
    void layoutUi();
    double computeDistanceKm() const;
    bool isFavorite() const;
    void setFavorite(bool favorite);
    StationDetail previewDetail() const;

    NetworkClient* networkClient_ = nullptr;

    QPushButton* backButton_ = nullptr;
    QLabel* pageTitle_ = nullptr;
    QFrame* stationInfoCard_ = nullptr;
    QFrame* ratingWidget_ = nullptr;
    QFrame* primaryTag_ = nullptr;
    QFrame* hoursTag_ = nullptr;
    QFrame* openTag_ = nullptr;
    QFrame* addressIcon_ = nullptr;
    QFrame* distanceIcon_ = nullptr;
    QLabel* stationNameLabel_ = nullptr;
    QLabel* ratingLabel_ = nullptr;
    QLabel* addressLabel_ = nullptr;
    QLabel* distanceLabel_ = nullptr;

    QFrame* statsCard_ = nullptr;
    QLabel* priceLabel_ = nullptr;
    QLabel* billingLabel_ = nullptr;
    QFrame* statsHorizontalDivider_ = nullptr;
    QLabel* totalStatLabel_ = nullptr;
    QLabel* idleStatLabel_ = nullptr;
    QLabel* fastStatLabel_ = nullptr;
    QLabel* slowStatLabel_ = nullptr;
    QList<QLabel*> statCaptions_;
    QList<QFrame*> statDividers_;

    QFrame* listCard_ = nullptr;
    QLabel* listTitleLabel_ = nullptr;
    QList<QPushButton*> filterButtons_;
    QScrollArea* chargerScroll_ = nullptr;
    QWidget* chargerContainer_ = nullptr;
    QVBoxLayout* chargerListLayout_ = nullptr;

    QPushButton* favoriteButton_ = nullptr;
    QPushButton* navigateButton_ = nullptr;

    qint64 currentStationId_ = 0;
    QString currentStationName_;
    QString currentAddress_;
    double stationLatitude_ = 0.0;
    double stationLongitude_ = 0.0;
    double currentPricePerKwh_ = 0.0;
    QList<ChargerInfo> currentChargers_;
    int filterMode_ = FilterAll;
    bool previewMode_ = false;
};
