#include "ui/widgets/EntityTableView.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

EntityTableView::EntityTableView(const QStringList& headers, QWidget* parent)
    : QWidget(parent)
    , table_(new QTableView(this))
    , model_(new QStandardItemModel(this))
    , proxy_(new QSortFilterProxyModel(this))
    , stack_(new QStackedWidget(this))
{
    model_->setHorizontalHeaderLabels(headers);
    proxy_->setSourceModel(model_);
    proxy_->setDynamicSortFilter(true);
    proxy_->setSortRole(SortValueRole);

    table_->setModel(proxy_);
    table_->setAlternatingRowColors(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSortingEnabled(true);
    table_->sortByColumn(0, Qt::AscendingOrder);
    table_->verticalHeader()->setVisible(false);
    table_->verticalHeader()->setDefaultSectionSize(42);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setMinimumSectionSize(92);

    auto* statePage = new QWidget(this);
    auto* stateLayout = new QVBoxLayout(statePage);
    stateLayout->setAlignment(Qt::AlignCenter);
    stateLayout->setSpacing(8);

    stateTitle_ = new QLabel(statePage);
    stateTitle_->setObjectName(QStringLiteral("stateTitle"));
    stateTitle_->setAlignment(Qt::AlignCenter);
    stateMessage_ = new QLabel(statePage);
    stateMessage_->setObjectName(QStringLiteral("stateMessage"));
    stateMessage_->setAlignment(Qt::AlignCenter);
    stateMessage_->setWordWrap(true);
    retryButton_ = new QPushButton(QStringLiteral("重新加载"), statePage);
    retryButton_->setObjectName(QStringLiteral("secondaryButton"));
    retryButton_->setFixedWidth(112);
    connect(retryButton_, &QPushButton::clicked, this, &EntityTableView::retryRequested);

    stateLayout->addWidget(stateTitle_);
    stateLayout->addWidget(stateMessage_);
    stateLayout->addSpacing(6);
    stateLayout->addWidget(retryButton_, 0, Qt::AlignHCenter);

    stack_->addWidget(table_);
    stack_->addWidget(statePage);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(stack_);

    setState(State::Empty);
}

void EntityTableView::setRows(const QList<Row>& rows)
{
    model_->removeRows(0, model_->rowCount());

    for (const Row& row : rows) {
        QList<QStandardItem*> items;
        for (const QString& cell : row.cells) {
            items.append(new QStandardItem(cell));
        }
        while (items.size() < model_->columnCount()) {
            items.append(new QStandardItem);
        }
        for (int column = 0; column < items.size(); ++column) {
            const QVariant sortValue = column < row.sortValues.size()
                ? row.sortValues.at(column)
                : items.at(column)->text();
            items.at(column)->setData(sortValue, SortValueRole);
        }
        if (!items.isEmpty()) {
            // ID 存在 model 的自定义 role 中，不依赖可变的视图行号或显示文本。
            items.first()->setData(row.entityId, EntityIdRole);
        }
        for (const CellStyle& style : row.styles) {
            if (style.column < 0 || style.column >= items.size()) {
                continue;
            }
            QStandardItem* item = items.at(style.column);
            if (style.foreground.isValid()) {
                item->setForeground(QBrush(style.foreground));
            }
            if (style.background.isValid()) {
                item->setBackground(QBrush(style.background));
            }
            if (style.bold) {
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
        }
        model_->appendRow(items);
    }

    setState(rows.isEmpty() ? State::Empty : State::Ready);
}

void EntityTableView::setState(State state, const QString& message)
{
    if (state == State::Ready) {
        stack_->setCurrentWidget(table_);
        return;
    }

    stack_->setCurrentIndex(1);
    retryButton_->setVisible(state == State::Error);

    switch (state) {
    case State::Loading:
        stateTitle_->setText(QStringLiteral("正在加载"));
        stateMessage_->setText(message.isEmpty()
                                   ? QStringLiteral("正在等待服务器返回数据…")
                                   : message);
        break;
    case State::Empty:
        stateTitle_->setText(QStringLiteral("暂无数据"));
        stateMessage_->setText(message.isEmpty()
                                   ? QStringLiteral("连接服务器后，此处将展示真实业务数据")
                                   : message);
        break;
    case State::Error:
        stateTitle_->setText(QStringLiteral("加载失败"));
        stateMessage_->setText(message.isEmpty()
                                   ? QStringLiteral("请检查服务器连接后重试")
                                   : message);
        break;
    case State::Ready:
        break;
    }
}

QVariant EntityTableView::entityIdAt(const QModelIndex& viewIndex) const
{
    if (!viewIndex.isValid()) {
        return {};
    }
    const QModelIndex sourceIndex = proxy_->mapToSource(viewIndex);
    return model_->index(sourceIndex.row(), 0).data(EntityIdRole);
}

QTableView* EntityTableView::tableView() const
{
    return table_;
}
