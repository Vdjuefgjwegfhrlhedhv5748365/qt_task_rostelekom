#pragma once

#include <QHash>
#include <QList>
#include <QPoint>
#include <QSize>
#include <QSqlDatabase>

struct Shape
{
    qint64 id = 0;
    int type = 0;
    QPoint position;
    QSize size;
};

struct ShapeLink
{
    qint64 firstId;
    qint64 secondId;
};

class ShapeDatabase
{
public:
    enum class LinkResult { Saved, Duplicate, Error };

    bool open(QString &error);
    QSqlDatabase connection() const;
    bool loadShapes(QList<Shape> &shapes, QString &error) const;
    bool loadLinks(QList<ShapeLink> &links, QString &error) const;
    bool loadCounts(QHash<int, int> &counts, QString &error) const;
    bool insertShape(Shape &shape, QString &error) const;
    LinkResult insertLink(qint64 firstId, qint64 secondId, QString &error) const;
    bool removeShape(qint64 id, QString &error);

private:
    QSqlDatabase database;
};
