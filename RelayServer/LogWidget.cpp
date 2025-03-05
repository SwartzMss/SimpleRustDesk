#include "LogWidget.h"

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
		layout->setContentsMargins(0, 0, 0, 0);  // 设置边距为 0
		layout->setSpacing(0);  // 设置间距为 0
		setLayout(layout);

		// 将 LogWidget 添加到父窗口的布局中
		if (!parent->layout()) {
			QVBoxLayout* parentLayout = new QVBoxLayout(parent);
			parentLayout->setContentsMargins(0, 0, 0, 0);  // 设置父窗口布局的边距为 0
			parentLayout->setSpacing(0);  // 设置父窗口布局的间距为 0
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

	QString timeStamp = QDateTime::currentDateTime().toString("hh:mm:ss");
	QString fullMessage = QString("[%1] %2").arg(timeStamp).arg(logMessage);

	QTextCursor cursor = logEdit->textCursor();
	cursor.movePosition(QTextCursor::End);
	cursor.insertText(fullMessage + "\n", format);
	logEdit->setTextCursor(cursor);
}