#include "LogWidget.h"

// 初始化静态成员变量
LogWidget* LogWidget::m_instance = nullptr;

LogWidget::LogWidget()
{
	logEdit = new QTextEdit(this);
	logEdit->setReadOnly(true);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(logEdit);
	setLayout(layout);
}

void LogWidget::init(QWidget* parent)
{
	if (parent) {
		this->setParent(parent);
		QVBoxLayout* layout = new QVBoxLayout(parent);
		layout->addWidget(this);
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