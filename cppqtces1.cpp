#include "cppqtces1.h"
#include "gdal.h"
#include "ogrsf_frmts.h"
#include "cpl_conv.h"

GraphicsView::GraphicsView(QWidget* parent)
    : QGraphicsView(parent), dragging(false) {}

void GraphicsView::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() > 0) {
        scale(1.25, 1.25);
    }
    else {
        scale(0.8, 0.8);
    }
    emit viewTransformed();
}

void GraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging) {
        QPointF delta = mapToScene(event->pos()) - mapToScene(lastMousePos);
        lastMousePos = event->pos();
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        translate(delta.x(), delta.y());
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        emit viewTransformed();
    }
    QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging = false;
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

CppQtCes1::CppQtCes1(QWidget* parent)
    : QWidget(parent)
{
    // 创建文件列表和按钮
    listWidget = new QListWidget(this);
    browseButton = new QPushButton("浏览Shapefile", this);
    resetButton = new QPushButton("复位", this);

    // 连接按钮信号和槽函数
    connect(browseButton, &QPushButton::clicked, this, &CppQtCes1::browseShapefile);
    connect(resetButton, &QPushButton::clicked, this, &CppQtCes1::resetView);

    // 创建按钮布局
    QVBoxLayout* buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(browseButton);
    buttonLayout->addWidget(resetButton);

    // 创建文件布局
    QVBoxLayout* fileLayout = new QVBoxLayout;
    fileLayout->addLayout(buttonLayout);
    fileLayout->addWidget(listWidget);

    QWidget* fileWidget = new QWidget(this);
    fileWidget->setLayout(fileLayout);

    // 创建图形视图和场景
    graphicsView = new GraphicsView(this);
    scene = new QGraphicsScene(this);
    graphicsView->setScene(scene);
    graphicsView->setRenderHint(QPainter::Antialiasing);

    scene->setBackgroundBrush(Qt::lightGray);
    scene->setSceneRect(-500, -500, 1000, 1000);

    QTransform transform;
    transform.scale(1, -1);
    graphicsView->setTransform(transform);

    // 创建分割器，用于调整文件列表和图形视图的大小比例
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(fileWidget);
    splitter->addWidget(graphicsView);
    splitter->setStretchFactor(1, 3);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter);
    setLayout(mainLayout);

    // 设置右键菜单
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, &QListWidget::customContextMenuRequested, this, &CppQtCes1::showContextMenu);

    // 连接视图变换信号到 updatePointSizes 槽函数
    connect(graphicsView, &GraphicsView::viewTransformed, this, &CppQtCes1::updatePointSizes);
}

CppQtCes1::~CppQtCes1()
{
    // 清理内存
}

void CppQtCes1::browseShapefile()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this,
        tr("打开Shapefile"),
        "/path/to/your/default/directory",
        tr("Shapefiles (*.shp)"));
    if (!filenames.isEmpty()) {
        for (const QString& filename : filenames) {
            currentColor = QColorDialog::getColor(Qt::red, this, tr("选择颜色"));
            if (currentColor.isValid()) {
                loadShapefile(filename);
            }
            else {
                QMessageBox::warning(this, tr("警告"), tr("无效颜色选择，未加载Shapefile。"));
            }
        }
    }
}

void CppQtCes1::updatePointSizes()
{
    QTransform viewTransform = graphicsView->transform();
    double scale = viewTransform.m11(); // 取X方向的缩放比例

    for (QGraphicsItem* item : scene->items()) {
        if (QGraphicsEllipseItem* ellipse = dynamic_cast<QGraphicsEllipseItem*>(item)) {
            double initialRadius = ellipse->data(0).toDouble();
            double adjustedRadius = initialRadius / scale;

            QRectF rect = ellipse->rect();
            QPointF center = rect.center();
            ellipse->setRect(
                center.x() - adjustedRadius,
                center.y() - adjustedRadius,
                2 * adjustedRadius,
                2 * adjustedRadius
            );
        }
    }
}

