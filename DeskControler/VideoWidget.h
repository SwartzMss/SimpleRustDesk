#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QImage>

class VideoWidget : public QOpenGLWidget {
	Q_OBJECT
public:
	explicit VideoWidget(QWidget* parent = nullptr);

public slots:
	void setFrame(const QImage& image);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QImage currentFrame;
	bool m_firstFrame = true;
};

#endif // VIDEOWIDGET_H
