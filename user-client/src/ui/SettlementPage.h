#pragma once

#include "model/Order.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QFrame;
class NetworkClient;

// 结束充电后的结算确认页；最终金额和余额以服务端结算响应为准。
class SettlementPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettlementPage(NetworkClient* networkClient, QWidget* parent = nullptr);

    void openOrder(const OrderInfo& order);

signals:
    void backRequested();
    void homeRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onBackClicked();
    void onPrimaryClicked();

private:
    void buildUi();
    void layoutUi();
    void render();
    void settleOrder();
    void showError(const QString& message);
    bool previewMode() const;
    double displayBalance() const;

    NetworkClient* networkClient_ = nullptr;
    QPushButton* backButton_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QFrame* successIcon_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QFrame* summaryCard_ = nullptr;
    QLabel* stationLabel_ = nullptr;
    QLabel* chargerLabel_ = nullptr;
    QFrame* metricsCard_ = nullptr;
    QLabel* energyLabel_ = nullptr;
    QLabel* durationLabel_ = nullptr;
    QFrame* feeCard_ = nullptr;
    QLabel* electricityLabel_ = nullptr;
    QLabel* serviceLabel_ = nullptr;
    QLabel* totalLabel_ = nullptr;
    QFrame* balanceCard_ = nullptr;
    QLabel* balanceLabel_ = nullptr;
    QPushButton* primaryButton_ = nullptr;
    QLabel* hintLabel_ = nullptr;

    OrderInfo order_;
    bool settled_ = false;
    bool requestInFlight_ = false;
    double previewBalance_ = 128.60;
};
