#pragma once

#include "model/User.h"

#include <QHash>
#include <QWidget>

class AdminApiClient;
class EntityTableView;
class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;
class QComboBox;

class UserManagementPage final : public QWidget
{
    Q_OBJECT

public:
    explicit UserManagementPage(AdminApiClient* api, QWidget* parent = nullptr);

    void refresh();

private:
    void search();
    void resetSearch();
    void loadPage(int page);
    void showPage(const UserListPage& page);
    void updateSelection(const QModelIndex& current);
    void changeSelectedUserStatus();
    qint64 selectedUserId() const;
    void setLoading(bool loading);

    AdminApiClient* api_ = nullptr;
    EntityTableView* table_ = nullptr;
    QLineEdit* phoneEdit_ = nullptr;
    QComboBox* statusFilter_ = nullptr;
    QComboBox* activityFilter_ = nullptr;
    QPushButton* searchButton_ = nullptr;
    QPushButton* resetButton_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    QPushButton* previousButton_ = nullptr;
    QPushButton* nextButton_ = nullptr;
    QPushButton* statusButton_ = nullptr;
    QLabel* totalLabel_ = nullptr;
    QLabel* pageLabel_ = nullptr;
    QLabel* selectionLabel_ = nullptr;
    UserListQuery query_;
    qint64 total_ = 0;
    QHash<qint64, AdminUser> usersById_;
};
