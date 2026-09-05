#pragma once

#include "model/Revenue.h"

#include <QWidget>

class AdminApiClient;
class QLabel;
class QPushButton;
class RevenueTrendChart;

class RevenueStatisticsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit RevenueStatisticsPage(AdminApiClient* api,
                                   QWidget* parent = nullptr);

    void refresh();
    void refreshSummary();

signals:
    void summaryUpdated(double todayRevenue,
                        double monthRevenue,
                        double totalRevenue,
                        const QString& generatedAt);
    void summaryInvalidated();

private:
    QWidget* createMetricCard(const QString& title,
                              const QString& accent,
                              QLabel** valueLabel);
    void selectDays(int days);
    void requestSummary();
    void requestTrend();
    void setSummaryLoading(bool loading);
    void showSummary(const RevenueSummary& summary);
    void showSummaryError(const QString& message);
    void showTrend(const RevenueTrend& trend);

    AdminApiClient* api_ = nullptr;
    QLabel* todayValue_ = nullptr;
    QLabel* monthValue_ = nullptr;
    QLabel* totalValue_ = nullptr;
    QLabel* summaryState_ = nullptr;
    QLabel* trendState_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    QPushButton* sevenDayButton_ = nullptr;
    QPushButton* thirtyDayButton_ = nullptr;
    RevenueTrendChart* chart_ = nullptr;
    int selectedDays_ = 7;
    quint64 trendGeneration_ = 0;
};
