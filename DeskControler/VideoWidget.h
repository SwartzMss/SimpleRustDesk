#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QImage>
#include <QMutex>

class VideoWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	explicit VideoWidget(QWidget* parent = nullptr);
	// …Ë÷√“ª÷°ÕºœÒ
	void setFrame(const QImage& image);
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	QImage currentFrame;
	QMutex mutex;
};

#endif // VIDEOWIDGET_H
