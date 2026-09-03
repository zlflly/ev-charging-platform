#include "ui/ProfilePage.h"

#include "net/NetworkClient.h"
#include "protocol/Protocol.h"
#include "session/Session.h"
#include "ui/theme/Theme.h"

#include <QBuffer>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFrame>
#include <QIcon>
#include <QImageReader>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QLineEdit>

namespace {

QString cssColor(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

QString cssRgba(const QColor& color, int alpha)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red()).arg(color.green()).arg(color.blue()).arg(alpha);
}

QPixmap tintedSvgPixmap(const QString& resourcePath, const QSize& size,
                        const QColor& color)
{
    const QPixmap source = QIcon(resourcePath).pixmap(size);
    if (source.isNull()) return {};
    QPixmap tinted(size);
    tinted.fill(color);
    QPainter painter(&tinted);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.drawPixmap(0, 0, source);
    return tinted;
}

class BackButton final : public QPushButton
{
public:
    explicit BackButton(QWidget* parent = nullptr) : QPushButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPixmap icon = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/back.svg"), QSize(25, 25),
            theme::textPrimary());
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2,
                               (height() - icon.height()) / 2, icon);
        }
    }
};

class MenuButton final : public QPushButton
{
public:
    MenuButton(const QString& iconPath, const QString& text, bool divider,
               QWidget* parent = nullptr)
        : QPushButton(parent), iconPath_(iconPath), divider_(divider)
    {
        setText(text);
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        if (divider_) {
            painter.setPen(QPen(theme::cardBorder(), 1));
            painter.drawLine(18, height() - 1, width() - 18, height() - 1);
        }

        const QPixmap icon = tintedSvgPixmap(iconPath_, QSize(27, 27),
                                             theme::primaryBlue());
        if (!icon.isNull()) {
            painter.drawPixmap(22, (height() - icon.height()) / 2, icon);
        }

        QFont font(QStringLiteral("Microsoft YaHei UI"));
        font.setPixelSize(18);
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(theme::textPrimary());
        painter.drawText(QRectF(72, 0, width() - 125, height()),
                         Qt::AlignVCenter | Qt::AlignLeft, text());

        const QPixmap arrow = tintedSvgPixmap(
            QStringLiteral(":/resources/icons/chevron.svg"), QSize(18, 18),
            theme::textPrimary());
        if (!arrow.isNull()) {
            painter.drawPixmap(width() - 43, (height() - arrow.height()) / 2, arrow);
        }
    }

private:
    QString iconPath_;
    bool divider_ = false;
};

class ProfileNavButton final : public QPushButton
{
public:
    ProfileNavButton(const QString& iconPath, const QString& label,
                     bool selected, QWidget* parent = nullptr)
        : QPushButton(parent), iconPath_(iconPath), label_(label), selected_(selected)
    {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QColor color = selected_ ? theme::primaryBlue() : theme::textSecondary();
        const QPixmap icon = tintedSvgPixmap(iconPath_, QSize(27, 27), color);
        if (!icon.isNull()) {
            painter.drawPixmap((width() - icon.width()) / 2, 4, icon);
        }
        QFont font(QStringLiteral("Microsoft YaHei UI"));
        font.setPixelSize(13);
        painter.setFont(font);
        painter.setPen(color);
        painter.drawText(QRectF(0, 35, width(), 17), Qt::AlignHCenter, label_);
    }

private:
    QString iconPath_;
    QString label_;
    bool selected_ = false;
};

class AvatarView final : public QLabel
{
public:
    explicit AvatarView(QWidget* parent = nullptr) : QLabel(parent)
    {
        setMinimumSize(1, 1);
    }

    void setAvatarUrl(const QString& avatarUrl, const QString& fallback)
    {
        pixmap_ = QPixmap();
        const int comma = avatarUrl.indexOf(QLatin1Char(','));
        if (comma > 0) {
            const QByteArray bytes = QByteArray::fromBase64(
                avatarUrl.mid(comma + 1).toUtf8());
            pixmap_.loadFromData(bytes);
        } else if (avatarUrl.startsWith(QStringLiteral("qrc:/")) ||
                   avatarUrl.startsWith(QStringLiteral(":/"))) {
            pixmap_.load(avatarUrl);
        }
        fallback_ = fallback.isEmpty() ? QStringLiteral("用") : fallback.left(1);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(theme::chipBg());
        painter.setPen(QPen(theme::primaryBlue(), 1));
        painter.drawEllipse(rect().adjusted(1, 1, -1, -1));

        if (!pixmap_.isNull()) {
            QPainterPath clip;
            clip.addEllipse(rect().adjusted(2, 2, -2, -2));
            painter.save();
            painter.setClipPath(clip);
            const QPixmap scaled = pixmap_.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation);
            painter.drawPixmap((width() - scaled.width()) / 2,
                               (height() - scaled.height()) / 2, scaled);
            painter.restore();
            return;
        }

