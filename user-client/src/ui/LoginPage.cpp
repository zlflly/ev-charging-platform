#include "ui/LoginPage.h"

#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/Session.h"
#include "ui/SvgIcon.h"
#include "ui/theme/Theme.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// 中国大陆手机号：1 开头的 11 位数字（本地预校验，服务端仍是最终裁决）
const QRegularExpression kPhonePattern(QStringLiteral("^1\\d{10}$"));

QString cssColor(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

QPixmap tintedSvgPixmap(const QString& resourcePath, const QSize& size,
                        const QColor& color)
{
    return svg::tintedPixmap(resourcePath, size, color);
}

// 自绘手机 icon，避免使用 emoji 或平台私有图标导致视觉不一致。
class PhoneChip : public QLabel {
public:
    explicit PhoneChip(QWidget* parent = nullptr) : QLabel(parent)
    {
        setFixedSize(40, 40);
        setAlignment(Qt::AlignCenter);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QPixmap icon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/phone.svg"), QSize(25, 25),
            theme::primaryBlue());
        if (!icon.isNull()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(theme::chipBg());
            painter.drawEllipse(rect());
            painter.drawPixmap((width() - icon.width()) / 2,
                               (height() - icon.height()) / 2, icon);
            return;
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(theme::chipBg());
        painter.drawEllipse(rect());

        const QRectF body(12, 8, 16, 24);
        painter.setBrush(theme::primaryBlue());
        painter.drawRoundedRect(body, 3, 3);
        painter.setBrush(theme::chipBg());
        painter.drawRoundedRect(body.adjusted(2, 3, -2, -4), 1.5, 1.5);
        painter.setBrush(theme::primaryBlue());
        painter.drawEllipse(QPointF(body.center().x(), body.bottom() - 2.5), 1.3, 1.3);
    }
};

} // namespace

