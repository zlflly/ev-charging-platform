#pragma once

#include "model/Charger.h"

#include <QHash>
#include <QWidget>

class AdminApiClient;
class EntityTableView;
class QLabel;
class QModelIndex;
class QPushButton;
class QComboBox;
class QLineEdit;

class ChargerManagementPage final : public QWidget
{
    Q_OBJECT

public:
    explicit ChargerManagementPage(AdminApiClient* api,
                                   QWidget* parent = nullptr);

    void refresh();

private:
    void updateSelection(const QModelIndex& current);
    void restartSelectedCharger();
    void setLoading(bool loading);
    void showError(const QString& message);
    void showChargers(const QList<Charger>& chargers);
    void populateStationFilter();
    void applyFilters();
    void resetFilters();
    void updateSortHint(int column, Qt::SortOrder order);
    qint64 selectedChargerId() const;
    static QString formatDuration(qint64 totalSeconds);

    AdminApiClient* api_ = nullptr;
    EntityTableView* table_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QLabel* filterCountLabel_ = nullptr;
    QLabel* sortHintLabel_ = nullptr;
    QLabel* selectionLabel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* stationFilter_ = nullptr;
    QComboBox* typeFilter_ = nullptr;
    QComboBox* statusFilter_ = nullptr;
    QPushButton* resetFilterButton_ = nullptr;
    QPushButton* faultAlertButton_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    QPushButton* restartButton_ = nullptr;
    QList<Charger> allChargers_;
    QHash<qint64, Charger> chargersById_;
};
