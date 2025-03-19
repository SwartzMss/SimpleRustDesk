#pragma once

#include <QObject>
#include <Windows.h>
#include <QThread>

class RemoteInputSimulator : public QObject
{
	Q_OBJECT
public:
	explicit RemoteInputSimulator(QObject* parent = nullptr);

public slots:
	void handleMouseEvent(int x, int y, int mask);
	void handleKeyboardEvent(int key, bool pressed);


private:
	QThread* m_workerThread; 
};