        painter.setPen(theme::textPrimary());
        QFont font(QStringLiteral("Microsoft YaHei UI"));
        font.setPixelSize(qMax(18, width() / 3));
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, fallback_);
    }

private:
    QPixmap pixmap_;
    QString fallback_;
};

QString fieldStyle()
{
    return QString(
        "QLineEdit { background: %1; border: 1px solid %2; border-radius: 10px;"
        " padding: 0 11px; color: %3; selection-background-color: %4; }"
        "QLineEdit:focus { border-color: %4; }"
        "QLineEdit::placeholder { color: %5; }")
        .arg(cssColor(theme::inputFill()), cssColor(theme::inputBorder()),
             cssColor(theme::textPrimary()), cssColor(theme::primaryBlue()),
             cssColor(theme::textMuted()));
}

QString buttonStyle(bool primary = false)
{
    return primary
        ? QString(
            "QPushButton { background: %1; color: white; border: 1px solid %2;"
            " border-radius: 11px; padding: 0 12px; }"
            "QPushButton:hover { background: %3; }"
            "QPushButton:pressed { background: %4; }"
            "QPushButton:disabled { background: %5; color: %6; border-color: transparent; }")
            .arg(cssColor(theme::primaryBlue()), cssColor(theme::primaryBlueHover()),
                 cssColor(theme::primaryBlueHover()), cssColor(theme::primaryBluePressed()),
                 cssColor(QColor(0x12, 0x33, 0x66)), cssColor(theme::textMuted()))
        : QString(
            "QPushButton { background: %1; color: %2; border: 1px solid %3;"
            " border-radius: 12px; padding: 0 14px; text-align: left; }"
            "QPushButton:hover { border-color: %4; background: %5; }")
            .arg(cssRgba(theme::cardFill(), 220), cssColor(theme::textPrimary()),
                 cssColor(theme::cardBorder()), cssColor(theme::primaryBlue()),
                 cssRgba(theme::primaryBlue(), 22));
}

} // namespace

ProfilePage::ProfilePage(NetworkClient* networkClient, QWidget* parent)
    : QWidget(parent)
    , networkClient_(networkClient)
{
    setObjectName(QStringLiteral("ProfilePage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#ProfilePage { background-color: %1; }")
                      .arg(cssColor(theme::background())));
    setMinimumSize(360, theme::loginCanvasHeight);
    buildUi();
    reload();
}

