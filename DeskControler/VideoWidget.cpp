#include "VideoWidget.h"
#include <QPainter>
#include <QKeyEvent>
#include "LogWidget.h"

VideoWidget::VideoWidget(QWidget* parent)
	: QOpenGLWidget(parent)
{
	setWindowFlags(Qt::Window);
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
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


void VideoWidget::mousePressEvent(QMouseEvent* event)
{
	int mask = 0;
	if (event->button() == Qt::LeftButton)
		mask = MouseLeftDown;
	else if (event->button() == Qt::RightButton)
		mask = MouseRightClick;
	else if (event->button() == Qt::MiddleButton)
		mask = MouseMiddleClick;

	QPointF pos = event->position();
	emit mouseEventCaptured(static_cast<int>(pos.x()), static_cast<int>(pos.y()), mask);
}

void VideoWidget::mouseReleaseEvent(QMouseEvent* event)
{
	int mask = 0;
	if (event->button() == Qt::LeftButton)
		mask = MouseLeftUp;

	QPointF pos = event->position();
	emit mouseEventCaptured(static_cast<int>(pos.x()), static_cast<int>(pos.y()), mask);
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	int mask = MouseDoubleClick;
	const QPointF pos = event->position();
	emit mouseEventCaptured(static_cast<int>(pos.x()), static_cast<int>(pos.y()), mask);
}

void VideoWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {
		QPointF pos = event->position();
		emit mouseEventCaptured(static_cast<int>(pos.x()), static_cast<int>(pos.y()), MouseMove);
	}
}


void VideoWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat()) return;
	LogWidget::instance()->addLog("keyPressEvent ", LogWidget::Warning);
	emit keyEventCaptured(event->key(), true);
}

void VideoWidget::keyReleaseEvent(QKeyEvent* event)
{
	LogWidget::instance()->addLog("keyReleaseEvent ", LogWidget::Warning);
	if (event->isAutoRepeat()) return;
	emit keyEventCaptured(event->key(), false);
}