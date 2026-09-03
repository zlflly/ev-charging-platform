#include "ui/StationCard.h"

#include "ui/theme/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSizePolicy>
#include <QVBoxLayout>

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

class StationPile : public QLabel
{
public:
    explicit StationPile(QWidget* parent = nullptr) : QLabel(parent)
    {
        setAlignment(Qt::AlignCenter);
        setFixedSize(52, 64);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.setPen(QPen(theme::cardBorder(), 1));
        painter.setBrush(theme::inputFill());
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 11, 11);

        const QPixmap source(QStringLiteral(":/resources/images/station_pile_generated.png"));
        if (source.isNull()) {
            painter.setPen(theme::primaryBlue());
            painter.setFont(theme::buttonFont());
            painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("EV"));
            return;
        }

        const QPixmap scaled = source.scaled(45, 60, Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        painter.drawPixmap((width() - scaled.width()) / 2,
                           (height() - scaled.height()) / 2, scaled);
    }
};

} // namespace

StationCard::StationCard(const StationInfo& station, QWidget* parent)
    : QFrame(parent)
    , station_(station)
{
    setObjectName(QStringLiteral("StationCard"));
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(QStringLiteral(
        "#StationCard { background-color: %1; border: 1px solid %2;"
        " border-radius: 17px; }"
        "#StationCard:hover { border-color: %3; }")
        .arg(cssRgba(theme::cardFill(), 235), cssColor(theme::cardBorder()),
             cssColor(theme::primaryBlue())));

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(20, 12, 18, 12);
    root->setSpacing(18);

    root->addWidget(new StationPile(this), 0, Qt::AlignVCenter);

    auto* middle = new QVBoxLayout();
    middle->setSpacing(3);
    middle->setContentsMargins(0, 0, 0, 0);

    auto* name = new QLabel(station_.name, this);
    name->setFont(theme::fieldLabelFont());
    name->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textPrimary())));
    name->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    name->setMinimumWidth(0);

    auto* type = new QLabel(QStringLiteral("快充  |  24h"), this);
    type->setFont(theme::inputFont());
    type->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textSecondary())));

    auto* capacity = new QLabel(this);
    capacity->setTextFormat(Qt::RichText);
    capacity->setFont(theme::inputFont());
    capacity->setStyleSheet(QStringLiteral("background: transparent;"));
    capacity->setText(QStringLiteral(
        "<span style='color:%1;'>总桩&nbsp; %2</span>"
        "<span style='color:%1;'>　|　空闲 </span>"
        "<span style='color:%3;'>%4</span>")
        .arg(cssColor(theme::textSecondary()))
        .arg(station_.totalChargers)
        .arg(cssColor(theme::primaryBlue()))
        .arg(station_.availableChargers));

    middle->addWidget(name);
    middle->addWidget(type);
    middle->addWidget(capacity);
    root->addLayout(middle, 1);

    auto* right = new QVBoxLayout();
    right->setSpacing(8);
    right->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* price = new QLabel(this);
    price->setTextFormat(Qt::RichText);
    price->setText(QStringLiteral(
        "<span style='color:%1; font-size:21px; font-weight:700;'>¥%2</span>"
        "<span style='color:%3; font-size:14px;'>/度</span>")
        .arg(cssColor(theme::priceAmber()))
        .arg(station_.pricePerKwh, 0, 'f', 2)
        .arg(cssColor(theme::textPrimary())));
    price->setAlignment(Qt::AlignRight);
    price->setStyleSheet(QStringLiteral("background: transparent;"));

    auto* distance = new QLabel(
        QStringLiteral("%1 km").arg(station_.distanceKm, 0, 'f', 1), this);
    distance->setFont(theme::inputFont());
    distance->setAlignment(Qt::AlignRight);
    distance->setStyleSheet(QStringLiteral(
        "color: %1; background: transparent;").arg(cssColor(theme::textPrimary())));

    right->addWidget(price);
    right->addWidget(distance);
    root->addLayout(right, 0);
}

void StationCard::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->pos())) {
        emit selected(station_.stationId);
    }
    QFrame::mouseReleaseEvent(event);
}