LoginPage::LoginPage(NetworkClient* networkClient, QWidget* parent)
    : QWidget(parent)
    , networkClient_(networkClient)
    , backgroundPixmap_(QStringLiteral(":/resources/images/login_bg.png"))
{
    setObjectName(QStringLiteral("LoginPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "#LoginPage { background-color: %1; }").arg(cssColor(theme::background())));
    // 卡片内容的最小高度约 278px，保证缩小窗口时核心按钮不会被裁掉。
    setMinimumSize(360, theme::loginCanvasHeight);

    // ---- 表单卡片：直接叠在整页背景上，坐标按手机画布比例布局 ----
    loginCard_ = new QFrame(this);
    loginCard_->setObjectName(QStringLiteral("LoginCard"));
    loginCard_->setStyleSheet(QString(
        "#LoginCard {"
        " background-color: %1;"
        " border: 1px solid %2;"
        " border-radius: %3px;"
        " }")
        .arg(cssColor(theme::cardFill()),
             cssColor(theme::cardBorder()))
        .arg(theme::cardRadius()));

    auto* cardLayout = new QVBoxLayout(loginCard_);
    cardLayout->setContentsMargins(20, 24, 20, 24);
    cardLayout->setSpacing(0);

    auto* labelRow = new QHBoxLayout();
    labelRow->setSpacing(10);
    labelRow->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* chip = new PhoneChip(loginCard_);
    auto* label = new QLabel(QStringLiteral("请输入手机号"), loginCard_);
    label->setFont(theme::fieldLabelFont());
    label->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textPrimary())));
    labelRow->addWidget(chip);
    labelRow->addWidget(label);
    labelRow->addStretch(1);
    cardLayout->addLayout(labelRow);
    cardLayout->addSpacing(16);

    phoneEdit_ = new QLineEdit(loginCard_);
    phoneEdit_->setObjectName(QStringLiteral("PhoneInput"));
    phoneEdit_->setPlaceholderText(QStringLiteral("11位手机号"));
    phoneEdit_->setFont(theme::inputFont());
    phoneEdit_->setMaxLength(11);
    phoneEdit_->setMinimumHeight(66);
    phoneEdit_->setFocusPolicy(Qt::ClickFocus);
    phoneEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
    phoneEdit_->setStyleSheet(QString(
        "QLineEdit#PhoneInput {"
        " background-color: %1;"
        " border: 1px solid %2;"
        " border-radius: %3px;"
        " padding: 0 18px;"
        " color: %4;"
        " selection-background-color: %5;"
        " }"
        "QLineEdit#PhoneInput:focus { border: 1px solid %5; }"
        "QLineEdit#PhoneInput::placeholder { color: %6; }")
        .arg(cssColor(theme::inputFill()),
             cssColor(theme::inputBorder()))
        .arg(theme::inputRadius())
        .arg(cssColor(theme::textPrimary()),
             cssColor(theme::primaryBlue()),
             cssColor(theme::textMuted())));
    cardLayout->addWidget(phoneEdit_);
    cardLayout->addSpacing(28);

    loginButton_ = new QPushButton(QStringLiteral("登录 / 自动注册"), loginCard_);
    loginButton_->setObjectName(QStringLiteral("PrimaryButton"));
    loginButton_->setCursor(Qt::PointingHandCursor);
    loginButton_->setFocusPolicy(Qt::NoFocus);
    loginButton_->setMinimumHeight(70);
    loginButton_->setFont(theme::buttonFont());
    loginButton_->setStyleSheet(QString(
        "QPushButton#PrimaryButton {"
        " background-color: %1;"
        " color: white;"
        " border: 1px solid %2;"
        " border-radius: %3px;"
        " }"
        "QPushButton#PrimaryButton:hover  { background-color: %4; }"
        "QPushButton#PrimaryButton:pressed{ background-color: %5; }"
        "QPushButton#PrimaryButton:disabled {"
        " background-color: %6; color: %7; border-color: transparent; }")
        .arg(cssColor(theme::primaryBlue()),
             cssColor(QColor(0x54, 0xAE, 0xFF)))
        .arg(theme::buttonRadius())
        .arg(cssColor(theme::primaryBlueHover()),
             cssColor(theme::primaryBluePressed()),
             cssColor(QColor(0x12, 0x33, 0x66)),
             cssColor(theme::textMuted())));

    auto* glow = new QGraphicsDropShadowEffect(loginButton_);
    glow->setBlurRadius(22);
    glow->setOffset(0, 5);
    glow->setColor(QColor(theme::primaryBlue().red(),
                          theme::primaryBlue().green(),
                          theme::primaryBlue().blue(), 105));
    loginButton_->setGraphicsEffect(glow);
    cardLayout->addWidget(loginButton_);

    hintLabel_ = new QLabel(loginCard_);
    hintLabel_->setWordWrap(true);
    hintLabel_->setAlignment(Qt::AlignCenter);
    hintLabel_->hide();
    cardLayout->addSpacing(8);
    cardLayout->addWidget(hintLabel_);

    // ---- 页脚 ----
    footerLabel_ = new QLabel(this);
    footerLabel_->setAlignment(Qt::AlignCenter);
    footerLabel_->setFont(theme::footerFont());
    footerLabel_->setTextFormat(Qt::RichText);
    footerLabel_->setText(QStringLiteral(
        "<span style='color:%1;'>登录即代表同意</span> "
        "<span style='color:%2;'>《用户协议》</span>"
        "<span style='color:%1;'> 和 </span>"
        "<span style='color:%2;'>《隐私政策》</span>")
        .arg(cssColor(theme::textMuted()), cssColor(theme::linkBlue())));
    footerLabel_->setStyleSheet(QStringLiteral("background: transparent;"));

    setFocusPolicy(Qt::StrongFocus);

    connect(loginButton_, &QPushButton::clicked,
            this, &LoginPage::onLoginClicked);
    connect(phoneEdit_, &QLineEdit::returnPressed,
            this, &LoginPage::onLoginClicked);
    connect(networkClient_, &NetworkClient::connected,
            this, &LoginPage::onConnected);
    connect(networkClient_, &NetworkClient::transportError,
            this, &LoginPage::onTransportError);

    QTimer::singleShot(0, this, [this] { setFocus(); });
    update();
}

void LoginPage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), theme::background());

    if (!backgroundPixmap_.isNull()) {
        const QPixmap scaled = backgroundPixmap_.scaled(
            size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int x = (width() - scaled.width()) / 2;
        const int y = (height() - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
    }
}

void LoginPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    const qreal scaleY = static_cast<qreal>(height()) / theme::loginCanvasHeight;
    const int inset = qMax(theme::loginCardHorizontalInset,
                           qRound(theme::loginCardHorizontalInset
                                  * static_cast<qreal>(width())
                                  / theme::loginCanvasWidth));
    const int cardTop = qRound(theme::loginCardTop * scaleY);
    const int cardHeight = qRound(theme::loginCardHeight * scaleY);
    loginCard_->setGeometry(inset, cardTop, width() - inset * 2, cardHeight);

    const int footerTop = qRound(theme::loginFooterTop * scaleY);
    footerLabel_->setGeometry(20, footerTop, qMax(0, width() - 40),
                              qRound(32 * scaleY));
}

void LoginPage::onLoginClicked()
{
    if (awaitingLoginResponse_) {
        return;
    }
    const QString phone = phoneEdit_->text().trimmed();
    if (!kPhonePattern.match(phone).hasMatch()) {
        showHint(QStringLiteral("请输入 1 开头的 11 位手机号"));
        return;
    }
    restoringSession_ = false;
    hintLabel_->hide();
    awaitingLoginResponse_ = true;
    loginButton_->setEnabled(false);
    loginButton_->setText(QStringLiteral("登录中..."));

    if (!networkClient_->isConnected()) {
        pendingPhone_ = phone;
        networkClient_->connectToServer(
            QString::fromUtf8(protocol::kDefaultHost), protocol::kDefaultPort);
        return;
    }
    sendLoginRequest(phone);
}

