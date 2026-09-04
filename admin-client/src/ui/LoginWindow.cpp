#include "ui/LoginWindow.h"

#include "api/AdminApiClient.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace {

class ScaledPixmapLabel final : public QLabel
{
public:
    explicit ScaledPixmapLabel(const QString& resourcePath, QWidget* parent = nullptr)
        : QLabel(parent)
        , source_(resourcePath)
    {
        setAlignment(Qt::AlignCenter);
        setMinimumHeight(250);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        refreshPixmap();
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QLabel::resizeEvent(event);
        refreshPixmap();
    }

private:
    void refreshPixmap()
    {
        if (!source_.isNull() && width() > 0 && height() > 0) {
            setPixmap(source_.scaled(size(), Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation));
        }
    }

    QPixmap source_;
};

QLabel* makeFeatureTag(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral(
        "color:#24527F; background:#EEF5FF; border:1px solid #CFE0F5;"
        "border-radius:10px; padding:7px 12px; font-size:12px;"));
    return label;
}

} // namespace

LoginWindow::LoginWindow(AdminApiClient* api, QWidget* parent)
    : QWidget(parent)
    , api_(api)
{
    Q_ASSERT(api_);
    setWindowTitle(QStringLiteral("EV 充电运营中心 - 管理员登录"));
    setMinimumSize(1080, 680);

    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(52, 42, 52, 42);
    rootLayout->setSpacing(34);

    auto* intro = new QVBoxLayout;
    intro->setSpacing(10);
    auto* eyebrow = new QLabel(QStringLiteral("EV OPERATIONS CLOUD"), this);
    eyebrow->setObjectName(QStringLiteral("eyebrow"));
    auto* title = new QLabel(QStringLiteral("让每一座充电站\n保持清晰、可靠、可控"), this);
    title->setStyleSheet(QStringLiteral(
        "font-size: 36px; font-weight: 750; color: #10233F;"));
    auto* description = new QLabel(
        QStringLiteral("统一查看设备健康、站点运营、用户状态与营收数据。"), this);
    description->setObjectName(QStringLiteral("pageSubtitle"));
    description->setWordWrap(true);

    auto* hero = new ScaledPixmapLabel(
        QStringLiteral(":/resources/admin-login-hero.png"), this);
    auto* featureRow = new QHBoxLayout;
    featureRow->setSpacing(10);
    featureRow->addWidget(makeFeatureTag(QStringLiteral("安全认证"), this));
    featureRow->addWidget(makeFeatureTag(QStringLiteral("实时运营"), this));
    featureRow->addWidget(makeFeatureTag(QStringLiteral("设备管理"), this));
    featureRow->addStretch();

    intro->addStretch(1);
    intro->addWidget(eyebrow);
    intro->addWidget(title);
    intro->addWidget(description);
    intro->addSpacing(10);
    intro->addWidget(hero, 3);
    intro->addLayout(featureRow);
    intro->addStretch(1);

    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("card"));
    card->setFixedWidth(380);
    card->setFixedHeight(370);
    auto* form = new QVBoxLayout(card);
    form->setContentsMargins(34, 34, 34, 34);
    form->setSpacing(14);
    auto* formTitle = new QLabel(QStringLiteral("管理员登录"), card);
    formTitle->setObjectName(QStringLiteral("pageTitle"));
    auto* formHint = new QLabel(QStringLiteral("请输入管理员账号与密码以进入运营后台"), card);
    formHint->setObjectName(QStringLiteral("muted"));
    accountEdit_ = new QLineEdit(card);
    accountEdit_->setPlaceholderText(QStringLiteral("管理员账号"));
    passwordEdit_ = new QLineEdit(card);
    passwordEdit_->setPlaceholderText(QStringLiteral("密码"));
    passwordEdit_->setEchoMode(QLineEdit::Password);
    submitButton_ = new QPushButton(QStringLiteral("登录"), card);
    submitButton_->setObjectName(QStringLiteral("primaryButton"));
    submitButton_->setDefault(true);
    messageLabel_ = new QLabel(card);
    messageLabel_->setWordWrap(true);
    messageLabel_->hide();
    connect(submitButton_, &QPushButton::clicked,
            this, &LoginWindow::submitLogin);
    connect(accountEdit_, &QLineEdit::returnPressed,
            this, &LoginWindow::submitLogin);
    connect(passwordEdit_, &QLineEdit::returnPressed,
            this, &LoginWindow::submitLogin);
    connect(api_, &AdminApiClient::loginSucceeded, this, [this] {
        setBusy(false);
        passwordEdit_->clear();
    });
    connect(api_, &AdminApiClient::loginFailed, this,
            [this](int, const QString& message) {
        setBusy(false);
        showMessage(message, true);
        passwordEdit_->selectAll();
        passwordEdit_->setFocus();
    });
    form->addWidget(formTitle);
    form->addWidget(formHint);
    form->addSpacing(10);
    form->addWidget(accountEdit_);
    form->addWidget(passwordEdit_);
    form->addWidget(submitButton_);
    form->addWidget(messageLabel_);
    form->addStretch();

    rootLayout->addLayout(intro, 1);
    rootLayout->addWidget(card, 0, Qt::AlignVCenter);
}

void LoginWindow::resetForLogin(const QString& message)
{
    setBusy(false);
    passwordEdit_->clear();
    if (message.isEmpty()) {
        messageLabel_->hide();
    } else {
        showMessage(message, false);
    }
    accountEdit_->setFocus();
}

void LoginWindow::submitLogin()
{
    if (api_->isLoginInFlight()) {
        return;
    }

    const QString account = accountEdit_->text().trimmed();
    const QString password = passwordEdit_->text();
    if (account.isEmpty() || password.isEmpty()) {
        showMessage(QStringLiteral("请输入管理员账号和密码"), true);
        (account.isEmpty() ? accountEdit_ : passwordEdit_)->setFocus();
        return;
    }

    messageLabel_->hide();
    setBusy(true);
    if (!api_->login(account, password)) {
        setBusy(false);
    }
}

void LoginWindow::setBusy(bool busy)
{
    accountEdit_->setEnabled(!busy);
    passwordEdit_->setEnabled(!busy);
    submitButton_->setEnabled(!busy);
    submitButton_->setText(busy ? QStringLiteral("正在登录…")
                                : QStringLiteral("登录"));
}

void LoginWindow::showMessage(const QString& message, bool error)
{
    messageLabel_->setText(message);
    messageLabel_->setStyleSheet(error
        ? QStringLiteral("color:#C43742;")
        : QStringLiteral("color:#1769E8;"));
    messageLabel_->show();
}
