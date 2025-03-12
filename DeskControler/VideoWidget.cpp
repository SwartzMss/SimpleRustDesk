#include "VideoWidget.h"
#include <QPainter>

VideoWidget::VideoWidget(QWidget* parent)
	: QOpenGLWidget(parent)
{
	// [MOD] 设置为独立窗口，使 VideoWidget 自己创建窗口
	setWindowFlags(Qt::Window);
}

void VideoWidget::setFrame(const QImage& image)
{
	QMutexLocker locker(&mutex);
	currentFrame = image;
	// [MOD] 根据解码后数据（图像）的尺寸调整窗口大小
	if (!currentFrame.isNull()) {
		// 可根据需要增加一些边距，比如+0或者其他值
		resize(currentFrame.width(), currentFrame.height());
	}
	update(); // 通知重绘
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	QMutexLocker locker(&mutex);
	if (!currentFrame.isNull()) {
		// 按比例缩放图像以适应窗口大小
		QImage scaled = currentFrame.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		QPoint topLeft((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
		painter.drawImage(topLeft, scaled);
	}
	else {
		painter.fillRect(rect(), Qt::black);
	}
}