void ProfilePage::buildUi()
{
    backButton_ = new BackButton(this);
    titleLabel_ = new QLabel(QStringLiteral("设置"), this);
    titleLabel_->setFont(theme::buttonFont());
    titleLabel_->setAlignment(Qt::AlignCenter);
    titleLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                   .arg(cssColor(theme::textPrimary())));

    accountCard_ = new QFrame(this);
    accountCard_->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    avatarView_ = new AvatarView(accountCard_);
    nameLabel_ = new QLabel(accountCard_);
    nameLabel_->setFont(theme::buttonFont());
    nameLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                  .arg(cssColor(theme::textPrimary())));
    phoneLabel_ = new QLabel(accountCard_);
    phoneLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                   .arg(cssColor(theme::textSecondary())));

    balanceCard_ = new QFrame(this);
    balanceCard_->setStyleSheet(QStringLiteral(
        "QFrame { background: %1; border: 1px solid %2; border-radius: 17px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));
    auto* balanceTitle = new QLabel(QStringLiteral("账户余额（元）"), balanceCard_);
    balanceTitle->setObjectName(QStringLiteral("BalanceTitle"));
    balanceTitle->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                    .arg(cssColor(theme::textSecondary())));
    balanceLabel_ = new QLabel(balanceCard_);
    balanceLabel_->setObjectName(QStringLiteral("BalanceValue"));
    balanceLabel_->setFont(theme::buttonFont());
    balanceLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                     .arg(cssColor(theme::textPrimary())));
    rechargeEdit_ = new QLineEdit(balanceCard_);
    rechargeEdit_->setPlaceholderText(QStringLiteral("充值金额"));
    rechargeEdit_->setValidator(new QDoubleValidator(0.01, 100000.0, 2, rechargeEdit_));
    rechargeEdit_->setFont(theme::footerFont());
    rechargeEdit_->setStyleSheet(fieldStyle());
    rechargeEdit_->setVisible(false);
    rechargeButton_ = new QPushButton(QStringLiteral("去充值"), balanceCard_);
    rechargeButton_->setFont(theme::buttonFont());
    rechargeButton_->setStyleSheet(buttonStyle(true));

    menuCard_ = new QFrame(this);
    menuCard_->setStyleSheet(QStringLiteral(
        "QFrame { background: %1; border: 1px solid %2; border-radius: 17px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));
    ordersButton_ = new MenuButton(QStringLiteral(":/resources/icons/order.svg"),
                                   QStringLiteral("我的订单"), true, menuCard_);
    favoritesButton_ = new MenuButton(QStringLiteral(":/resources/icons/favorite.svg"),
                                      QStringLiteral("我的收藏"), true, menuCard_);
    couponButton_ = new MenuButton(QStringLiteral(":/resources/icons/coupon.svg"),
                                   QStringLiteral("我的优惠券"), false, menuCard_);

    supportCard_ = new QFrame(this);
    supportCard_->setStyleSheet(QStringLiteral(
        "QFrame { background: %1; border: 1px solid %2; border-radius: 17px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));
    settingsButton_ = new MenuButton(QStringLiteral(":/resources/icons/settings.svg"),
                                     QStringLiteral("设置"), true, supportCard_);
    helpButton_ = new MenuButton(QStringLiteral(":/resources/icons/phone.svg"),
                                 QStringLiteral("帮助"), false, supportCard_);

    logoutButton_ = new QPushButton(QStringLiteral("退出登录"), this);
    logoutButton_->setFont(theme::footerFont());
    logoutButton_->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: 1px solid %2;"
        " border-radius: 12px; } QPushButton:hover { border-color: %3; color: %3; }")
        .arg(cssColor(theme::danger()), cssColor(theme::cardBorder()),
             cssColor(theme::danger())));

    settingsCard_ = new QFrame(this);
    settingsCard_->setStyleSheet(QStringLiteral(
        "QFrame { background: %1; border: 1px solid %2; border-radius: 17px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));
    settingsAvatarView_ = new AvatarView(settingsCard_);
    avatarButton_ = new QPushButton(QStringLiteral("更换头像"), settingsCard_);
    avatarButton_->setFont(theme::footerFont());
    avatarButton_->setStyleSheet(buttonStyle());
    nicknameEdit_ = new QLineEdit(settingsCard_);
    nicknameEdit_->setPlaceholderText(QStringLiteral("输入昵称（最多20字）"));
    nicknameEdit_->setMaxLength(20);
    nicknameEdit_->setFont(theme::footerFont());
    nicknameEdit_->setStyleSheet(fieldStyle());
    saveNicknameButton_ = new QPushButton(QStringLiteral("保存"), settingsCard_);
    saveNicknameButton_->setFont(theme::footerFont());
    saveNicknameButton_->setStyleSheet(buttonStyle(true));

    hintLabel_ = new QLabel(this);
    hintLabel_->setAlignment(Qt::AlignCenter);
    hintLabel_->setWordWrap(true);
    hintLabel_->setFont(theme::footerFont());
    hintLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                  .arg(cssColor(theme::textSecondary())));

    bottomBar_ = new QFrame(this);
    bottomBar_->setStyleSheet(QStringLiteral(
        "QFrame { background: %1; border: 1px solid %2; border-radius: 17px; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder())));
    const QList<QPair<QString, QString>> navItems = {
        {QStringLiteral(":/resources/icons/home.svg"), QStringLiteral("首页")},
        {QStringLiteral(":/resources/icons/station.svg"), QStringLiteral("站点")},
        {QStringLiteral(":/resources/icons/charge.svg"), QStringLiteral("充电")},
        {QStringLiteral(":/resources/icons/order.svg"), QStringLiteral("订单")},
        {QStringLiteral(":/resources/icons/profile.svg"), QStringLiteral("我的")},
    };
    for (int index = 0; index < navItems.size(); ++index) {
        navButtons_.append(new ProfileNavButton(navItems[index].first,
                                                  navItems[index].second,
                                                  index == navItems.size() - 1,
                                                  bottomBar_));
    }

    settingsCard_->setVisible(false);
    backButton_->setVisible(false);
    titleLabel_->setVisible(false);
    logoutButton_->setVisible(false);

    connect(backButton_, &QPushButton::clicked, this, &ProfilePage::onBackClicked);
    connect(avatarButton_, &QPushButton::clicked, this, &ProfilePage::onAvatarClicked);
    connect(saveNicknameButton_, &QPushButton::clicked,
            this, &ProfilePage::onSaveNicknameClicked);
    connect(rechargeButton_, &QPushButton::clicked,
            this, &ProfilePage::onRechargeClicked);
    connect(ordersButton_, &QPushButton::clicked, this, &ProfilePage::onOrdersClicked);
    connect(favoritesButton_, &QPushButton::clicked, this, [this] {
        showHint(QStringLiteral("收藏的站点会在站点详情中保留"));
    });
    connect(couponButton_, &QPushButton::clicked, this, [this] {
        showHint(QStringLiteral("暂无可用优惠券"));
    });
    connect(settingsButton_, &QPushButton::clicked,
            this, &ProfilePage::onSettingsClicked);
    connect(helpButton_, &QPushButton::clicked, this, [this] {
        showHint(QStringLiteral("我的手机号：19129588260"));
    });
    connect(logoutButton_, &QPushButton::clicked, this, &ProfilePage::onLogoutClicked);

    connect(navButtons_[0], &QPushButton::clicked, this, &ProfilePage::homeRequested);
    connect(navButtons_[1], &QPushButton::clicked, this, &ProfilePage::stationsRequested);
    connect(navButtons_[2], &QPushButton::clicked, this, &ProfilePage::chargingRequested);
    connect(navButtons_[3], &QPushButton::clicked, this, &ProfilePage::ordersRequested);
}

void ProfilePage::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), theme::background());
}

void ProfilePage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutUi();
}

