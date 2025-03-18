#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QObject>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QDateTime>
#include <QMutex>
#include <QFile>
#include <QTextStream>

class LogWidget : public QWidget
{
	Q_OBJECT
public:
	// ��־����ö��
	enum LogLevel {
		Info,
		Warning,
		Error
	};

	// �������ʷ���
	static LogWidget* instance()
	{
		static QMutex mutex;
		QMutexLocker locker(&mutex);
		if (!m_instance) {
			m_instance = new LogWidget();
		}
		return m_instance;
	}

	// ��ʼ�����������ø�����
	void init(QWidget* parent);

	// �����־����ʾ�� UI ͬʱд���ļ�
	void addLog(const QString& logMessage, LogLevel level = Info);

	// ���ÿ�������͸�ֵ
	LogWidget(const LogWidget&) = delete;
	LogWidget& operator=(const LogWidget&) = delete;

private:
	// ˽�й��캯��
	explicit LogWidget();
	// ����ʱ�ر���־�ļ�
	~LogWidget() {
		if (m_logFile) {
			m_logFile->close();
		}
	}

	// ����ʵ��
	static LogWidget* m_instance;

	// UI���
	QTextEdit* logEdit;
	// ��־�ļ�����
	QFile* m_logFile;
};

#endif // LOGWIDGET_H
