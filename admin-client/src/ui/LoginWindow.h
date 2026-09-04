#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;
class QLabel;
class QPushButton;
class AdminApiClient;

// 管理员登录视图：只负责输入和交互反馈，认证流程由 AdminApiClient 处理。
class LoginWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(AdminApiClient* api, QWidget* parent = nullptr);
    void resetForLogin(const QString& message = {});

private:
    void submitLogin();
    void setBusy(bool busy);
    void showMessage(const QString& message, bool error);

    AdminApiClient* api_ = nullptr;
    QLineEdit* accountEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QPushButton* submitButton_ = nullptr;
    QLabel* messageLabel_ = nullptr;
};
