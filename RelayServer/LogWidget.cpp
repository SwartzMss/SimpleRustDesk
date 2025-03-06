#include "LogWidget.h"

// ��ʼ����̬��Ա����
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
		this->setParent(parent);  // ���ø�����

		// �������ֲ����ñ߾�ͼ��Ϊ 0
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->addWidget(logEdit);
		layout->setContentsMargins(0, 0, 0, 0);  // ���ñ߾�Ϊ 0
		layout->setSpacing(0);  // ���ü��Ϊ 0
		setLayout(layout);

		// �� LogWidget ��ӵ������ڵĲ�����
		if (!parent->layout()) {
			QVBoxLayout* parentLayout = new QVBoxLayout(parent);
			parentLayout->setContentsMargins(0, 0, 0, 0);  // ���ø����ڲ��ֵı߾�Ϊ 0
			parentLayout->setSpacing(0);  // ���ø����ڲ��ֵļ��Ϊ 0
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