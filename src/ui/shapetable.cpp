#include "ui/shapetable.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPainter>
#include <QPolygonF>
#include <QSqlTableModel>
#include <QStatusBar>
#include <QStringList>
#include <QStyle>
#include <QVariant>
#include <QWidget>

ShapeFilters::ShapeFilters(QSqlTableModel *model, QStatusBar *status, QWidget *parent)
    : QToolBar(tr("Фильтры"), parent)
    , model(model)
    , status(status)
    , numberFilter(new QLineEdit(this))
    , typeFilter(new QComboBox(this))
    , countFilter(new QLineEdit(this))
{
    numberFilter->setPlaceholderText(tr("Номер фигуры"));
    numberFilter->setMaximumWidth(140);
    addWidget(numberFilter);

    typeFilter->addItem(tr("Все типы"));
    typeFilter->addItem(tr("Прямоугольник"));
    typeFilter->addItem(tr("Эллипс"));
    typeFilter->addItem(tr("Многоугольник"));
    addWidget(typeFilter);

    countFilter->setPlaceholderText(tr("Количество по типу"));
    countFilter->setMaximumWidth(170);
    addWidget(countFilter);

    auto *applyFilters = addAction(tr("Применить"));
    auto *resetFilters = addAction(tr("Сбросить"));

    connect(applyFilters, &QAction::triggered, this, &ShapeFilters::applyFilters);
    connect(resetFilters, &QAction::triggered, this, &ShapeFilters::resetFilters);
}

void ShapeFilters::applyFilters()
{
    QStringList conditions;

    const QString numberText = numberFilter->text().trimmed();
    if (!numberText.isEmpty()) {
        bool ok = false;
        const qint64 number = numberText.toLongLong(&ok);
        if (!ok || number < 1) {
            status->showMessage(tr("Введи положительный номер фигуры"), 3000);
            return;
        }
        conditions << QString("id = %1").arg(number);
    }

    if (typeFilter->currentIndex() > 0) {
        conditions << QString("type = %1").arg(typeFilter->currentIndex() - 1);
    }

    const QString countText = countFilter->text().trimmed();
    if (!countText.isEmpty()) {
        bool ok = false;
        const qint64 count = countText.toLongLong(&ok);
        if (!ok || count < 1) {
            status->showMessage(tr("Введи положительное количество фигур"), 3000);
            return;
        }

        conditions << QString(
                "(SELECT total FROM shape_counts "
                "WHERE shape_counts.type = shapes.type) = %1").arg(count);
    }

    model->setFilter(conditions.join(" AND "));
}

void ShapeFilters::resetFilters()
{
    numberFilter->clear();
    typeFilter->setCurrentIndex(0);
    countFilter->clear();
    model->setFilter(QString());
}

ShapeCountDelegate::ShapeCountDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ShapeCountDelegate::setCounts(const QHash<int, int> &newCounts)
{
    counts = newCounts;
}

void ShapeCountDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
           const QModelIndex &index) const
{
    const int type = index.data().toInt();
    const int count = counts.value(type);

    QStyleOptionViewItem cell(option);
    initStyleOption(&cell, index);
    cell.text.clear();

    const QStyle *style = cell.widget ? cell.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &cell, painter, cell.widget);

    if (count == 0) {
        return;
    }

    const int icons = count <= 3 ? 1 : (count <= 10 ? 2 : 3);
    const bool selected = cell.state.testFlag(QStyle::State_Selected);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(cell.palette.color(
        selected ? QPalette::HighlightedText : QPalette::Text));
    painter->setBrush(Qt::NoBrush);

    for (int i = 0; i < icons; ++i) {
        const int x = cell.rect.left() + 6 + i * 18;
        const int y = cell.rect.center().y() - 5;
        const QRect icon(x, y, 13, 10);

        switch (type) {
        case 0:
            painter->drawRect(icon);
            break;
        case 1:
            painter->drawEllipse(icon);
            break;
        case 2: {
            QPolygonF triangle;
            triangle << QPointF(x + 6.5, y)
                     << QPointF(x + 13, y + 10)
                     << QPointF(x, y + 10);
            painter->drawPolygon(triangle);
            break;
        }
        }
    }

    painter->restore();
}
