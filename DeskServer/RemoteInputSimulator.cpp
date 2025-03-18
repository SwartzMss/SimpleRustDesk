#include "RemoteInputSimulator.h"
#include "LogWidget.h"
#include <Windows.h>
#include <QKeyEvent> 

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

	HWND targetHwnd = GetForegroundWindow();
	if (targetHwnd) {
		// 尝试激活前台窗口
		if (!SetForegroundWindow(targetHwnd)) {
			LogWidget::instance()->addLog("Failed to set foreground window in handleMouseEvent", LogWidget::Warning);
		}
	}
	else {
		LogWidget::instance()->addLog("No foreground window found in handleMouseEvent", LogWidget::Warning);
	}

	POINT point = { x, y };

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



WORD mapIntKeyToVK(int key)
{
	// 字母：Qt 的 Key_A ~ Key_Z 与 ASCII 'A'-'Z' 对应
	if (key >= Qt::Key_A && key <= Qt::Key_Z)
		return static_cast<WORD>(key);

	// 数字：Qt 的 Key_0 ~ Key_9 与 ASCII '0'-'9' 对应
	if (key >= Qt::Key_0 && key <= Qt::Key_9)
		return static_cast<WORD>(key);

	// 功能键：Qt::Key_F1 ~ Qt::Key_F12
	if (key >= Qt::Key_F1 && key <= Qt::Key_F12)
		return VK_F1 + (key - Qt::Key_F1);

	// 箭头键
	if (key == Qt::Key_Left)
		return VK_LEFT;
	if (key == Qt::Key_Up)
		return VK_UP;
	if (key == Qt::Key_Right)
		return VK_RIGHT;
	if (key == Qt::Key_Down)
		return VK_DOWN;

	// 常用特殊键
	if (key == Qt::Key_Space)
		return VK_SPACE;
	if (key == Qt::Key_Return || key == Qt::Key_Enter)
		return VK_RETURN;
	if (key == Qt::Key_Escape)
		return VK_ESCAPE;
	if (key == Qt::Key_Backspace)
		return VK_BACK;
	if (key == Qt::Key_Tab)
		return VK_TAB;
	if (key == Qt::Key_Shift)
		return VK_SHIFT;
	if (key == Qt::Key_Control)
		return VK_CONTROL;
	if (key == Qt::Key_Alt)
		return VK_MENU;
	if (key == Qt::Key_CapsLock)
		return VK_CAPITAL;
	if (key == Qt::Key_Insert)
		return VK_INSERT;
	if (key == Qt::Key_Delete)
		return VK_DELETE;
	if (key == Qt::Key_Home)
		return VK_HOME;
	if (key == Qt::Key_End)
		return VK_END;
	if (key == Qt::Key_PageUp)
		return VK_PRIOR; // Page Up
	if (key == Qt::Key_PageDown)
		return VK_NEXT;  // Page Down

	// 标点符号及 OEM 键（根据美式键盘）
	if (key == Qt::Key_Comma)
		return VK_OEM_COMMA;
	if (key == Qt::Key_Period)
		return VK_OEM_PERIOD;
	if (key == Qt::Key_Slash)
		return VK_OEM_2; // '/' 键
	if (key == Qt::Key_Semicolon)
		return VK_OEM_1; // ';:' 键
	if (key == Qt::Key_Apostrophe)
		return VK_OEM_7; // 单引号/双引号键
	if (key == Qt::Key_BracketLeft)
		return VK_OEM_4; // '[' 键
	if (key == Qt::Key_BracketRight)
		return VK_OEM_6; // ']' 键
	if (key == Qt::Key_Backslash)
		return VK_OEM_5; // '\' 键

	// 数字键盘（如果需要单独处理，可以检查 Qt::KeypadModifier 或专用键值）
	// 此处省略处理，通常发送的数字键为标准键盘数字

	// 其他未覆盖的键返回 0 表示无法映射
	return 0;
}

void RemoteInputSimulator::handleKeyboardEvent(int protoKey, bool pressed)
{

	HWND targetHwnd = GetForegroundWindow();
	if (targetHwnd) {
		// 尝试激活前台窗口
		if (!SetForegroundWindow(targetHwnd)) {
			LogWidget::instance()->addLog("Failed to set foreground window in handleMouseEvent", LogWidget::Warning);
		}
	}
	else {
		LogWidget::instance()->addLog("No foreground window found in handleMouseEvent", LogWidget::Warning);
	}

	// 将 protoKey 转换为 proto 枚举类型（ControlKey）
	WORD vk = mapIntKeyToVK(protoKey);
	if (vk == 0) {
		LogWidget::instance()->addLog("Unmapped key received in RemoteInputSimulator", LogWidget::Warning);
		return;
	}

	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = vk;
	input.ki.dwFlags = pressed ? 0 : KEYEVENTF_KEYUP;

	UINT sent = SendInput(1, &input, sizeof(INPUT));
	if (sent != 1) {
		LogWidget::instance()->addLog("SendInput failed in handleKeyboardEvent", LogWidget::Warning);
	}
}

