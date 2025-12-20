#include "imagewidget.h"
#include <QFile>
#include <QIODevice>
#include <QDebug>
#include <QPainter>


ImageWidget::ImageWidget(bool _isAfter, QWidget *parent) : QWidget(parent), isAfter(_isAfter)
{
}

void ImageWidget::paintEvent(QPaintEvent *event){
	QWidget::paintEvent(event);

	if (data == nullptr) return;

	int width = data->GetWidth();
	int height = data->GetHeight();
	sint* image_R = data->GetCurrentImage_R(isAfter);
	sint* image_G = data->GetCurrentImage_G(isAfter);
	sint* image_B = data->GetCurrentImage_B(isAfter);

	QPainter painter(this);

	uchar* data = new uchar[width * height * 3];
	for (int i = 0; i < width * height; i++){
		sint x = image_R[i];
		sint y = image_G[i];
		sint z = image_B[i];
		if (x > 255) x = 255;
		if (x < 0) x = 0;
		if (y > 255) y = 255;
		if (y < 0) y = 0;
		if (z > 255) z = 255;
		if (z < 0) z = 0;

		data[i * 3] = static_cast<uchar>(x);
		data[i * 3 + 1] = static_cast<uchar>(y);
		data[i * 3 + 2] = static_cast<uchar>(z);
	}

	QImage im(data, width, height, width*3, QImage::Format_RGB888);

	painter.setPen(QPen(Qt::red, 3));
	painter.drawImage(QRectF(0, 0, width, height), im);

	painter.end();
	delete[] data;
}

void ImageWidget::setTime(int t){
	time = t;
	update();
}

void ImageWidget::update(){
	QWidget::update();
	int w = data->GetWidth();
	int h = data->GetHeight();
	setMaximumSize(w, h);
	setMinimumSize(w, h);
}

void ImageWidget::setData(VideoRecolor* value){
	data = value;
	connect(value, &VideoRecolor::updated, this, &ImageWidget::update);
}