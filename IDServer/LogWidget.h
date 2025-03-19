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
	// 日志级别枚举
	enum LogLevel {
		Info,
		Warning,
		Error
	};

	// 单例访问方法
	static LogWidget* instance()
	{
		static QMutex mutex;
		QMutexLocker locker(&mutex);
		if (!m_instance) {
			m_instance = new LogWidget();
		}
		return m_instance;
	}

	// 初始化函数，设置父窗口
	void init(QWidget* parent);

	// 添加日志，建议标记为 Q_INVOKABLE 以支持跨线程调用
	Q_INVOKABLE void addLog(const QString& logMessage, LogLevel level = Info);

	// 禁用拷贝构造和赋值
	LogWidget(const LogWidget&) = delete;
	LogWidget& operator=(const LogWidget&) = delete;

private:
	// 私有构造函数
	explicit LogWidget();
	// 析构时关闭日志文件
	~LogWidget() {
		if (m_logFile) {
			m_logFile->close();
		}
	}

	// 实际更新日志的内部函数
	void appendLog(const QString& logMessage, LogLevel level, const QString& callerThreadId);

	// 单例实例
	static LogWidget* m_instance;

	// UI组件
	QTextEdit* logEdit;
	// 日志文件对象
	QFile* m_logFile;
};

#endif // LOGWIDGET_H
