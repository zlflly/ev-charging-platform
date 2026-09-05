#pragma once

#include "model/Order.h"

#include <QHash>
#include <QWidget>

class AdminApiClient;
class EntityTableView;
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QModelIndex;

class OrderManagementPage final : public QWidget
{
    Q_OBJECT

public:
    explicit OrderManagementPage(AdminApiClient* api, QWidget* parent = nullptr);

    void refresh();

private:
    void search();
    void resetSearch();
    void loadPage(int page);
    void showPage(const OrderListPage& page);
    void updateSelection(const QModelIndex& current);
    void showOrderDetail(const AdminOrder* order);
    void setLoading(bool loading);

    AdminApiClient* api_ = nullptr;
    EntityTableView* table_ = nullptr;
    QLineEdit* keywordEdit_ = nullptr;
    QComboBox* statusFilter_ = nullptr;
    QComboBox* paymentFilter_ = nullptr;
    QPushButton* searchButton_ = nullptr;
    QPushButton* resetButton_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    QPushButton* previousButton_ = nullptr;
    QPushButton* nextButton_ = nullptr;
    QLabel* totalValue_ = nullptr;
    QLabel* reservedValue_ = nullptr;
    QLabel* chargingValue_ = nullptr;
    QLabel* unpaidValue_ = nullptr;
    QLabel* paidAmountValue_ = nullptr;
    QLabel* queryLabel_ = nullptr;
    QLabel* pageLabel_ = nullptr;
    QLabel* updatedLabel_ = nullptr;
    QLabel* detailTitle_ = nullptr;
    QLabel* detailUser_ = nullptr;
    QLabel* detailDevice_ = nullptr;
    QLabel* detailBilling_ = nullptr;
    QLabel* detailTimes_ = nullptr;
    OrderListQuery query_;
    qint64 total_ = 0;
    QHash<qint64, AdminOrder> ordersById_;
};
