#pragma once

#include <QList>
#include <QMainWindow>

class QLabel;
class NetworkClient;
class AdminSession;
class QPushButton;
class QStackedWidget;
class QWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(NetworkClient* network,
                        AdminSession* session,
                        QWidget* parent = nullptr);

signals:
    void logoutRequested();

private:
    QWidget* createSidebar();
    QWidget* createTopbar();
    QWidget* createOverviewPage();
    QWidget* createChargerPage();
    QWidget* createStationPage();
    QWidget* createUserPage();
    QWidget* createRevenuePage();
    QWidget* createMetricCard(const QString& label,
                              const QString& value,
                              const QString& hint,
                              const QString& accent);
    QWidget* createSectionCard(const QString& title, QWidget* content);
    QWidget* createPlaceholderPanel(const QString& title,
                                    const QString& message,
                                    const QString& accent);

    void addNavigationButton(const QString& text, int pageIndex);
    void switchPage(int pageIndex);
    void connectToServer();
    void sendPing();
    void updateClock();
    void setConnectionState(const QString& state, const QString& text);
    void refreshAdminInfo();

    NetworkClient* network_ = nullptr;
    AdminSession* session_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QLabel* pageTitle_ = nullptr;
    QLabel* pageSubtitle_ = nullptr;
    QLabel* clockLabel_ = nullptr;
    QLabel* connectionBadge_ = nullptr;
    QLabel* adminNameLabel_ = nullptr;
    QLabel* adminAccountLabel_ = nullptr;
    QPushButton* connectionButton_ = nullptr;
    QList<QPushButton*> navigationButtons_;
};
