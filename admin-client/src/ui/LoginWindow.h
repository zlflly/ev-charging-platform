#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;

// Commit 0 只建立登录界面容器；认证与会话在 Commit 1 对接服务器。
class LoginWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);

signals:
    void loginRequested(const QString& account, const QString& password);

private:
    QLineEdit* accountEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
};
