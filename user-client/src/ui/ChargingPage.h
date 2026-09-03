#pragma once

#include "model/Order.h"
#include "model/Station.h"

#include <QWidget>

class ChargingHeroWidget;
class QLabel;
class QPushButton;
class QFrame;
class QTimer;
class NetworkClient;

// 移动端充电流程页：未预约 -> 已预约 -> 充电中 -> 结算。
// 正常运行时动作交给服务端；预览模式用于无服务端时验证完整交互。
class ChargingPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChargingPage(NetworkClient* networkClient, QWidget* parent = nullptr);

    void openWithCharger(const ChargerInfo& charger,
                         const QString& stationName,
                         double pricePerKwh);

signals:
    void backRequested();
    void settlementRequested(const OrderInfo& order);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onActionClicked();
    void onBackClicked();
    void onCopyOrderClicked();
    void onFeeDetailsClicked();
    void onChargingTick();

private:
    enum LocalState { Initial, Reserved, Charging, Error };

    void buildUi();
    void layoutUi();
    void requestActiveOrder();
    void requestOrderStatus();
    void reserveCharger();
    void startCharging();
    void stopCharging();
    void renderState();
    void renderInitialState();
    void renderReservedState();
    void renderChargingState();
    void renderErrorState(const QString& message);
    void updateChargingReadout();
    void updateBalanceLabel();
    void setRequestInFlight(bool inFlight);
    void showFeeDetails();
    void setOrderFromPending();
    QString formatDuration(qint64 milliseconds) const;
    double progressForEnergy(double energyKwh) const;
    double displayBalance() const;
    bool previewMode() const;

    NetworkClient* networkClient_ = nullptr;

    QPushButton* backButton_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* orderLabel_ = nullptr;
    QPushButton* copyOrderButton_ = nullptr;
    ChargingHeroWidget* heroWidget_ = nullptr;

    QFrame* metricsCard_ = nullptr;
    QLabel* energyTitleLabel_ = nullptr;
    QLabel* energyValueLabel_ = nullptr;
    QLabel* durationTitleLabel_ = nullptr;
    QLabel* durationValueLabel_ = nullptr;
    QLabel* powerTitleLabel_ = nullptr;
    QLabel* powerValueLabel_ = nullptr;

    QFrame* costCard_ = nullptr;
    QLabel* estimatedTitleLabel_ = nullptr;
    QLabel* estimatedValueLabel_ = nullptr;
    QLabel* balanceTitleLabel_ = nullptr;
    QLabel* balanceValueLabel_ = nullptr;

    QPushButton* feeDetailsButton_ = nullptr;
    QPushButton* actionButton_ = nullptr;
    QLabel* hintLabel_ = nullptr;

    QTimer* ticker_ = nullptr;
    ChargerInfo pendingCharger_;
    QString pendingStationName_;
    double pendingPricePerKwh_ = 0.0;
    OrderInfo currentOrder_;
    LocalState state_ = Initial;
    bool hasOrder_ = false;
    bool requestInFlight_ = false;
    bool pollInFlight_ = false;
    bool serverEnergyKnown_ = false;
    int pollTicks_ = 0;
    double previewBalance_ = 128.60;
};
