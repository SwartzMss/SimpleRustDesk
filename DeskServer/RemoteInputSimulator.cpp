#include "RemoteInputSimulator.h"
#include "LogWidget.h"
#include <Windows.h>

enum MouseMask {
	MouseMove = 0x01, // 鼠标移动
	MouseLeftDown = 0x02, // 鼠标左键按下
	MouseLeftUp = 0x04, // 鼠标左键释放
	MouseDoubleClick = 0x08, // 鼠标左键双击
	MouseRightClick = 0x10, // 鼠标右键单击
	MouseMiddleClick = 0x20  // 鼠标中键单击
};

RemoteInputSimulator::RemoteInputSimulator(QObject* parent)
	: QObject(parent)
{
}

void RemoteInputSimulator::handleMouseEvent(int x, int y, int mask)
{
	// 如果客户端发送的是窗口坐标，需要进行一次坐标转换:
	POINT point = { x, y };

	// 如果坐标已经是屏幕坐标，就不需要这一句了。
	// ClientToScreen(targetWindowHandle, &point); <-- 删除这句

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


