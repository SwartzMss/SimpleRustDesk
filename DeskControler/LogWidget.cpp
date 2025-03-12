#include "LogWidget.h"
#include <QCoreApplication>
#include <QThread>

// 初始化静态成员变量
LogWidget* LogWidget::m_instance = nullptr;

LogWidget::LogWidget()
{
	logEdit = new QTextEdit(this);
	logEdit->setReadOnly(true);
	logEdit->setLineWrapMode(QTextEdit::NoWrap);
}

void LogWidget::init(QWidget* parent)
{
	if (parent) {
		this->setParent(parent);  // 设置父窗口

		// 创建布局并设置边距和间距为 0
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->addWidget(logEdit);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		setLayout(layout);

		// 将 LogWidget 添加到父窗口的布局中
		if (!parent->layout()) {
			QVBoxLayout* parentLayout = new QVBoxLayout(parent);
			parentLayout->setContentsMargins(0, 0, 0, 0);
			parentLayout->setSpacing(0);
			parent->setLayout(parentLayout);
		}
		parent->layout()->addWidget(this);
	}
}

void LogWidget::addLog(const QString& logMessage, LogLevel level)
{
	QTextCharFormat format;
	switch (level) {
	case Info:
		format.setForeground(Qt::black);
		break;
	case Warning:
		format.setForeground(Qt::red);
		break;
	case Error:
		format.setForeground(Qt::darkRed);
		break;
	default:
		format.setForeground(Qt::black);
		break;
	}

	// 1) 获取当前线程ID
	// 判断是否是主线程：若当前线程等于 QApplication 主线程，则记为0，否则取真实ID
	quintptr threadIdVal = 0;
	if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
		threadIdVal = reinterpret_cast<quintptr>(QThread::currentThreadId());
	}
	// 将线程ID转换成字符串
	QString threadIdStr = QString::number(threadIdVal);

	// 2) 生成带时间戳、线程ID的日志行
	QString timeStamp = QDateTime::currentDateTime().toString("hh:mm:ss");
	// 使用占位符来对齐：timeStamp占 8 个字符宽度，threadIdStr占 5 个字符宽度
	// (也可根据自己需求微调宽度)
	QString fullMessage = QString("[%1] [TID:%2] %3")
		.arg(timeStamp, -8)                // 左对齐，宽度8
		.arg(threadIdStr, 5, QLatin1Char(' ')) // 右对齐，宽度5
		.arg(logMessage);

	// 3) 插入文本到 QTextEdit
	QTextCursor cursor = logEdit->textCursor();
	cursor.movePosition(QTextCursor::End);
	cursor.insertText(fullMessage + "\n", format);
	logEdit->setTextCursor(cursor);
}