void LoginPage::onConnected()
{
    if (!pendingPhone_.isEmpty()) {
        const QString phone = pendingPhone_;
        pendingPhone_.clear();
        sendLoginRequest(phone);
        return;
    }

    // 连接断开后 NetworkClient 会自动重连；重连成功时重新登录只读的
    // Session 手机号，恢复服务端 TCP 会话，但不把用户带回登录页。
    if (Session::instance().isLoggedIn() && !awaitingLoginResponse_) {
        restoringSession_ = true;
        awaitingLoginResponse_ = true;
        loginButton_->setEnabled(false);
        loginButton_->setText(QStringLiteral("恢复登录中..."));
        sendLoginRequest(Session::instance().phone());
    }
}

void LoginPage::onTransportError(int transportErrorCode,
                                 const QString& message)
{
    if (pendingPhone_.isEmpty() && !restoringSession_) {
        return;
    }
    pendingPhone_.clear();
    const bool wasRestoring = restoringSession_;
    restoringSession_ = false;
    finishLoginAttempt();
    showHint(wasRestoring
                 ? QStringLiteral("登录状态已失效，请重新登录")
                 : protocol::describeError(transportErrorCode, message));
}

void LoginPage::sendLoginRequest(const QString& phone)
{
    QJsonObject data;
    data.insert(QStringLiteral("phone"), phone);
    networkClient_->sendRequest(QStringLiteral("user.login"), data,
        [this](const protocol::Response& response) {
            finishLoginAttempt();
            handleLoginResponse(response.code, response.message,
                                response.data.value(QStringLiteral("isNewUser")).toBool(),
                                response.data);
        });
}

void LoginPage::finishLoginAttempt()
{
    awaitingLoginResponse_ = false;
    loginButton_->setEnabled(true);
    loginButton_->setText(QStringLiteral("登录 / 自动注册"));
}

void LoginPage::handleLoginResponse(int code, const QString& message,
                                    bool isNewUser, const QJsonObject& userData)
{
    const bool wasRestoring = restoringSession_;
    restoringSession_ = false;
    if (code != protocol::CodeOk) {
        showHint(protocol::describeError(code, message));
        return;
    }
    const qint64 userId =
        static_cast<qint64>(userData.value(QStringLiteral("userId")).toDouble());
    if (userId <= 0) {
        showHint(QStringLiteral("登录响应异常（用户信息缺失），请稍后重试。"));
        return;
    }
    const QString status = userData.value(QStringLiteral("status")).toString();
    if (!status.isEmpty() && status != QStringLiteral("ACTIVE")) {
        showHint(QStringLiteral("该账号当前不可用，请联系客服处理。"));
        return;
    }
    const QString phone = userData.value(QStringLiteral("phone")).toString().isEmpty()
        ? phoneEdit_->text().trimmed()
        : userData.value(QStringLiteral("phone")).toString();
    Session::instance().setUser(
        userId,
        phone,
        userData.value(QStringLiteral("nickname")).toString(),
        userData.value(QStringLiteral("avatarUrl")).toString(),
        userData.value(QStringLiteral("balance")).toDouble(),
        userData.value(QStringLiteral("status")).toString());

    QSettings settings;
    settings.setValue(QStringLiteral("auth/phone"), phone);
    settings.setValue(QStringLiteral("auth/remember"), true);
    Q_UNUSED(isNewUser);
    if (wasRestoring) {
        emit sessionRestored();
    } else {
        emit loginSucceeded();
    }
}

void LoginPage::tryRestoreLogin()
{
    if (!networkClient_ || Session::instance().isLoggedIn()) {
        return;
    }
    QSettings settings;
    if (!settings.value(QStringLiteral("auth/remember"), false).toBool()) {
        return;
    }
    const QString phone = settings.value(QStringLiteral("auth/phone")).toString().trimmed();
    if (!kPhonePattern.match(phone).hasMatch()) {
        clearRememberedLogin();
        return;
    }
    phoneEdit_->setText(phone);
    hintLabel_->hide();
    restoringSession_ = true;
    awaitingLoginResponse_ = true;
    loginButton_->setEnabled(false);
    loginButton_->setText(QStringLiteral("恢复登录中..."));
    if (networkClient_->isConnected()) {
        sendLoginRequest(phone);
    } else {
        pendingPhone_ = phone;
        networkClient_->connectToServer(
            QString::fromUtf8(protocol::kDefaultHost), protocol::kDefaultPort);
    }
}

void LoginPage::clearRememberedLogin()
{
    QSettings settings;
    settings.remove(QStringLiteral("auth/phone"));
    settings.remove(QStringLiteral("auth/remember"));
    restoringSession_ = false;
    pendingPhone_.clear();
}

void LoginPage::showHint(const QString& text, bool isError)
{
    hintLabel_->setText(text);
    hintLabel_->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent; font-size: 13px;")
        .arg(cssColor(isError ? theme::danger() : theme::textSecondary())));
    hintLabel_->show();
}
