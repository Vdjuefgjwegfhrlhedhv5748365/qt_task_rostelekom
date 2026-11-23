#pragma once

#include "shapedatabase.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QComboBox;
class QSpinBox;
class QSqlTableModel;
class QTableView;
class ShapeCountDelegate;
class ShapeScene;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    bool loadScene();
    bool setupShapeTable();
    void setupShapeActions();
    void addShape();
    void connectSelectedShapes();
    void deleteSelectedShape();
    void refreshTable();
    void refreshCounts();

    Ui::MainWindow *ui;
    ShapeDatabase database;
    ShapeScene *scene;
    QSqlTableModel *shapeModel = nullptr;
    QTableView *shapeTable = nullptr;
    ShapeCountDelegate *countDelegate = nullptr;
    QComboBox *shapeType = nullptr;
    QSpinBox *widthBox = nullptr;
    QSpinBox *heightBox = nullptr;
};
