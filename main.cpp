#include <QApplication>
#include "cppqtces1.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    CppQtCes1 w;
    w.show();
    return a.exec();
}



//shan ge

////#include <QApplication>
////#include <QMainWindow>
////#include <QImage>
////#include <QFileDialog>
////#include <QLabel>
////#include <QMessageBox>
////#include <QMenuBar>
////#include <QAction>
////#include <QScrollArea>
////#include <QVBoxLayout>
////#include <gdal_priv.h>
////#include <vector>
////#include <QString>
////#include <QWidget>
////#include <QList>
////#include <QDebug>
////
////// 根据GDAL数据类型确定QImage格式
////QImage::Format determineQImageFormat(GDALDataType dataType) {
////    switch (dataType) {
////    case GDT_UInt16:
////    case GDT_Int16:
////        return QImage::Format_Grayscale16;
////    case GDT_UInt32:
////    case GDT_Int32:
////    case GDT_Float32:
////    case GDT_Float64:
////        return QImage::Format_RGB32;
////    case GDT_Byte:
////    default:
////        return QImage::Format_Grayscale8;
////    }
////}
////
////// 应用灰度映射到单波段灰度图像
////QImage applyGrayscaleMap(const QImage& grayImage) {
////    return grayImage.convertToFormat(QImage::Format_Grayscale8);
////}
////
////// 显示单波段图像
////void displaySingleBandImage(const QImage& bandImage, QWidget* parent) {
////    QImage grayscaleImage = applyGrayscaleMap(bandImage);
////
////    QLabel* imageLabel = new QLabel(parent);
////    imageLabel->setPixmap(QPixmap::fromImage(grayscaleImage));
////    imageLabel->setScaledContents(true);
////
////    QScrollArea* scrollArea = new QScrollArea(parent);
////    scrollArea->setWidget(imageLabel);
////    scrollArea->setAlignment(Qt::AlignCenter);
////
////    QWidget* container = new QWidget(parent);
////    QVBoxLayout* layout = new QVBoxLayout(container);
////    layout->addWidget(scrollArea);
////    container->setLayout(layout);
////    container->show();
////
////    QMainWindow* mainWindow = new QMainWindow();
////    mainWindow->setCentralWidget(container);
////    mainWindow->resize(grayscaleImage.size().width() + 20, grayscaleImage.size().height() + 70); // 留些边距
////    mainWindow->show();
////}
////
////// 显示多波段图像
////void displayMultiBandImage(const QList<QImage>& bandImages, QWidget* parent) {
////    if (bandImages.size() < 3) {
////        QMessageBox::information(parent, "Warning", "Not enough band images to display");
////        return;
////    }
////
////    int width = bandImages[0].width();
////    int height = bandImages[0].height();
////
////    QImage trueColorImage(width, height, QImage::Format_RGB32);
////
////    for (int y = 0; y < height; ++y) {
////        for (int x = 0; x < width; ++x) {
////            QRgb pixelValue = qRgb(
////                qRed(bandImages[0].pixel(x, y)), // Red
////                qGreen(bandImages[1].pixel(x, y)), // Green
////                qBlue(bandImages[2].pixel(x, y))  // Blue
////            );
////            trueColorImage.setPixel(x, y, pixelValue);
////        }
////    }
////
////    QLabel* imageLabel = new QLabel(parent);
////    imageLabel->setPixmap(QPixmap::fromImage(trueColorImage));
////    imageLabel->setScaledContents(true);
////
////    QScrollArea* scrollArea = new QScrollArea(parent);
////    scrollArea->setWidget(imageLabel);
////    scrollArea->setAlignment(Qt::AlignCenter);
////
////    QWidget* container = new QWidget(parent);
////    QVBoxLayout* layout = new QVBoxLayout(container);
////    layout->addWidget(scrollArea);
////    container->setLayout(layout);
////    container->show();
////
////    QMainWindow* mainWindow = new QMainWindow();
////    mainWindow->setCentralWidget(container);
////    mainWindow->resize(width, height);
////    mainWindow->show();
////}
////
////// 导入栅格数据
////void importRasterData(QWidget* parent) {
////    QString fileName = QFileDialog::getOpenFileName(parent, "Open Raster Data", "", "Raster files (*.tif *.tiff *.img *.bmp *.png *.jpg)");
////    qDebug() << "Selected file:" << fileName;
////
////    if (fileName.isEmpty()) {
////        QMessageBox::information(parent, "Info", "No file selected");
////        return;
////    }
////
////    GDALAllRegister(); // 注册GDAL驱动
////    GDALDataset* poDataset = (GDALDataset*)GDALOpen(fileName.toStdString().c_str(), GA_ReadOnly);
////
////    if (poDataset == nullptr) {
////        qDebug() << "Failed to open raster data file:" << CPLGetLastErrorMsg();
////        QMessageBox::critical(parent, "Error", "Failed to open raster data file");
////        return;
////    }
////
////    int nBands = poDataset->GetRasterCount(); // 获取波段数量
////    qDebug() << "Number of bands:" << nBands;
////
////    if (nBands < 1) {
////        QMessageBox::critical(parent, "Error", "Raster data file contains no bands");
////        GDALClose(poDataset);
////        return;
////    }
////
////    QList<QImage> bandImages;
////    for (int i = 1; i <= nBands; ++i) {
////        GDALRasterBand* poBand = poDataset->GetRasterBand(i);
////        int nXSize = poBand->GetXSize();
////        int nYSize = poBand->GetYSize();
////        GDALDataType dataType = poBand->GetRasterDataType();
////        QImage::Format imageFormat = determineQImageFormat(dataType);
////        QImage image(nXSize, nYSize, imageFormat);
////
////        // 检查数据类型并处理图像数据
////        if (dataType == GDT_Byte) {
////            std::vector<GByte> buffer(nXSize * nYSize);
////            poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, buffer.data(), nXSize, nYSize, GDT_Byte, 0, 0);
////            for (int y = 0; y < nYSize; ++y) {
////                for (int x = 0; x < nXSize; ++x) {
////                    image.setPixel(x, y, qRgb(buffer[y * nXSize + x], buffer[y * nXSize + x], buffer[y * nXSize + x]));
////                }
////            }
////        }
////        else {
////            poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, image.bits(), nXSize, nYSize, dataType, 0, 0);
////        }
////
////        bandImages.append(image);
////    }
////
////    GDALClose(poDataset);
////
////    if (nBands == 1) {
////        // 单波段数据，显示灰度图像
////        displaySingleBandImage(bandImages[0], parent);
////    }
////    else if (nBands >= 3) {
////        // 多波段数据，显示真彩色图像
////        displayMultiBandImage(bandImages, parent);
////    }
////    else {
////        QMessageBox::critical(parent, "Error", "Raster data file contains less than 3 bands");
////    }
////}
////
////// 主函数
////int main(int argc, char* argv[]) {
////    QApplication app(argc, argv);
////    QMainWindow mainWindow;
////
////    // 注册GDAL驱动
////    GDALAllRegister();
////
////    // 创建菜单栏和菜单项
////    QMenuBar* menuBar = new QMenuBar(&mainWindow);
////    QMenu* fileMenu = new QMenu("File", menuBar);
////    QAction* importAction = new QAction("Open Raster Data", fileMenu);
////
////    // 将菜单项添加到菜单
////    fileMenu->addAction(importAction);
////    menuBar->addMenu(fileMenu);
////    mainWindow.setMenuBar(menuBar);
////
////    // 连接菜单项的信号与槽
////    QObject::connect(importAction, &QAction::triggered, [&mainWindow]() {
////        importRasterData(&mainWindow);
////        });
////
////    // 显示主窗口
////    mainWindow.show();
////    return app.exec();
////}