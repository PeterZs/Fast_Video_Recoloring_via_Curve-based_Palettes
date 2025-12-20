#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>
#include <Q3DSurface>
#include <QtDataVisualization>
#include <QDockWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QFileDialog>
#include <QCheckBox>
#include <QSplitter>
#include <QSizePolicy>
#include <QProgressDialog>
#include <QGroupBox>
#include "../main_algorithm.h"
#include <QLineEdit>
#include "mainwindow.h"
#include "imagewidget.h"
#include "openglwidget.h"
#include "paletteviewwidget.h"
#include "../Qt-Color-Widgets\include\QtColorWidgets\color_dialog.hpp"

// Setup UI

#include <QColorDialog>

using namespace color_widgets;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent){
    setWindowTitle(title);
    setGeometry(100, 100, 520, 520);

    data = new VideoRecolor();

    QWidget* mainWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout();

    //top main menu==============================================================================================
    QHBoxLayout* firstRowLayout = new QHBoxLayout();
    QPushButton* openVideoBtn = new QPushButton("Open Video");
    QPushButton* importPaletteBtn = new QPushButton("Import Palette");
    QPushButton* CalcVideoBtn = new QPushButton("Recolor");
    QPushButton* ResetBtn = new QPushButton("Reset");
    QPushButton* CalcPaletteBtn = new QPushButton("Extract Palette");;

    firstRowLayout->addWidget(openVideoBtn);
    firstRowLayout->addWidget(CalcPaletteBtn);
    firstRowLayout->addWidget(CalcVideoBtn);
    firstRowLayout->addWidget(ResetBtn);
    mainLayout->addLayout(firstRowLayout);
    //top main menu==============================================================================================

    //original image and recolored image=========================================================================
    ImageWidget* recolored_image = new ImageWidget(true);
    recolored_image->setData(data);

    ImageWidget* original_image = new ImageWidget(false);
    original_image->setData(data);

    imageBeforeDockWidget = new QDockWidget();
    imageBeforeDockWidget->setWidget(original_image);
    imageBeforeDockWidget->setWindowTitle("Original Image");
    addDockWidget(Qt::TopDockWidgetArea, imageBeforeDockWidget);
    imageBeforeDockWidget->setFloating(true);
    imageBeforeDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    imageBeforeDockWidget->setGeometry(760, 100, 400, 400);
    imageBeforeDockWidget->hide();

    imageAfterDockWidget = new QDockWidget();
    imageAfterDockWidget->setWidget(recolored_image);
    imageAfterDockWidget->setWindowTitle("Recolored Image");
    imageAfterDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::TopDockWidgetArea, imageAfterDockWidget);
    imageAfterDockWidget->setFloating(true);
    imageAfterDockWidget->setGeometry(760, 100, 400, 400);
    imageAfterDockWidget->hide();
    //original image and recolored image=========================================================================
    

    //secondRow==================================================================================
    QHBoxLayout* SliderRowLayout = new QHBoxLayout();
    QSlider* timeSlider = new QSlider(Qt::Horizontal);
    QLabel* newValue = new QLabel("0");
    QLabel* line = new QLabel("/");
    QLabel* maxValue = new QLabel("0");

    SliderRowLayout->addWidget(timeSlider);
    SliderRowLayout->addWidget(newValue);
    SliderRowLayout->addWidget(line);
    SliderRowLayout->addWidget(maxValue);
 

    //=========================================================================================
    QHBoxLayout* secondRowLayout = new QHBoxLayout();
    QPushButton* playBtn = new QPushButton("Auto Play");
    QPushButton* stopBtn = new QPushButton("Stop Play");
    QPushButton* exportVideoBtn = new QPushButton("Export Video");
    QPushButton* exportPaletteBtn = new QPushButton("Export Palette");
    secondRowLayout->addWidget(playBtn);
    secondRowLayout->addWidget(stopBtn);
    //=========================================================================================

    QHBoxLayout* timeWindowLayout = new QHBoxLayout();
    QSlider* timeWindowSlider = new QSlider(Qt::Horizontal);
    timeWindowSlider->setMinimum(-3000);
    timeWindowSlider->setMaximum(10000);

    slider = new QSlider(Qt::Horizontal);
    slider->setTickInterval(1);

    QPushButton* autoPlayButton = new QPushButton("Play");
    QTimer* autoPlayTimer = new QTimer();

    autoPlayTimer->setInterval(40);
    //image play and vis=========================================================================================
    QGroupBox* videoProgressGroupBox = new QGroupBox(tr("Import and Export"));
    QHBoxLayout* videoProgressLayout = new QHBoxLayout;

    videoProgressLayout->addWidget(importPaletteBtn);
    videoProgressLayout->addWidget(exportPaletteBtn);
    videoProgressLayout->addWidget(exportVideoBtn);
    videoProgressGroupBox->setLayout(videoProgressLayout);

    mainLayout->addWidget(videoProgressGroupBox);
    QGroupBox* groupBox = new QGroupBox(tr("Video progress"));
    QVBoxLayout* vbox = new QVBoxLayout;

    vbox->addLayout(SliderRowLayout); 
    vbox->addLayout(secondRowLayout);
    groupBox->setLayout(vbox);

    QDockWidget* dockPaletteWidget = new QDockWidget();
    PaletteViewWidget* paletteWidget = new PaletteViewWidget();
    paletteWidget->setMinimumSize(500, 150);
    paletteWidget->setData(data);

    dockPaletteWidget->setWidget(paletteWidget);
    dockPaletteWidget->setWindowTitle("Frame Palette");
    addDockWidget(Qt::RightDockWidgetArea, dockPaletteWidget);
    dockPaletteWidget->setFloating(true);
    dockPaletteWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

    slider->setMinimum(0);
    slider->setMaximum(0);

    color_widgets::ColorDialog* dialog = new color_widgets::ColorDialog();
    dialog->setWindowTitle("Color picker");

    QDockWidget* dockColorWidget = new QDockWidget();

    dockColorWidget->setWidget(dialog);
    dockColorWidget->setWindowTitle("Color picker");
    addDockWidget(Qt::RightDockWidgetArea, dockColorWidget);
    dockColorWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

    mainLayout->addWidget(groupBox);
    mainLayout->addWidget(dockPaletteWidget);
    mainLayout->addWidget(dockColorWidget);
    mainWidget->setLayout(mainLayout);
    this->setCentralWidget(mainWidget);

    connect(playBtn, &QPushButton::clicked, [=]() {
        int totalFrameNum = data->GetFrameCnt();
        double gap = 1.0 / data->GetFps();
        isplaying++;
        int nowisplaying = isplaying;
        for (int i = 0; i < totalFrameNum; i++) {
            if (isplaying != nowisplaying) break;
            timeSlider->setValue(i);
            data->ChangeFrameTime(i);
            newValue->setText(QString::number(i));
            QCoreApplication::processEvents();
            Sleep(gap * 1000);
        }
    });
    connect(stopBtn, &QPushButton::clicked, [=]() {isplaying=0;});
    connect(timeSlider, &QSlider::valueChanged, [=](const int& value) {data->ChangeFrameTime(value); newValue->setText(QString::number(value)); });
    connect(CalcVideoBtn, &QPushButton::clicked, [=]() {data->DeformBezierCurve(); });
    connect(CalcPaletteBtn, &QPushButton::clicked, [=]() { data->CalcVideoPalette();});
    connect(exportVideoBtn, &QPushButton::clicked, [=]() {this->exportVideo(); });
    connect(ResetBtn, &QPushButton::clicked, [=]() {data->ResetAllFramePalettes(); timeSlider->setValue(0); });

    connect(openVideoBtn, &QPushButton::clicked, [=]() {
        this->openVideo(true);
        timeSlider->setValue(0);
        timeSlider->setMaximum(data->GetFrameCnt() - 1);
        maxValue->setText(QString::number(data->GetFrameCnt() - 1));
    });

    connect(dialog, &ColorDialog::colorChanged, paletteWidget, &PaletteViewWidget::getColor);
    connect(paletteWidget, &PaletteViewWidget::setColor, dialog, &ColorDialog::setColor);
    connect(exportPaletteBtn, &QPushButton::clicked, [=]() { exportEditedFramePalettes();});
    connect(importPaletteBtn, &QPushButton::clicked, [=]() { importEditedFramePalettes();});
}