// 加载Shapefile文件
void CppQtCes1::loadShapefile(const QString& filename)
{
    if (loadedShapefiles.contains(filename)) {
        QMessageBox::information(this, tr("提示"), tr("Shapefile已经加载。"));
        return;
    }

    GDALAllRegister();

    GDALDataset* poDS = (GDALDataset*)GDALOpenEx(filename.toStdString().c_str(),
        GDAL_OF_VECTOR,
        nullptr, nullptr, nullptr);

    if (poDS == nullptr) {
        qDebug() << "打开Shapefile失败：" << CPLGetLastErrorMsg();
        QMessageBox::critical(this, tr("错误"), tr("打开Shapefile失败，请检查文件路径或文件格式。"));
        return;
    }

    // 获取Shapefile文件的坐标系信息
    OGRSpatialReference* sourceSRS = nullptr;
    if (poDS->GetLayerCount() > 0) {
        sourceSRS = poDS->GetLayer(0)->GetSpatialRef();
    }

    // 创建目标坐标系（WGS84）
    OGRSpatialReference targetSRS;
    targetSRS.SetWellKnownGeogCS("WGS84");

    // 创建坐标变换对象
    OGRCoordinateTransformation* coordTransform = nullptr;
    if (sourceSRS) {
        coordTransform = OGRCreateCoordinateTransformation(sourceSRS, &targetSRS);
        if (coordTransform == nullptr) {
            qDebug() << "创建坐标变换对象失败";
            GDALClose(poDS);
            return;
        }
    }

    int layerCount = poDS->GetLayerCount();
    if (layerCount == 0) {
        qDebug() << "Shapefile中没有图层。";
        QMessageBox::information(this, tr("提示"), tr("Shapefile中没有图层。"));
        GDALClose(poDS);
        return;
    }

    QList<QGraphicsItem*> items;  // 存储当前shp文件的图形项
    QRectF boundingRect;

    for (int i = 0; i < layerCount; ++i) {
        OGRLayer* poLayer = poDS->GetLayer(i);
        if (poLayer == nullptr) {
            qDebug() << "获取图层失败：" << i;
            continue;
        }

        QString layerInfo = QString("图层 %1: 名称=%2, 要素数量=%3")
            .arg(i + 1)
            .arg(poLayer->GetName())
            .arg(poLayer->GetFeatureCount());
        qDebug() << layerInfo;

        QListWidgetItem* item = new QListWidgetItem(layerInfo);
        item->setData(Qt::UserRole, filename);
        listWidget->addItem(item);

        poLayer->ResetReading();
        OGRFeature* poFeature;
        while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
            OGRGeometry* poGeometry = poFeature->GetGeometryRef();
            if (poGeometry == nullptr) {
                OGRFeature::DestroyFeature(poFeature);
                continue;
            }

            // 将几何对象转换到目标坐标系
            if (coordTransform) {
                if (poGeometry->transform(coordTransform) != OGRERR_NONE) {
                    qDebug() << "几何对象坐标转换失败";
                    OGRFeature::DestroyFeature(poFeature);
                    continue;
                }
            }

            OGRwkbGeometryType geometryType = wkbFlatten(poGeometry->getGeometryType());
            switch (geometryType) {
            case wkbPoint: {
                OGRPoint* poPoint = (OGRPoint*)poGeometry;
                QPointF point(poPoint->getX(), poPoint->getY());

                // 固定点的半径，不随视图缩放而变化
                double fixedRadius = 5.0; // 可以根据需要调整

                // 创建椭圆形的图形项表示点，并调整大小
                QGraphicsEllipseItem* pointItem = new QGraphicsEllipseItem(
                    point.x() - fixedRadius,
                    point.y() - fixedRadius,
                    2 * fixedRadius,
                    2 * fixedRadius
                );

                // 设置点的颜色和边界
                pointItem->setBrush(currentColor);

                // 设置边界颜色和宽度，并使线宽保持不变
                QPen pen(Qt::black, 1); // 初始边界宽度为1
                pen.setCosmetic(true); // 保持线宽度不变，即使缩放视图
                pointItem->setPen(pen);

                // 添加自定义属性来存储初始半径
                pointItem->setData(0, fixedRadius);

                scene->addItem(pointItem);
                items.append(pointItem);
                boundingRect = boundingRect.united(pointItem->sceneBoundingRect());  // 更新包围矩形
                qDebug() << "添加点: 坐标=" << point << "，半径=" << fixedRadius;
                break;
            }

            case wkbLineString: {
                OGRLineString* poLine = (OGRLineString*)poGeometry;
                QPainterPath path;
                for (int j = 0; j < poLine->getNumPoints(); ++j) {
                    QPointF point(poLine->getX(j), poLine->getY(j));
                    if (j == 0) {
                        path.moveTo(point);
                    }
                    else {
                        path.lineTo(point);
                    }
                }
                QGraphicsPathItem* lineItem = new QGraphicsPathItem(path);
                QPen pen(currentColor, 3);
                pen.setCosmetic(true);
                lineItem->setPen(pen);
                scene->addItem(lineItem);
                items.append(lineItem);
                boundingRect = boundingRect.united(lineItem->boundingRect());
                qDebug() << "添加线: 点数量=" << poLine->getNumPoints();
                break;
            }
            case wkbPolygon: {
                OGRPolygon* poPolygon = (OGRPolygon*)poGeometry;
                OGRLinearRing* poRing = poPolygon->getExteriorRing();
                QPolygonF polygon;
                for (int j = 0; j < poRing->getNumPoints(); ++j) {
                    QPointF point(poRing->getX(j), poRing->getY(j));
                    polygon << point;
                }
                QGraphicsPolygonItem* polygonItem = new QGraphicsPolygonItem(polygon);
                polygonItem->setBrush(currentColor);

                QPen pen(Qt::black, 2);
                pen.setCosmetic(true);
                polygonItem->setPen(pen);

                scene->addItem(polygonItem);
                items.append(polygonItem);
                boundingRect = boundingRect.united(polygonItem->boundingRect());
                qDebug() << "添加多边形: 点数量=" << polygon.size();

                for (int ringIdx = 0; ringIdx < poPolygon->getNumInteriorRings(); ++ringIdx) {
                    poRing = poPolygon->getInteriorRing(ringIdx);
                    polygon.clear();
                    for (int j = 0; j < poRing->getNumPoints(); ++j) {
                        QPointF point(poRing->getX(j), poRing->getY(j));
                        polygon << point;
                    }
                    QGraphicsPolygonItem* interiorPolygonItem = new QGraphicsPolygonItem(polygon);
                    interiorPolygonItem->setBrush(Qt::red);

                    QPen interiorPen(Qt::black, 2);
                    interiorPen.setCosmetic(true);
                    interiorPolygonItem->setPen(interiorPen);

                    scene->addItem(interiorPolygonItem);
                    items.append(interiorPolygonItem);
                    boundingRect = boundingRect.united(interiorPolygonItem->boundingRect());
                    qDebug() << "添加内部环: 点数量=" << polygon.size();
                }
                break;
            }

            default:
                qDebug() << "未处理的几何类型: " << poGeometry->getGeometryType();
                break;
            }

            OGRFeature::DestroyFeature(poFeature);
        }
    }

    loadedShapefiles.insert(filename, items);
    scene->setSceneRect(scene->itemsBoundingRect());  // 更新场景矩形
    graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    qDebug() << "场景矩形:" << scene->sceneRect();

    GDALClose(poDS);

    // 释放坐标变换对象
    if (coordTransform) {
        OGRCoordinateTransformation::DestroyCT(coordTransform);
    }
}

