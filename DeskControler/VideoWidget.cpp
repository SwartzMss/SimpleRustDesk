#include "VideoWidget.h"
#include <QPainter>

VideoWidget::VideoWidget(QWidget* parent)
	: QOpenGLWidget(parent)
{
	setWindowFlags(Qt::Window);
}

void VideoWidget::setFrame(const QImage& image)
{
	currentFrame = image;

	// 首帧时设置固定尺寸（用于滚动区域）
	if (m_firstFrame && !currentFrame.isNull()) {
		setMinimumSize(currentFrame.size());
		resize(currentFrame.size());
		m_firstFrame = false;
	}

	update();
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	if (!currentFrame.isNull()) {
		painter.drawImage(rect(), currentFrame);
	}
	else {
		painter.fillRect(rect(), Qt::black);
	}
}
