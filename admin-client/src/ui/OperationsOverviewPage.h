#pragma once

#include <QWidget>

class AdminApiClient;
class ChargerStatusOverviewWidget;
class QLabel;
class QListWidget;
class QPushButton;
class RevenueTrendChart;

class OperationsOverviewPage final : public QWidget
{
    Q_OBJECT

public:
    explicit OperationsOverviewPage(AdminApiClient* api,
                                    QWidget* parent = nullptr);

    void refresh();

private:
    QWidget* createMetricCard(const QString& label,
                              const QString& value,
                              const QString& hint,
                              const QString& accent,
                              QLabel** valueLabel,
                              QLabel** hintLabel = nullptr);
    QWidget* createSectionCard(const QString& title, QWidget* content);
    QWidget* createAttentionPanel();

    void refreshRevenue();
    void refreshStationCount();
    void refreshFaultChargers();
    void refreshUserCounts();
    void refreshActiveUserCount();
    void updateAttentionSummary();

    AdminApiClient* api_ = nullptr;
    ChargerStatusOverviewWidget* chargerOverviewWidget_ = nullptr;
    RevenueTrendChart* revenueTrendChart_ = nullptr;
    QListWidget* activeUserList_ = nullptr;
    QListWidget* faultList_ = nullptr;
    QLabel* todayRevenueLabel_ = nullptr;
    QLabel* monthRevenueLabel_ = nullptr;
    QLabel* totalRevenueLabel_ = nullptr;
    QLabel* stationTotalLabel_ = nullptr;
    QLabel* chargerTotalLabel_ = nullptr;
    QLabel* userTotalLabel_ = nullptr;
    QLabel* userHintLabel_ = nullptr;
    QLabel* attentionSummaryLabel_ = nullptr;
    QLabel* revenueUpdatedLabel_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    qint64 activeUserCount_ = -1;
    qint64 faultChargerCount_ = -1;
    int trendGeneration_ = 0;
};
