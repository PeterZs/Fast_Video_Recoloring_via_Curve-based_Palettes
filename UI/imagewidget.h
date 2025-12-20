#ifndef ImageWidget_H
#define ImageWidget_H

#include <QWidget>
#include <QString>
#include "../main_algorithm.h"

class ImageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImageWidget(bool _isAfter, QWidget *parent = nullptr);

    void setData(VideoRecolor* value);

protected:
    virtual void paintEvent(QPaintEvent *event);
signals:

public slots:
    void setTime(int t);
    void update();

protected:
    VideoRecolor* data = nullptr;
    int time = 0;

private:
    bool isAfter;
};

#endif // ImageWidget_H
