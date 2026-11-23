#pragma once

#include <QHash>
#include <QStyledItemDelegate>
#include <QToolBar>

class QComboBox;
class QLineEdit;
class QSqlTableModel;
class QStatusBar;

class ShapeFilters : public QToolBar
{
    Q_OBJECT

public:
    ShapeFilters(QSqlTableModel *model, QStatusBar *status, QWidget *parent);

private:
    void applyFilters();
    void resetFilters();

    QSqlTableModel *model;
    QStatusBar *status;
    QLineEdit *numberFilter;
    QComboBox *typeFilter;
    QLineEdit *countFilter;
};

class ShapeCountDelegate : public QStyledItemDelegate
{
public:
    explicit ShapeCountDelegate(QObject *parent);
    void setCounts(const QHash<int, int> &newCounts);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    QHash<int, int> counts;
};
