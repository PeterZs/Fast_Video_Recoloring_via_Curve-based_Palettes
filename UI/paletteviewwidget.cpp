#include "paletteviewwidget.h"

#include<QTimer>
#include<QDebug>
#include<cmath>
#include<QMouseEvent>
#include<QCryptographicHash>
#include <QColorDialog>
#include <QMenu>
#include "qrgb.h"
#include "math.h"
#include <algorithm>
#include "Qt-Color-Widgets\include\QtColorWidgets\color_dialog.hpp"
using namespace std;
using namespace color_widgets;

// Widget shows a palette-time view

PaletteViewWidget::PaletteViewWidget(QWidget *parent) : OpenGLWidget(parent)
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [=](){ blink = !blink; update(); });
    timer->start(800);

	pMenu = new QMenu(this);
	QAction *pResetTask = new QAction(tr("reset this color"), this);
	QAction *pResetAllTask = new QAction(tr("reset all color"), this);
	QAction* pRemoveSelection = new QAction(tr("remove this selection"), this);
	pMenu->addAction(pResetTask);
	pMenu->addAction(pResetAllTask);
	pMenu->addAction(pRemoveSelection);

	connect(pResetTask, SIGNAL(triggered()), this, SLOT(ResetCurrFramePaletteColor()));
	connect(pResetAllTask, SIGNAL(triggered()), this, SLOT(ResetAllFramePalettes()));
	connect(pRemoveSelection, SIGNAL(triggered()), this, SLOT(removeSelection()));
}

void PaletteViewWidget::setTime(int t) {
	time = t;
	if (selected_vid != -1 ) {
		/*data->Recolor();*/
	}
}

void PaletteViewWidget::getColor(QColor c) {
	int r = qRed(c.rgb());
	int g = qGreen(c.rgb());
	int b = qBlue(c.rgb());
	
	if (selected_vid != -1) {
		data->SetPaletteColor(selected_vid, c);
		update();
		data->RecolorFrame(data->currFrameId);
	}
}

void PaletteViewWidget::paintGL()
{
    OpenGLWidget::paintGL();

    glScalef(scale, scale, scale);

	int w = width(), h = height();
	double aspect = w * 1.0 / h;

	//绘制白色矩形框
	/*
	glBegin(GL_LINES);

	for (int p = 0; p <= 1; p++)
	{
		double q = p - .5;
		glColor3f(1., 1., 1.);
		glVertex3f(q, -.5, 0.);
		glColor3f(1., 1., 1.);
		glVertex3f(q, .5, 0.);
		glColor3f(1., 1., 1.);
		glVertex3f(-.5, q, 0.);
		glColor3f(1., 1., 1.);
		glVertex3f(.5, q, 0.);
	}
	glEnd();
	*/
	if (!data->isPaletteCalc) return;
	int framePaletteSize = data->GetFramePaletteSize();

	double** oriPalette_R = data->GetOriginalPalette_R();
	double** oriPalette_G = data->GetOriginalPalette_G();
	double** oriPalette_B = data->GetOriginalPalette_B();
	double** curPalette_R = data->GetChangedPalette_R();
	double** curPalette_G = data->GetChangedPalette_G();
	double** curPalette_B = data->GetChangedPalette_B();
	int index = data->currFrameId;
	

	float space = 0.02;
	float recw = 0.06;
	float x = -0.5, y = 0.45, y1 = 0.1;
	palette_pos.clear();
	
	for (int i = 0; i < framePaletteSize; i++)
	{
		float or = oriPalette_R[i][index] / 255.0, og = oriPalette_G[i][index] / 255.0, ob = oriPalette_B[i][index] / 255.0;
		glColor3f(or , og, ob);
		glRectf(x, y, x + recw, y - recw*aspect);
		
		float cr = curPalette_R[i][index] / 255.0, cg = curPalette_G[i][index] / 255.0, cb = curPalette_B[i][index] / 255.0;

		if (selected_vid == i) {
			double ex = 0.006;
			glColor3f(0.8, 0, 0);
			//glRectf(x- ex, y1+ ex*aspect, x + recw+ ex, y1 - recw*aspect- ex*aspect);
			glRectf(x, y1-recw*aspect - 0.01*aspect, x + recw, y1 - recw*aspect - 0.02*aspect);

		}

		glColor3f(cr , cg, cb);
		glRectf(x, y1, x + recw, y1 - recw*aspect);

		palette_pos.push_back(cv::Vec3d(x + recw / 2, y1 - recw*aspect / 2, 0));

		x += recw + space;
	}
}

void PaletteViewWidget::mousePressEvent(QMouseEvent *ev){

	if (!data->isPaletteCalc) return;

	if (ev->button() == Qt::LeftButton) {
		//1. 坐标系变换，将屏幕坐标系 -> opengl坐标系
		int w = width(), h = height();
		double half_w = w / 2.0, half_h = h / 2.0;
		double new_x = (ev->x() - half_w) / half_w / scale;
		double new_y = -(ev->y() - half_h) / half_h / scale;

		double** curPalette_R = data->GetChangedPalette_R();
		double** curPalette_G = data->GetChangedPalette_G();
		double** curPalette_B = data->GetChangedPalette_B();
		int index = data->currFrameId;

		int framePaletteSize = data->GetFramePaletteSize();

		//2. 找到离鼠标点击最近的点
		double aspect = w*1.0 / h;
		float recw = 0.07;
		float rech = recw * aspect;

		for (int i = 0; i < framePaletteSize; i++)
		{
			double x = palette_pos[i][0];
			double y = palette_pos[i][1];

			double dis1 = fabs(x - new_x);
			double dis2 = fabs(y - new_y);

			if (dis1 < recw / 2 && dis2 < rech / 2) {
				selected_vid = i;

				double r = curPalette_R[i][index];
				double g = curPalette_G[i][index];
				double b = curPalette_B[i][index];

				QColor c;
				c.setRed(r);
				c.setGreen(g);
				c.setBlue(b);
				Q_EMIT setColor(c);
				break;
			}
		}
	}
	else if (ev->button() == Qt::RightButton) {
		pMenu->exec(cursor().pos());
	}
}

void PaletteViewWidget::ResetCurrFramePaletteColor() {
	if (selected_vid != -1) {
		data->ResetCurrFramePaletteColor(selected_vid);
		emit update();
	}
}

void PaletteViewWidget::ResetAllFramePalettes() {
	if (selected_vid != -1) {
		data->ResetCurrFrameAllPaletteColors();
		emit update();
	}
}

void PaletteViewWidget::removeSelection() {
	if (selected_vid != -1) {
		data->RemoveSelection(selected_vid);
		emit update();
	}
}