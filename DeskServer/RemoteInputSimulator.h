#pragma once

#include <QObject>
#include <Windows.h>

class RemoteInputSimulator : public QObject
{
	Q_OBJECT
public:
	explicit RemoteInputSimulator(QObject* parent = nullptr);

	void handleMouseEvent(int x, int y, int mask);
	void handleKeyboardEvent(int key, bool pressed);
};
