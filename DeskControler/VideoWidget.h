#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QImage>
#include <QMouseEvent>
#include <QKeyEvent>


enum MouseMask {
	MouseMove = 0x01, // 鼠标移动
	MouseLeftDown = 0x02, // 鼠标左键按下
	MouseLeftUp = 0x04, // 鼠标左键释放
	MouseDoubleClick = 0x08, // 鼠标左键双击
	MouseRightClick = 0x10, // 鼠标右键单击
	MouseMiddleClick = 0x20  // 鼠标中键单击
};

class VideoWidget : public QOpenGLWidget {
	Q_OBJECT
public:
	explicit VideoWidget(QWidget* parent = nullptr);

signals:
	void mouseEventCaptured(int x, int y, int mask);
	void keyEventCaptured(int key, bool pressed);

public slots:
	void setFrame(const QImage& image);

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event)	 override;

	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;


private:
	QImage currentFrame;
	bool m_firstFrame = true;
};

#endif // VIDEOWIDGET_H
