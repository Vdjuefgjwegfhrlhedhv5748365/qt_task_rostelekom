#pragma once

#include <QGraphicsScene>
#include <QHash>
#include <QPoint>

struct Shape;
struct ShapeLink;

class ShapeScene : public QGraphicsScene
{
public:
    explicit ShapeScene(QObject *parent);

    QPoint nextPosition() const;
    void drawShape(const Shape &shape);
    void drawLink(const ShapeLink &link);
    QList<qint64> selectedShapeIds() const;
    void removeShape(qint64 id);

private:
    QHash<qint64, QGraphicsItem *> shapesById;
    int nextSlot = 0;
};