// 重置视图
void CppQtCes1::resetView()
{
    QTransform transform;
    transform.scale(1, -1);  // 保留Y轴反转
    graphicsView->setTransform(transform);
    graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

// 显示右键菜单
void CppQtCes1::showContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("右键菜单"), this);

    QAction actionRemove("移除Shapefile", this);
    connect(&actionRemove, &QAction::triggered, this, &CppQtCes1::removeSelectedShapefile);
    contextMenu.addAction(&actionRemove);

    QAction actionChangeColor("更改颜色", this);
    connect(&actionChangeColor, &QAction::triggered, this, &CppQtCes1::changeColor);
    contextMenu.addAction(&actionChangeColor);

    QAction actionMoveUp("上移", this);
    connect(&actionMoveUp, &QAction::triggered, this, &CppQtCes1::moveItemUp);
    contextMenu.addAction(&actionMoveUp);

    QAction actionMoveDown("下移", this);
    connect(&actionMoveDown, &QAction::triggered, this, &CppQtCes1::moveItemDown);
    contextMenu.addAction(&actionMoveDown);

    contextMenu.exec(mapToGlobal(pos));
}

// 移除选中的Shapefile
void CppQtCes1::removeSelectedShapefile()
{
    QListWidgetItem* item = listWidget->currentItem();
    if (item) {
        QString filename = item->data(Qt::UserRole).toString();
        if (loadedShapefiles.contains(filename)) {
            QList<QGraphicsItem*> items = loadedShapefiles.take(filename);
            for (QGraphicsItem* graphicsItem : items) {
                scene->removeItem(graphicsItem);
                delete graphicsItem;
            }
            delete item;
        }
    }
}

