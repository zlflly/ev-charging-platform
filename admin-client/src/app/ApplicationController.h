#pragma once

#include <QObject>
#include <QString>

class AdminApiClient;
class AdminSession;
class LoginWindow;
class MainWindow;
class NetworkClient;

// 统一管理应用级依赖和登录页/主窗口切换。
class ApplicationController final : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject* parent = nullptr);
    ~ApplicationController() override;
    void start();

private:
    void showLogin(const QString& message = {});
    void showMainWindow();

    NetworkClient* network_ = nullptr;
    AdminSession* session_ = nullptr;
    AdminApiClient* api_ = nullptr;
    LoginWindow* loginWindow_ = nullptr;
    MainWindow* mainWindow_ = nullptr;
};
