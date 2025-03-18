#include "RemoteInputSimulator.h"
#include "LogWidget.h"
#include <Windows.h>

enum MouseMask {
	MouseMove = 0x01, // ����ƶ�
	MouseLeftDown = 0x02, // ����������
	MouseLeftUp = 0x04, // �������ͷ�
	MouseDoubleClick = 0x08, // ������˫��
	MouseRightClick = 0x10, // ����Ҽ�����
	MouseMiddleClick = 0x20  // ����м�����
};

RemoteInputSimulator::RemoteInputSimulator(QObject* parent)
	: QObject(parent)
{
}

void RemoteInputSimulator::handleMouseEvent(int x, int y, int mask)
{
	// ����ͻ��˷��͵��Ǵ������꣬��Ҫ����һ������ת��:
	POINT point = { x, y };

	// ��������Ѿ�����Ļ���꣬�Ͳ���Ҫ��һ���ˡ�
	// ClientToScreen(targetWindowHandle, &point); <-- ɾ�����

	int screenX = (65535 * x) / GetSystemMetrics(SM_CXSCREEN);
	int screenY = (65535 * y) / GetSystemMetrics(SM_CYSCREEN);

	std::vector<INPUT> inputs;

	INPUT moveInput = {};
	moveInput.type = INPUT_MOUSE;
	moveInput.mi.dx = screenX;
	moveInput.mi.dy = screenY;
	moveInput.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

	inputs.push_back(moveInput);

	if (mask & MouseLeftDown) {
		INPUT downInput = {};
		downInput.type = INPUT_MOUSE;
		downInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		inputs.push_back(downInput);
	}

	if (mask & MouseLeftUp) {
		INPUT upInput = {};
		upInput.type = INPUT_MOUSE;
		upInput.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		inputs.push_back(upInput);
	}

	if (mask & MouseRightClick) {
		INPUT downInput = {};
		downInput.type = INPUT_MOUSE;
		downInput.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

		INPUT upInput = {};
		upInput.type = INPUT_MOUSE;
		upInput.mi.dwFlags = MOUSEEVENTF_RIGHTUP;

		inputs.push_back(downInput);
		inputs.push_back(upInput);
	}

	if (mask & MouseMiddleClick) {
		INPUT downInput = {};
		downInput.type = INPUT_MOUSE;
		downInput.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;

		INPUT upInput = {};
		upInput.type = INPUT_MOUSE;
		upInput.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;

		inputs.push_back(downInput);
		inputs.push_back(upInput);
	}

	if (mask & MouseDoubleClick) {
		for (int i = 0; i < 2; ++i) {
			handleMouseEvent(x, y, MouseLeftDown);
			Sleep(50);
			handleMouseEvent(x, y, MouseLeftUp);
		}
		return;
	}

	if (!inputs.empty()) {
		SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
	}
}


