#include "VideoWidget.h"
#include <QPainter>

VideoWidget::VideoWidget(QWidget* parent)
	: QOpenGLWidget(parent)
{
}

void VideoWidget::setFrame(const QImage& image)
{
	QMutexLocker locker(&mutex);
	currentFrame = image;
	update(); // ֪ͨ�ػ�
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	QMutexLocker locker(&mutex);
	if (!currentFrame.isNull()) {
		// ����������ͼ������Ӧ���ڴ�С
		QImage scaled = currentFrame.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		QPoint topLeft((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
		painter.drawImage(topLeft, scaled);
	}
	else {
		painter.fillRect(rect(), Qt::black);
	}
}
