#pragma once

#include <QWidget>

#include <array>

class AdminApiClient;
class QLabel;
class QProgressBar;
class QPushButton;
struct ChargerStatusOverview;

class ChargerStatusOverviewWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ChargerStatusOverviewWidget(AdminApiClient* api,
                                         QWidget* parent = nullptr);

    void refresh();

signals:
    void overviewUpdated(qint64 total);

private:
    struct StatusWidgets {
        QLabel* count = nullptr;
        QLabel* percent = nullptr;
        QProgressBar* progress = nullptr;
    };

    QWidget* createStatusCard(int index,
                              const QString& title,
                              const QString& hint,
                              const QString& accent);
    void setLoading(bool loading);
    void showError(const QString& message);
    void showOverview(const ChargerStatusOverview& overview);

    AdminApiClient* api_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    std::array<StatusWidgets, 4> statuses_;
};
