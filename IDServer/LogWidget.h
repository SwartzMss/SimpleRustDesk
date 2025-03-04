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
			m_instance = new LogWidget(); // 不再传递 parent
		}
		return m_instance;
	}

	// 初始化函数，设置父窗口
	void init(QWidget* parent);

	// 添加日志
	void addLog(const QString& logMessage, LogLevel level = Info);

	// 删除拷贝构造函数和赋值运算符
	LogWidget(const LogWidget&) = delete;
	LogWidget& operator=(const LogWidget&) = delete;

private:
	// 私有构造函数
	explicit LogWidget(); // 不再需要 parent 参数
	~LogWidget() = default;

	// 单例实例
	static LogWidget* m_instance;

	// UI 组件
	QTextEdit* logEdit;
};

#endif // LOGWIDGET_H