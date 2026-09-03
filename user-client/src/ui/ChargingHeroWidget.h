#pragma once

#include <QString>
#include <QWidget>

// 充电页中心进度环：车辆背景由 ChargingPage 绘制，这里负责环形进度和读数。
class ChargingHeroWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChargingHeroWidget(QWidget* parent = nullptr);

    // 保留小数进度让弧线连续移动，中心文字仍显示四舍五入后的整数。
    void setProgress(double percent);
    void setElapsedText(const QString& text);
    void setSubtitleVisible(bool visible);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    double progressPercent_ = 0.0;
    QString elapsedText_;
    bool showSubtitle_ = false;
};
