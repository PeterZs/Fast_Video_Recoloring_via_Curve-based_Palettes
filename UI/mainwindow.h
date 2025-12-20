#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../main_algorithm.h"
#include <QSlider>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
	int isplaying;
	QDockWidget *imageBeforeDockWidget = nullptr;
	QDockWidget *imageAfterDockWidget = nullptr;
	void exportVideo();
    void exportCurrentFrame();
    void openVideo(bool merge);
    void exportEditedFramePalettes();
    void importEditedFramePalettes();

    VideoRecolor* data = nullptr;
    QSlider *slider = nullptr;
    QSlider *mergeStepSlider = nullptr;
	
    // QString title = "Video Recoloring Tool (Build " __DATE__ " " __TIME__ ")";
	QString title = "Video Recoloring Tool";

	bool isPlaying = false;
};

#endif // MAINWINDOW_H
