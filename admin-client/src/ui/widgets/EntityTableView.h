#pragma once

#include <QModelIndex>
#include <QColor>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <QWidget>

class QLabel;
class QPushButton;
class QSortFilterProxyModel;
class QStandardItemModel;
class QStackedWidget;
class QTableView;

class EntityTableView final : public QWidget
{
    Q_OBJECT

public:
    enum class State { Loading, Ready, Empty, Error };
    static constexpr int EntityIdRole = Qt::UserRole + 1;
    static constexpr int SortValueRole = Qt::UserRole + 2;

    struct CellStyle {
        int column = -1;
        QColor foreground;
        QColor background;
        bool bold = false;
    };

    struct Row {
        QVariant entityId;
        QStringList cells;
        QList<CellStyle> styles;
        QList<QVariant> sortValues;
    };

    explicit EntityTableView(const QStringList& headers, QWidget* parent = nullptr);

    void setRows(const QList<Row>& rows);
    void setState(State state, const QString& message = {});
    QVariant entityIdAt(const QModelIndex& viewIndex) const;
    QTableView* tableView() const;

signals:
    void retryRequested();

private:
    QTableView* table_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QSortFilterProxyModel* proxy_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QLabel* stateTitle_ = nullptr;
    QLabel* stateMessage_ = nullptr;
    QPushButton* retryButton_ = nullptr;
};