void ProfilePage::layoutUi()
{
    const qreal sx = static_cast<qreal>(width()) / theme::loginCanvasWidth;
    const qreal sy = static_cast<qreal>(height()) / theme::loginCanvasHeight;
    auto X = [sx](qreal value) { return qRound(value * sx); };
    auto Y = [sy](qreal value) { return qRound(value * sy); };
    const int side = X(24);
    const int contentWidth = width() - side * 2;

    backButton_->setGeometry(X(15), Y(14), X(36), Y(34));
    titleLabel_->setGeometry(X(110), Y(14), X(250), Y(34));
    accountCard_->setGeometry(side, Y(15), contentWidth, Y(112));
    avatarView_->setGeometry(X(6), Y(12), X(92), Y(92));
    nameLabel_->setGeometry(X(128), Y(24), X(220), Y(34));
    phoneLabel_->setGeometry(X(128), Y(67), X(220), Y(24));

    balanceCard_->setGeometry(side, Y(143), contentWidth, Y(128));
    if (QLabel* label = balanceCard_->findChild<QLabel*>(QStringLiteral("BalanceTitle"))) {
        label->setGeometry(X(25), Y(18), X(180), Y(26));
    }
    balanceLabel_->setGeometry(X(25), Y(52), X(210), Y(53));
    rechargeEdit_->setGeometry(X(214), Y(79), X(104), Y(34));
    rechargeButton_->setGeometry(X(326), Y(39), X(105), Y(46));

    menuCard_->setGeometry(side, Y(289), contentWidth, Y(204));
    ordersButton_->setGeometry(0, 0, contentWidth, Y(68));
    favoritesButton_->setGeometry(0, Y(68), contentWidth, Y(68));
    couponButton_->setGeometry(0, Y(136), contentWidth, Y(68));

    supportCard_->setGeometry(side, Y(509), contentWidth, Y(136));
    settingsButton_->setGeometry(0, 0, contentWidth, Y(68));
    helpButton_->setGeometry(0, Y(68), contentWidth, Y(68));

    hintLabel_->setGeometry(side, Y(653), contentWidth, Y(29));
    bottomBar_->setGeometry(side, Y(692), contentWidth, Y(80));
    const int navWidth = bottomBar_->width() / navButtons_.size();
    for (int i = 0; i < navButtons_.size(); ++i) {
        navButtons_[i]->setGeometry(i * navWidth, 0,
                                    i == navButtons_.size() - 1
                                        ? bottomBar_->width() - i * navWidth : navWidth,
                                    bottomBar_->height());
    }

    settingsCard_->setGeometry(side, Y(78), contentWidth, Y(282));
    settingsAvatarView_->setGeometry(X(28), Y(28), X(92), Y(92));
    avatarButton_->setGeometry(X(143), Y(38), X(116), Y(38));
    nicknameEdit_->setGeometry(X(143), Y(91), X(182), Y(38));
    saveNicknameButton_->setGeometry(X(333), Y(91), X(72), Y(38));
    logoutButton_->setGeometry(side, Y(384), contentWidth, Y(44));

    const bool rootVisible = !settingsMode_;
    accountCard_->setVisible(rootVisible);
    balanceCard_->setVisible(rootVisible);
    menuCard_->setVisible(rootVisible);
    supportCard_->setVisible(rootVisible);
    bottomBar_->setVisible(rootVisible);
    settingsCard_->setVisible(settingsMode_);
    backButton_->setVisible(settingsMode_);
    titleLabel_->setVisible(settingsMode_);
    logoutButton_->setVisible(settingsMode_);
    if (settingsMode_) {
        hintLabel_->setGeometry(side, Y(382), contentWidth, Y(60));
    }
}

