#pragma once

#include "model/Order.h"

#include <QJsonObject>
#include <QList>
#include <QWidget>

class QLabel;
class QFrame;
class QPushButton;
class QLineEdit;
class QScrollArea;
class QWidget;
class NetworkClient;

// 移动端订单页，历史记录来自 order.history；服务端没有返回的记录不在此伪造。
class OrderPage : public QWidget
{
    Q_OBJECT

public:
    explicit OrderPage(NetworkClient* networkClient, QWidget* parent = nullptr);

    void reload();

signals:
    void backRequested();
    void continueOrderRequested(const OrderInfo& order);
    void navigateAgainRequested(const OrderInfo& order);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onFilterClicked();
    void onSearchChanged(const QString& text);

private:
    void buildUi();
    void layoutUi();
    void renderSummary(const QJsonObject& summary);
    void renderOrders();
    void showState(const QString& text, bool error = false);
    bool matchesFilter(const OrderInfo& order) const;
    QString statusText(const OrderInfo& order) const;
    QString formatDate(qint64 timestampMs) const;
    QString formatDuration(qint64 milliseconds) const;

    NetworkClient* networkClient_ = nullptr;
    QPushButton* backButton_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QFrame* summaryCard_ = nullptr;
    QLabel* totalLabel_ = nullptr;
    QLabel* activeLabel_ = nullptr;
    QLabel* pendingLabel_ = nullptr;
    QLabel* spendLabel_ = nullptr;
    QList<QPushButton*> filterButtons_;
    QLineEdit* searchEdit_ = nullptr;
    QScrollArea* orderScroll_ = nullptr;
    QWidget* orderContainer_ = nullptr;
    QLabel* stateLabel_ = nullptr;

    QList<OrderInfo> orders_;
    QJsonObject summary_;
    int filterMode_ = 0;
    QString searchText_;
    bool loading_ = false;
};
