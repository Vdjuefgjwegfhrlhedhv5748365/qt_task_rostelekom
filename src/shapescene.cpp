#include "shapescene.h"

#include "shapedatabase.h"

#include <QBrush>
#include <QGraphicsItem>
#include <QPen>
#include <QPolygonF>
#include <QVariant>

namespace {
constexpr int shapeIdRole = 0;
constexpr int firstLinkIdRole = 1;
constexpr int secondLinkIdRole = 2;
}

ShapeScene::ShapeScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(0, 0, 400, 300);
    setBackgroundBrush(QColor("#f4f6f8"));
}

QPoint ShapeScene::nextPosition() const
{
    return {20 + (nextSlot % 3) * 125, 20 + (nextSlot / 3) * 90};
}

void ShapeScene::drawShape(const Shape &shape)
{
    const QRectF bounds(shape.position, shape.size);
    const QPen outline(QColor("#263547"), 1.4);
    QGraphicsItem *item = nullptr;

    switch (shape.type) {
    case 0:
        item = addRect(bounds, outline);
        break;
    case 1:
        item = addEllipse(bounds, outline);
        break;
    case 2: {
        QPolygonF triangle;
        triangle << QPointF(bounds.center().x(), bounds.top())
                 << bounds.bottomRight() << bounds.bottomLeft();
        item = addPolygon(triangle, outline);
        break;
    }
    default:
        return;
    }

    item->setFlag(QGraphicsItem::ItemIsSelectable);
    item->setData(shapeIdRole, shape.id);
    shapesById.insert(shape.id, item);
    ++nextSlot;
    setSceneRect(sceneRect().united(item->sceneBoundingRect().adjusted(-20, -20, 20, 20)));
}

void ShapeScene::drawLink(const ShapeLink &link)
{
    const auto *first = shapesById.value(link.firstId);
    const auto *second = shapesById.value(link.secondId);
    if (!first || !second) {
        return;
    }

    auto *line = addLine(QLineF(first->sceneBoundingRect().center(),
                               second->sceneBoundingRect().center()),
                         QPen(QColor("#3368a1"), 1.5));
    line->setZValue(-1);
    line->setData(firstLinkIdRole, link.firstId);
    line->setData(secondLinkIdRole, link.secondId);
}

QList<qint64> ShapeScene::selectedShapeIds() const
{
    QList<qint64> ids;
    const auto selection = selectedItems();
    for (const auto *item : selection) {
        ids.append(item->data(shapeIdRole).toLongLong());
    }
    return ids;
}

void ShapeScene::removeShape(qint64 id)
{
    const auto sceneItems = items();
    for (auto *item : sceneItems) {
        if (item->data(firstLinkIdRole).toLongLong() == id ||
            item->data(secondLinkIdRole).toLongLong() == id) {
            delete item;
        }
    }
    delete shapesById.take(id);
}
