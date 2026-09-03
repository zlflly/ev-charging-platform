#pragma once

#include <QLineEdit>
#include <QPixmap>
#include <QWidget>

class NetworkClient;
class QFrame;
class QLabel;
class QPushButton;
class QVBoxLayout;

// ============================================================================
// 登录页（手机版样式）
//
// 整页由你提供的深色背景图承载标题 + 副标题 + 车辆主视觉；
// 表单卡片按手机画布比例叠加：手机图标 + "请输入手机号"label + 输入框
// （placeholder "11位手机号"）+ "登录/自动注册"按钮；底部为协议文案。
//
// 功能流程沿用 WSL 参考项目：NetworkClient::sendRequest("user.login", ...)
// + Session::setUser(...) + describeError(...)。
// ============================================================================
class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(NetworkClient* networkClient, QWidget* parent = nullptr);

    // 从 QSettings 恢复上次登录手机号；真正的用户信息仍由服务端返回。
    void tryRestoreLogin();
    void clearRememberedLogin();

signals:
    void loginSucceeded();
    void sessionRestored();

private slots:
    void onLoginClicked();
    void onConnected();
    void onTransportError(int transportErrorCode, const QString& message);

private:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    void sendLoginRequest(const QString& phone);
    void finishLoginAttempt();
    void handleLoginResponse(int code, const QString& message,
                             bool isNewUser, const class QJsonObject& userData);
    void showHint(const QString& text, bool isError = true);

    NetworkClient* networkClient_ = nullptr;
    QPixmap backgroundPixmap_;
    QFrame* loginCard_ = nullptr;
    QLineEdit* phoneEdit_ = nullptr;
    QPushButton* loginButton_ = nullptr;
    QLabel* hintLabel_ = nullptr;
    QLabel* footerLabel_ = nullptr;

    bool awaitingLoginResponse_ = false;
    bool restoringSession_ = false;
    QString pendingPhone_;
};