// 更改颜色
void CppQtCes1::changeColor()
{
    QListWidgetItem* item = listWidget->currentItem();
    if (item) {
        QString filename = item->data(Qt::UserRole).toString();
        if (loadedShapefiles.contains(filename)) {
            QColor newColor = QColorDialog::getColor(currentColor, this, tr("选择颜色"));
            if (newColor.isValid()) {
                QList<QGraphicsItem*> items = loadedShapefiles.value(filename);
                for (QGraphicsItem* graphicsItem : items) {
                    if (auto* ellipse = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
                        ellipse->setBrush(newColor);
                    }
                    else if (auto* line = dynamic_cast<QGraphicsPathItem*>(graphicsItem)) {
                        QPen pen = line->pen();
                        pen.setColor(newColor);
                        line->setPen(pen);
                    }
                    else if (auto* polygon = dynamic_cast<QGraphicsPolygonItem*>(graphicsItem)) {
                        polygon->setBrush(newColor);
                    }
                }
                currentColor = newColor;
            }
            else {
                QMessageBox::warning(this, tr("警告"), tr("无效颜色选择，颜色未更改。"));
            }
        }
    }
}

// 上移文件列表项
void CppQtCes1::moveItemUp()
{
    int currentIndex = listWidget->currentRow();
    if (currentIndex > 0) {
        QListWidgetItem* currentItem = listWidget->takeItem(currentIndex);
        listWidget->insertItem(currentIndex - 1, currentItem);
        listWidget->setCurrentItem(currentItem);
        updateGraphicsZOrder();
    }
}

// 下移文件列表项
void CppQtCes1::moveItemDown()
{
    int currentIndex = listWidget->currentRow();
    if (currentIndex < listWidget->count() - 1) {
        QListWidgetItem* currentItem = listWidget->takeItem(currentIndex);
        listWidget->insertItem(currentIndex + 1, currentItem);
        listWidget->setCurrentItem(currentItem);
        updateGraphicsZOrder();
    }
}

// 更新图形项层级
void CppQtCes1::updateGraphicsZOrder()
{
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem* item = listWidget->item(i);
        QString filename = item->data(Qt::UserRole).toString();
        if (loadedShapefiles.contains(filename)) {
            QList<QGraphicsItem*> items = loadedShapefiles.value(filename);
            for (QGraphicsItem* graphicsItem : items) {
                graphicsItem->setZValue(i);
            }
        }
    }
}
