#include "ui/LoginWindow.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("EV 充电运营中心 - 管理员登录"));
    setMinimumSize(960, 620);

    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(80, 64, 80, 64);
    rootLayout->setSpacing(48);

    auto* intro = new QVBoxLayout;
    intro->setSpacing(12);
    auto* eyebrow = new QLabel(QStringLiteral("EV OPERATIONS CLOUD"), this);
    eyebrow->setObjectName(QStringLiteral("eyebrow"));
    auto* title = new QLabel(QStringLiteral("让每一座充电站\n保持清晰、可靠、可控"), this);
    title->setStyleSheet(QStringLiteral("font-size: 34px; font-weight: 750;"));
    auto* description = new QLabel(
        QStringLiteral("统一查看设备健康、站点运营、用户状态与营收数据。"), this);
    description->setObjectName(QStringLiteral("pageSubtitle"));
    intro->addStretch();
    intro->addWidget(eyebrow);
    intro->addWidget(title);
    intro->addWidget(description);
    intro->addStretch();

    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("card"));
    card->setFixedWidth(390);
    auto* form = new QVBoxLayout(card);
    form->setContentsMargins(34, 34, 34, 34);
    form->setSpacing(14);
    auto* formTitle = new QLabel(QStringLiteral("管理员登录"), card);
    formTitle->setObjectName(QStringLiteral("pageTitle"));
    auto* formHint = new QLabel(QStringLiteral("身份认证将在 Commit 1 接入服务器"), card);
    formHint->setObjectName(QStringLiteral("muted"));
    accountEdit_ = new QLineEdit(card);
    accountEdit_->setPlaceholderText(QStringLiteral("管理员账号"));
    passwordEdit_ = new QLineEdit(card);
    passwordEdit_->setPlaceholderText(QStringLiteral("密码"));
    passwordEdit_->setEchoMode(QLineEdit::Password);
    auto* submit = new QPushButton(QStringLiteral("登录"), card);
    submit->setObjectName(QStringLiteral("primaryButton"));
    connect(submit, &QPushButton::clicked, this, [this] {
        emit loginRequested(accountEdit_->text().trimmed(), passwordEdit_->text());
    });
    form->addWidget(formTitle);
    form->addWidget(formHint);
    form->addSpacing(10);
    form->addWidget(accountEdit_);
    form->addWidget(passwordEdit_);
    form->addWidget(submit);
    form->addStretch();

    rootLayout->addLayout(intro, 1);
    rootLayout->addWidget(card);
}
