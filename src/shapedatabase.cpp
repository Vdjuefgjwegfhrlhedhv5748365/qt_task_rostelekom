#include "shapedatabase.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>

bool ShapeDatabase::open(QString &error)
{
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (directory.isEmpty() || !QDir().mkpath(directory)) {
        error = QStringLiteral("Не удалось создать папку для базы данных");
        return false;
    }

    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(QDir(directory).filePath("shapes.sqlite"));
    if (!database.open() || !database.transaction()) {
        error = database.lastError().text();
        return false;
    }

    const QStringList statements = {
        R"(CREATE TABLE IF NOT EXISTS shapes (
            id INTEGER PRIMARY KEY, type INTEGER NOT NULL,
            width INTEGER NOT NULL, height INTEGER NOT NULL,
            x INTEGER NOT NULL, y INTEGER NOT NULL
        ))",
        R"(CREATE TABLE IF NOT EXISTS shape_links (
            first_id INTEGER NOT NULL, second_id INTEGER NOT NULL,
            PRIMARY KEY (first_id, second_id)
        ))",
        R"(WITH positions AS (
            SELECT id, ROW_NUMBER() OVER (ORDER BY id) - 1 AS slot FROM shapes
        )
        UPDATE shapes
        SET x = 20 + (positions.slot % 3) * 125,
            y = 20 + (positions.slot / 3) * 90
        FROM positions
        WHERE shapes.id = positions.id
          AND (shapes.x <> 20 + (positions.slot % 3) * 125
            OR shapes.y <> 20 + (positions.slot / 3) * 90))",
        R"(CREATE TABLE IF NOT EXISTS shape_counts (
            type INTEGER PRIMARY KEY, total INTEGER NOT NULL
        ))",
        "DELETE FROM shape_counts",
        "INSERT INTO shape_counts (type, total) SELECT type, COUNT(*) FROM shapes GROUP BY type",
        R"(CREATE TRIGGER IF NOT EXISTS shapes_count_after_insert
           AFTER INSERT ON shapes
           BEGIN
               INSERT INTO shape_counts (type, total) VALUES (NEW.type, 1)
               ON CONFLICT(type) DO UPDATE SET total = total + 1;
           END)",
        R"(CREATE TRIGGER IF NOT EXISTS shapes_count_after_delete
           AFTER DELETE ON shapes
           BEGIN
               UPDATE shape_counts SET total = total - 1 WHERE type = OLD.type;
           END)"
    };

    QSqlQuery query(database);
    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            error = query.lastError().text();
            database.rollback();
            return false;
        }
    }

    if (!database.commit()) {
        error = database.lastError().text();
        database.rollback();
        return false;
    }
    return true;
}

QSqlDatabase ShapeDatabase::connection() const
{
    return database;
}

bool ShapeDatabase::loadShapes(QList<Shape> &shapes, QString &error) const
{
    QSqlQuery query(database);
    if (!query.exec("SELECT id, type, x, y, width, height FROM shapes ORDER BY id")) {
        error = query.lastError().text();
        return false;
    }

    while (query.next()) {
        shapes.append({query.value(0).toLongLong(), query.value(1).toInt(),
                       QPoint(query.value(2).toInt(), query.value(3).toInt()),
                       QSize(query.value(4).toInt(), query.value(5).toInt())});
    }
    error = query.lastError().text();
    return !query.lastError().isValid();
}

bool ShapeDatabase::loadLinks(QList<ShapeLink> &links, QString &error) const
{
    QSqlQuery query(database);
    if (!query.exec("SELECT first_id, second_id FROM shape_links")) {
        error = query.lastError().text();
        return false;
    }

    while (query.next()) {
        links.append({query.value(0).toLongLong(), query.value(1).toLongLong()});
    }
    error = query.lastError().text();
    return !query.lastError().isValid();
}

bool ShapeDatabase::loadCounts(QHash<int, int> &counts, QString &error) const
{
    QSqlQuery query(database);
    if (!query.exec("SELECT type, total FROM shape_counts")) {
        error = query.lastError().text();
        return false;
    }

    while (query.next()) {
        counts.insert(query.value(0).toInt(), query.value(1).toInt());
    }
    error = query.lastError().text();
    return !query.lastError().isValid();
}

bool ShapeDatabase::insertShape(Shape &shape, QString &error) const
{
    QSqlQuery query(database);
    if (!query.prepare("INSERT INTO shapes (type, width, height, x, y) VALUES (?, ?, ?, ?, ?)")) {
        error = query.lastError().text();
        return false;
    }

    query.addBindValue(shape.type);
    query.addBindValue(shape.size.width());
    query.addBindValue(shape.size.height());
    query.addBindValue(shape.position.x());
    query.addBindValue(shape.position.y());
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }

    shape.id = query.lastInsertId().toLongLong();
    return true;
}

ShapeDatabase::LinkResult ShapeDatabase::insertLink(qint64 firstId, qint64 secondId,
                                                    QString &error) const
{
    QSqlQuery query(database);
    if (!query.prepare("INSERT OR IGNORE INTO shape_links (first_id, second_id) VALUES (?, ?)")) {
        error = query.lastError().text();
        return LinkResult::Error;
    }

    query.addBindValue(qMin(firstId, secondId));
    query.addBindValue(qMax(firstId, secondId));
    if (!query.exec()) {
        error = query.lastError().text();
        return LinkResult::Error;
    }
    return query.numRowsAffected() == 0 ? LinkResult::Duplicate : LinkResult::Saved;
}

bool ShapeDatabase::removeShape(qint64 id, QString &error)
{
    if (!database.transaction()) {
        error = database.lastError().text();
        return false;
    }

    QSqlQuery query(database);
    const auto rollbackWithQueryError = [&]() {
        error = query.lastError().text();
        database.rollback();
        return false;
    };

    if (!query.prepare("DELETE FROM shape_links WHERE first_id = ? OR second_id = ?")) {
        return rollbackWithQueryError();
    }
    query.addBindValue(id);
    query.addBindValue(id);
    if (!query.exec() || !query.prepare("DELETE FROM shapes WHERE id = ?")) {
        return rollbackWithQueryError();
    }

    query.addBindValue(id);
    if (!query.exec()) {
        return rollbackWithQueryError();
    }
    if (!database.commit()) {
        error = database.lastError().text();
        database.rollback();
        return false;
    }
    return true;
}