void ProfilePage::reload()
{
    renderSession();
    hintLabel_->clear();
    hintLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                  .arg(cssColor(theme::textSecondary())));
}

void ProfilePage::renderSession()
{
    const Session& session = Session::instance();
    const QString nickname = session.nickname().isEmpty()
        ? QStringLiteral("未设置昵称") : session.nickname();
    QString displayPhone = session.phone();
    if (displayPhone.size() >= 7) {
        displayPhone = displayPhone.left(3) + QStringLiteral("****")
            + displayPhone.right(4);
    }
    nameLabel_->setText(nickname);
    phoneLabel_->setText(displayPhone.isEmpty() ? QStringLiteral("未登录") : displayPhone);
    nicknameEdit_->setText(session.nickname());
    balanceLabel_->setText(QStringLiteral("¥%1").arg(session.balance(), 0, 'f', 2));
    const QString fallback = nickname.left(1);
    static_cast<AvatarView*>(avatarView_)->setAvatarUrl(session.avatarUrl(), fallback);
    static_cast<AvatarView*>(settingsAvatarView_)->setAvatarUrl(session.avatarUrl(), fallback);
}

void ProfilePage::setBusy(bool busy, const QString& buttonText)
{
    requestInFlight_ = busy;
    avatarButton_->setEnabled(!busy);
    saveNicknameButton_->setEnabled(!busy);
    rechargeButton_->setEnabled(!busy);
    ordersButton_->setEnabled(!busy);
    favoritesButton_->setEnabled(!busy);
    couponButton_->setEnabled(!busy);
    settingsButton_->setEnabled(!busy);
    helpButton_->setEnabled(!busy);
    logoutButton_->setEnabled(!busy);
    if (!buttonText.isEmpty()) {
        if (sender() == saveNicknameButton_) saveNicknameButton_->setText(buttonText);
        if (sender() == rechargeButton_) rechargeButton_->setText(buttonText);
    }
    if (!busy) {
        saveNicknameButton_->setText(QStringLiteral("保存"));
        rechargeButton_->setText(rechargeEditorVisible_
                                     ? QStringLiteral("确认充值") : QStringLiteral("去充值"));
    }
}

void ProfilePage::showHint(const QString& message, bool error)
{
    hintLabel_->setText(message);
    hintLabel_->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                  .arg(cssColor(error ? theme::danger()
                                                      : theme::textSecondary())));
}

void ProfilePage::onSaveNicknameClicked()
{
    if (requestInFlight_) return;
    const QString nickname = nicknameEdit_->text().trimmed();
    if (nickname.isEmpty()) {
        showHint(QStringLiteral("昵称不能为空"), true);
        return;
    }
    if (!Session::instance().isLoggedIn() || !networkClient_) {
        showHint(QStringLiteral("登录状态已失效，请重新登录"), true);
        return;
    }
    setBusy(true, QStringLiteral("保存中"));
    QJsonObject data;
    data.insert(QStringLiteral("nickname"), nickname);
    networkClient_->sendRequest(QString::fromUtf8(protocol::action::kUserProfileUpdate),
                                data, [this, nickname](const protocol::Response& response) {
        setBusy(false);
        if (!response.isOk()) {
            showHint(protocol::describeError(response.code, response.message), true);
            return;
        }
        Session::instance().setNickname(response.data
            .value(QStringLiteral("nickname")).toString(nickname));
        renderSession();
        showHint(QStringLiteral("昵称已保存"));
    });
}

