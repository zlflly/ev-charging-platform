#include "ui/ChargerCard.h"

#include "ui/theme/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>

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

QColor statusColor(int status)
{
    switch (status) {
    case protocol::ChargerStatusIdle:     return theme::success();
    case protocol::ChargerStatusCharging: return theme::priceAmber();
    case protocol::ChargerStatusFault:    return theme::danger();
    default:                              return theme::textSecondary();
    }
}

} // namespace

ChargerCard::ChargerCard(const ChargerInfo& charger, int displayIndex,
                         QWidget* parent)
    : QFrame(parent)
    , charger_(charger)
{
    setObjectName(QStringLiteral("ChargerCard"));
    setCursor(charger_.isIdle() ? Qt::PointingHandCursor : Qt::ArrowCursor);
    setStyleSheet(QStringLiteral(
        "QFrame#ChargerCard { background-color: %1; border: 1px solid %2;"
        " border-radius: 12px; }"
        "QFrame#ChargerCard:hover { border-color: %3; }")
        .arg(cssRgba(theme::cardFill(), 220), cssColor(theme::cardBorder()),
             cssColor(theme::primaryBlue())));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(16, 8, 14, 8);
    root->setSpacing(14);

    auto* indexLabel = new QLabel(QString::number(displayIndex), this);
    indexLabel->setFixedSize(28, 28);
    indexLabel->setAlignment(Qt::AlignCenter);
    indexLabel->setStyleSheet(QStringLiteral(
        "background-color: %1; color: white; border-radius: 14px;"
        "font-size: 14px; font-weight: 700;").arg(cssColor(theme::primaryBlue())));

    auto* codeLabel = new QLabel(charger_.code, this);
    codeLabel->setMinimumWidth(76);
    codeLabel->setStyleSheet(QStringLiteral(
        "font-size: 17px; font-weight: 600; color: %1; background: transparent;")
        .arg(cssColor(theme::textPrimary())));

    auto* typeLabel = new QLabel(charger_.typeLabel(), this);
    typeLabel->setMinimumWidth(48);
    typeLabel->setStyleSheet(QStringLiteral(
        "font-size: 16px; color: %1; background: transparent;")
        .arg(cssColor(theme::textPrimary())));

    auto* powerLabel = new QLabel(
        QStringLiteral("%1kW").arg(charger_.powerKw, 0, 'f', 0), this);
    powerLabel->setMinimumWidth(66);
    powerLabel->setStyleSheet(QStringLiteral(
        "font-size: 16px; color: %1; background: transparent;")
        .arg(cssColor(theme::textPrimary())));

    auto* statusLabel = new QLabel(charger_.statusLabel(), this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setFixedSize(72, 28);
    const QString accent = cssColor(statusColor(charger_.status));
    statusLabel->setStyleSheet(QStringLiteral(
        "border: 1px solid %1; border-radius: 12px; padding: 0 8px;"
        "color: %1; font-size: 15px; font-weight: 600; background: transparent;")
        .arg(accent));

    root->addWidget(indexLabel);
    root->addWidget(codeLabel);
    root->addWidget(typeLabel);
    root->addWidget(powerLabel, 1);
    root->addWidget(statusLabel);
}

void ChargerCard::mouseReleaseEvent(QMouseEvent* event)
{
    if (charger_.isIdle() && event->button() == Qt::LeftButton &&
        rect().contains(event->pos())) {
        emit selected(charger_.chargerId);
    }
    QFrame::mouseReleaseEvent(event);
}
