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
			m_instance = new LogWidget(); // ���ٴ��� parent
		}
		return m_instance;
	}

	// ��ʼ�����������ø�����
	void init(QWidget* parent);

	// �����־
	void addLog(const QString& logMessage, LogLevel level = Info);

	// ɾ���������캯���͸�ֵ�����
	LogWidget(const LogWidget&) = delete;
	LogWidget& operator=(const LogWidget&) = delete;

private:
	// ˽�й��캯��
	explicit LogWidget(); // ������Ҫ parent ����
	~LogWidget() = default;

	// ����ʵ��
	static LogWidget* m_instance;

	// UI ���
	QTextEdit* logEdit;
};

#endif // LOGWIDGET_H