void ProfilePage::onBackClicked()
{
    if (!settingsMode_) {
        emit backRequested();
        return;
    }
    settingsMode_ = false;
    hintLabel_->clear();
    layoutUi();
}

void ProfilePage::onSettingsClicked()
{
    if (requestInFlight_) return;
    settingsMode_ = true;
    hintLabel_->clear();
    layoutUi();
}

void ProfilePage::onAvatarClicked()
{
    if (requestInFlight_) return;
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择头像"), QString(),
        QStringLiteral("图片文件 (*.png *.jpg *.jpeg)"));
    if (path.isEmpty()) return;

    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        showHint(QStringLiteral("头像读取失败，请选择 PNG 或 JPG 图片"), true);
        return;
    }
    if (image.width() > 160 || image.height() > 160) {
        image = image.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        showHint(QStringLiteral("头像处理失败，请重试"), true);
        return;
    }
    pendingAvatarDataUrl_ = QStringLiteral("data:image/png;base64,%1")
        .arg(QString::fromLatin1(bytes.toBase64()));
    if (!Session::instance().isLoggedIn() || !networkClient_) {
        showHint(QStringLiteral("登录状态已失效，请重新登录"), true);
        return;
    }
    setBusy(true, QStringLiteral("上传中"));
    QJsonObject data;
    data.insert(QStringLiteral("avatarData"), QString::fromLatin1(bytes.toBase64()));
    data.insert(QStringLiteral("avatarMime"), QStringLiteral("png"));
    networkClient_->sendRequest(QString::fromUtf8(protocol::action::kUserProfileUpdate),
                                data, [this](const protocol::Response& response) {
        setBusy(false);
        if (!response.isOk()) {
            showHint(protocol::describeError(response.code, response.message), true);
            return;
        }
        Session::instance().setAvatarUrl(pendingAvatarDataUrl_);
        pendingAvatarDataUrl_.clear();
        renderSession();
        showHint(QStringLiteral("头像已更新"));
    });
}

void ProfilePage::onRechargeClicked()
{
    if (requestInFlight_) return;
    if (!rechargeEditorVisible_) {
        rechargeEditorVisible_ = true;
        rechargeEdit_->setVisible(true);
        rechargeButton_->setText(QStringLiteral("确认充值"));
        rechargeEdit_->setFocus();
        return;
    }
    bool ok = false;
    const double amount = rechargeEdit_->text().trimmed().toDouble(&ok);
    if (!ok || amount <= 0.0 || amount > 100000.0) {
        showHint(QStringLiteral("请输入 0.01 至 100000 元的充值金额"), true);
        return;
    }
    if (!Session::instance().isLoggedIn() || !networkClient_) {
        showHint(QStringLiteral("登录状态已失效，请重新登录"), true);
        return;
    }
    setBusy(true, QStringLiteral("充值中"));
    QJsonObject data;
    data.insert(QStringLiteral("amount"), amount);
    networkClient_->sendRequest(QString::fromUtf8(protocol::action::kUserRecharge),
                                data, [this](const protocol::Response& response) {
        setBusy(false);
        if (!response.isOk()) {
            showHint(protocol::describeError(response.code, response.message), true);
            return;
        }
        if (!response.data.contains(QStringLiteral("balance"))) {
            showHint(QStringLiteral("充值响应缺少余额，请刷新后确认"), true);
            return;
        }
        Session::instance().setBalance(response.data
            .value(QStringLiteral("balance")).toDouble());
        rechargeEdit_->clear();
        rechargeEditorVisible_ = false;
        rechargeEdit_->setVisible(false);
        renderSession();
        showHint(QStringLiteral("充值成功，余额已更新"));
    });
}

void ProfilePage::onOrdersClicked()
{
    emit ordersRequested();
}

void ProfilePage::onLogoutClicked()
{
    if (requestInFlight_) return;
    emit logoutRequested();
}
