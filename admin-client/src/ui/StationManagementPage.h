#pragma once

#include "model/Station.h"

#include <QHash>
#include <QWidget>

class AdminApiClient;
class EntityTableView;
class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;

class StationManagementPage final : public QWidget
{
    Q_OBJECT

public:
    explicit StationManagementPage(AdminApiClient* api,
                                   QWidget* parent = nullptr);

    void refresh();

private:
    void applyFilter();
    void showStations(const QList<Station>& stations, qint64 selectStationId);
    void updateSelection(const QModelIndex& current);
    void loadDetail(qint64 stationId);
    void showDetail(const StationDetail& detail);
    void createStation();
    void editSelectedStation();
    void updateDetailSelection(const QModelIndex& current);
    void changeSelectedChargerStatus();
    void restartSelectedCharger();
    void selectStation(qint64 stationId);
    void setListLoading(bool loading);
    qint64 selectedStationId() const;
    qint64 selectedDetailChargerId() const;

    AdminApiClient* api_ = nullptr;
    EntityTableView* stationTable_ = nullptr;
    EntityTableView* detailTable_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QLabel* stationCountLabel_ = nullptr;
    QLabel* chargerCountLabel_ = nullptr;
    QLabel* filterCountLabel_ = nullptr;
    QLabel* detailTitle_ = nullptr;
    QLabel* detailMeta_ = nullptr;
    QLabel* detailSelectionLabel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    QPushButton* createButton_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* detailStatusButton_ = nullptr;
    QPushButton* detailRestartButton_ = nullptr;
    QList<Station> allStations_;
    QHash<qint64, Station> stationsById_;
    QHash<qint64, StationCharger> detailChargersById_;
    qint64 pendingSelectionId_ = 0;
    quint64 detailRequestGeneration_ = 0;
};