MainWindow::~MainWindow()
{
}

void MainWindow::openVideo(bool merge) {
    if (data == nullptr) return;

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("*.mp4 *.avi"));
    dialog.setViewMode(QFileDialog::Detail);

    if (dialog.exec()) {
        QStringList fileName = dialog.selectedFiles();
        for (auto s : fileName)
            data->OpenVideo(QString(s));
        imageBeforeDockWidget->show();
        imageAfterDockWidget->show();
    }
    else return;
}

void MainWindow::exportVideo() {
    if (data == nullptr) return;

    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::DirectoryOnly);
    QStringList dirName;
    if (fileDialog.exec() == QDialog::Accepted){
        dirName = fileDialog.selectedFiles();
        data->ExportRecoloredVideo(dirName[0]);
    }
}

void MainWindow::exportEditedFramePalettes() {
    if (data == nullptr) return;

    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::DirectoryOnly);
    QStringList dirName;
    if (fileDialog.exec() == QDialog::Accepted) {
        dirName = fileDialog.selectedFiles();
        data->ExportEditedFramePalettes(dirName[0]);
    }
}

void MainWindow::importEditedFramePalettes() {
    if (data == nullptr) return;

    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    QStringList dirName;
    if (fileDialog.exec() == QDialog::Accepted) {
        dirName = fileDialog.selectedFiles();
        data->ImportEditedFramePalettes(dirName[0]);
    }
}