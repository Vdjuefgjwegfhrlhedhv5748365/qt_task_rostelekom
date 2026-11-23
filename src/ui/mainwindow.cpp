#include "ui/mainwindow.h"

#include "shapescene.h"
#include "ui/shapetable.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QGraphicsView>
#include <QLabel>
#include <QPainter>
#include <QSpinBox>
#include <QSqlError>
#include <QSqlTableModel>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new ShapeScene(this))
{
    ui->setupUi(this);
    setWindowTitle(tr("Редактор фигур"));

    auto *view = new QGraphicsView(scene, this);
    view->setRenderHint(QPainter::Antialiasing);
    setCentralWidget(view);

    if (!loadScene() || !setupShapeTable()) {
        return;
    }

    setupShapeActions();
    addToolBarBreak();
    addToolBar(new ShapeFilters(shapeModel, statusBar(), this));
    statusBar()->showMessage(tr("Фигуры и связи загружены"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::loadScene()
{
    QString error;
    QList<Shape> shapes;
    QList<ShapeLink> links;

    if (!database.open(error) ||
        !database.loadShapes(shapes, error) ||
        !database.loadLinks(links, error)) {
        statusBar()->showMessage(tr("Не удалось загрузить данные: %1").arg(error));
        return false;
    }

    for (const Shape &shape : shapes) {
        scene->drawShape(shape);
    }
    for (const ShapeLink &link : links) {
        scene->drawLink(link);
    }
    return true;
}

bool MainWindow::setupShapeTable()
{
    shapeModel = new QSqlTableModel(this, database.connection());
    shapeModel->setTable("shapes");
    shapeModel->setHeaderData(0, Qt::Horizontal, tr("Номер"));
    shapeModel->setHeaderData(1, Qt::Horizontal, tr("Тип"));
    shapeModel->setHeaderData(2, Qt::Horizontal, tr("Ширина"));
    shapeModel->setHeaderData(3, Qt::Horizontal, tr("Высота"));

    if (!shapeModel->select()) {
        statusBar()->showMessage(tr("Не удалось открыть список фигур: %1")
                                 .arg(shapeModel->lastError().text()));
        return false;
    }

    shapeTable = new QTableView;
    shapeTable->setModel(shapeModel);
    shapeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    shapeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    shapeTable->hideColumn(4);
    shapeTable->hideColumn(5);
    shapeTable->resizeColumnsToContents();

    countDelegate = new ShapeCountDelegate(shapeTable);
    shapeTable->setItemDelegateForColumn(1, countDelegate);
    shapeTable->setColumnWidth(1, 72);
    refreshCounts();

    connect(shapeModel, &QAbstractItemModel::modelReset, this, &MainWindow::refreshCounts);
    connect(shapeTable, &QTableView::clicked, scene, &QGraphicsScene::clearSelection);
    connect(scene, &QGraphicsScene::selectionChanged, shapeTable, [this]() {
        if (!scene->selectedItems().isEmpty()) {
            shapeTable->clearSelection();
            shapeTable->setCurrentIndex(QModelIndex());
        }
    });

    auto *dock = new QDockWidget(tr("Список фигур"), this);
    dock->setWidget(shapeTable);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    return true;
}

void MainWindow::setupShapeActions()
{
    auto *toolbar = addToolBar(tr("Фигуры"));
    shapeType = new QComboBox(toolbar);
    shapeType->addItems({tr("Прямоугольник"), tr("Эллипс"), tr("Многоугольник")});
    toolbar->addWidget(shapeType);

    widthBox = new QSpinBox(toolbar);
    widthBox->setRange(10, 100);
    widthBox->setValue(100);
    toolbar->addWidget(new QLabel(tr("Ширина")));
    toolbar->addWidget(widthBox);

    heightBox = new QSpinBox(toolbar);
    heightBox->setRange(10, 60);
    heightBox->setValue(60);
    toolbar->addWidget(new QLabel(tr("Высота")));
    toolbar->addWidget(heightBox);

    auto *addAction = toolbar->addAction(tr("Добавить фигуру"));
    auto *connectAction = toolbar->addAction(tr("Соединить"));
    auto *deleteAction = toolbar->addAction(tr("Удалить фигуру"));
    connect(addAction, &QAction::triggered, this, &MainWindow::addShape);
    connect(connectAction, &QAction::triggered, this, &MainWindow::connectSelectedShapes);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedShape);
}

void MainWindow::addShape()
{
    Shape shape;
    shape.type = shapeType->currentIndex();
    shape.position = scene->nextPosition();
    shape.size = QSize(widthBox->value(), heightBox->value());

    QString error;
    if (!database.insertShape(shape, error)) {
        statusBar()->showMessage(tr("Не удалось сохранить фигуру: %1").arg(error));
        return;
    }

    scene->drawShape(shape);
    statusBar()->showMessage(tr("Фигура №%1 сохранена").arg(shape.id), 3000);
    refreshTable();
}

void MainWindow::connectSelectedShapes()
{
    const auto selected = scene->selectedShapeIds();
    if (selected.size() != 2) {
        statusBar()->showMessage(tr("Выдели две фигуры"), 3000);
        return;
    }

    const ShapeLink link{selected.at(0), selected.at(1)};
    QString error;
    switch (database.insertLink(link.firstId, link.secondId, error)) {
    case ShapeDatabase::LinkResult::Saved:
        scene->drawLink(link);
        statusBar()->showMessage(tr("Связь сохранена"), 3000);
        break;
    case ShapeDatabase::LinkResult::Duplicate:
        statusBar()->showMessage(tr("Эти фигуры уже соединены"), 3000);
        break;
    case ShapeDatabase::LinkResult::Error:
        statusBar()->showMessage(tr("Не удалось сохранить связь: %1").arg(error));
        break;
    }
}

void MainWindow::deleteSelectedShape()
{
    const auto selected = scene->selectedShapeIds();
    if (selected.size() > 1) {
        statusBar()->showMessage(tr("Для удаления выбери одну фигуру"), 3000);
        return;
    }

    qint64 id;
    if (selected.size() == 1) {
        id = selected.first();
    } else {
        const QModelIndex current = shapeTable->currentIndex();
        if (!current.isValid()) {
            statusBar()->showMessage(tr("Выбери фигуру на сцене или в таблице"), 3000);
            return;
        }
        id = shapeModel->index(current.row(), 0).data().toLongLong();
    }

    QString error;
    if (!database.removeShape(id, error)) {
        statusBar()->showMessage(tr("Не удалось удалить фигуру: %1").arg(error));
        return;
    }

    scene->removeShape(id);
    statusBar()->showMessage(tr("Фигура №%1 удалена").arg(id), 3000);
    refreshTable();
}

void MainWindow::refreshTable()
{
    if (!shapeModel->select()) {
        statusBar()->showMessage(tr("Не удалось обновить список фигур: %1")
                                 .arg(shapeModel->lastError().text()));
    }
}

void MainWindow::refreshCounts()
{
    QHash<int, int> counts;
    QString error;
    if (!database.loadCounts(counts, error)) {
        statusBar()->showMessage(tr("Не удалось загрузить количество фигур: %1").arg(error));
        return;
    }

    countDelegate->setCounts(counts);
    shapeTable->viewport()->update();
}
