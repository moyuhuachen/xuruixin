#ifndef CPPQTCES1_H
#define CPPQTCES1_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QListWidget>
#include <QPushButton>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDebug>
#include <QFileDialog>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QSplitter>
#include <QColorDialog>
#include <QScrollBar>
#include "gdal_priv.h"
#include "ogr_spatialref.h"

class GraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    GraphicsView(QWidget* parent = nullptr);

signals:
    void viewTransformed();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool dragging; // 是否正在拖动
    QPoint lastMousePos; // 上一次鼠标位置
};

class CppQtCes1 : public QWidget
{
    Q_OBJECT

public:
    CppQtCes1(QWidget* parent = nullptr);
    ~CppQtCes1();

private slots:
    void browseShapefile();
    void loadShapefile(const QString& filename);
    void resetView();
    void showContextMenu(const QPoint& pos);
    void removeSelectedShapefile();
    void changeColor();
    void moveItemUp();
    void moveItemDown();
    void updatePointSizes();
    void updateGraphicsZOrder();

private:
    QListWidget* listWidget;
    QPushButton* browseButton;
    QPushButton* resetButton;
    GraphicsView* graphicsView;
    QGraphicsScene* scene;
    QColor currentColor;
    QMap<QString, QList<QGraphicsItem*>> loadedShapefiles;
};

#endif // CPPQTCES1_H
