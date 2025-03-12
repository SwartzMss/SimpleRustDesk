#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QImage>
#include <QMutex>
#include <QMutexLocker> // [MOD] 添加头文件以支持 QMutexLocker

class VideoWidget : public QOpenGLWidget {
	Q_OBJECT

public:
	// 构造函数保持不变，parent 默认 nullptr
	explicit VideoWidget(QWidget* parent = nullptr);
	// 设置视频帧，更新显示，并根据图像大小调整窗口尺寸 [MOD]
	void setFrame(const QImage& image);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QImage currentFrame;
	QMutex mutex;
};

#endif // VIDEOWIDGET_H
