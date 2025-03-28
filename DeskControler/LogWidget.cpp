#include "LogWidget.h"
#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QDateTime>
#include <QTextStream>

// Initialize static member variable
LogWidget* LogWidget::m_instance = nullptr;

LogWidget::LogWidget()
{
	// Initialize the UI log display widget
	logEdit = new QTextEdit(this);
	logEdit->setReadOnly(true);
	logEdit->setLineWrapMode(QTextEdit::NoWrap);

	// Initialize log file in append mode using the executable's name as base
	QString exeName = QFileInfo(QCoreApplication::applicationFilePath()).baseName();
	QString logFileName = exeName + ".log";
	m_logFile = new QFile(logFileName, this);
	if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
		qWarning() << "Failed to open log file!";
	}
}

void LogWidget::init(QWidget* parent)
{
	if (parent) {
		this->setParent(parent);  // Set parent window

		// Create layout with zero margins and spacing
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->addWidget(logEdit);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
		setLayout(layout);

		// Add LogWidget to the parent's layout
		if (!parent->layout()) {
			QVBoxLayout* parentLayout = new QVBoxLayout(parent);
			parentLayout->setContentsMargins(0, 0, 0, 0);
			parentLayout->setSpacing(0);
			parent->setLayout(parentLayout);
		}
		parent->layout()->addWidget(this);
	}
}


void LogWidget::appendLog(const QString& logMessage, LogLevel level, const QString& callerThreadId)
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
	QString fullMessage = QString("[%1] [TID:0x%2] %3")
		.arg(timeStamp, 8)
		.arg(callerThreadId)
		.arg(logMessage);

	QTextCursor cursor = logEdit->textCursor();
	cursor.movePosition(QTextCursor::End);
	cursor.insertText(fullMessage + "\n", format);
	logEdit->setTextCursor(cursor);

	if (m_logFile && m_logFile->isOpen()) {
		QTextStream out(m_logFile);
		out << fullMessage << "\n";
		m_logFile->flush();
	}
}

void LogWidget::addLog(const QString& logMessage, LogLevel level)
{
	QString callerThreadId = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);

	if (QThread::currentThread() != this->thread()) {
		QMetaObject::invokeMethod(this, [=]() {
			this->appendLog(logMessage, level, callerThreadId);
			}, Qt::QueuedConnection);
		return;
	}
	appendLog(logMessage, level, "0");
}
