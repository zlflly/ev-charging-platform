#pragma once

#include <QList>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QFrame;
class QPaintEvent;
class QResizeEvent;
class NetworkClient;

// 移动端个人中心。资料和余额均以服务端响应为准，页面只负责输入、展示和重试。
class ProfilePage : public QWidget
{
    Q_OBJECT

public:
    explicit ProfilePage(NetworkClient* networkClient, QWidget* parent = nullptr);

    void reload();

signals:
    void backRequested();
    void homeRequested();
    void stationsRequested();
    void chargingRequested();
    void ordersRequested();
    void logoutRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onBackClicked();
    void onSaveNicknameClicked();
    void onAvatarClicked();
    void onRechargeClicked();
    void onOrdersClicked();
    void onLogoutClicked();
    void onSettingsClicked();

private:
    void buildUi();
    void layoutUi();
    void renderSession();
    void setBusy(bool busy, const QString& buttonText = {});
    void showHint(const QString& message, bool error = false);

    NetworkClient* networkClient_ = nullptr;
    QPushButton* backButton_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QFrame* accountCard_ = nullptr;
    QFrame* menuCard_ = nullptr;
    QFrame* supportCard_ = nullptr;
    QLabel* avatarView_ = nullptr;
    QFrame* settingsCard_ = nullptr;
    QLabel* settingsAvatarView_ = nullptr;
    QLabel* nameLabel_ = nullptr;
    QLabel* phoneLabel_ = nullptr;
    QPushButton* avatarButton_ = nullptr;
    QLineEdit* nicknameEdit_ = nullptr;
    QPushButton* saveNicknameButton_ = nullptr;
    QFrame* balanceCard_ = nullptr;
    QLabel* balanceLabel_ = nullptr;
    QLineEdit* rechargeEdit_ = nullptr;
    QPushButton* rechargeButton_ = nullptr;
    QPushButton* ordersButton_ = nullptr;
    QPushButton* favoritesButton_ = nullptr;
    QPushButton* couponButton_ = nullptr;
    QPushButton* settingsButton_ = nullptr;
    QPushButton* helpButton_ = nullptr;
    QPushButton* logoutButton_ = nullptr;
    QLabel* hintLabel_ = nullptr;
    QFrame* bottomBar_ = nullptr;
    QList<QPushButton*> navButtons_;

    bool requestInFlight_ = false;
    bool settingsMode_ = false;
    bool rechargeEditorVisible_ = false;
    QString pendingAvatarDataUrl_;
};
