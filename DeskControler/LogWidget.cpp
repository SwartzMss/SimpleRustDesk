#include "LogWidget.h"
#include <QCoreApplication>
#include <QThread>

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
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		setLayout(layout);

		// �� LogWidget ��ӵ������ڵĲ�����
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

	// 1) ��ȡ��ǰ�߳�ID
	// �ж��Ƿ������̣߳�����ǰ�̵߳��� QApplication ���̣߳����Ϊ0������ȡ��ʵID
	quintptr threadIdVal = 0;
	if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
		threadIdVal = reinterpret_cast<quintptr>(QThread::currentThreadId());
	}
	// ���߳�IDת�����ַ���
	QString threadIdStr = QString::number(threadIdVal);

	// 2) ���ɴ�ʱ������߳�ID����־��
	QString timeStamp = QDateTime::currentDateTime().toString("hh:mm:ss");
	// ʹ��ռλ�������룺timeStampռ 8 ���ַ���ȣ�threadIdStrռ 5 ���ַ����
	// (Ҳ�ɸ����Լ�����΢�����)
	QString fullMessage = QString("[%1] [TID:%2] %3")
		.arg(timeStamp, -8)                // ����룬���8
		.arg(threadIdStr, 5, QLatin1Char(' ')) // �Ҷ��룬���5
		.arg(logMessage);

	// 3) �����ı��� QTextEdit
	QTextCursor cursor = logEdit->textCursor();
	cursor.movePosition(QTextCursor::End);
	cursor.insertText(fullMessage + "\n", format);
	logEdit->setTextCursor(cursor);
}
