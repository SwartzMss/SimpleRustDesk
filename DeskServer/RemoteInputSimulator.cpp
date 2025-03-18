#include "RemoteInputSimulator.h"
#include "LogWidget.h"
#include <Windows.h>
#include <QKeyEvent> 

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

	HWND targetHwnd = GetForegroundWindow();
	if (targetHwnd) {
		// ���Լ���ǰ̨����
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
	// ��ĸ��Qt �� Key_A ~ Key_Z �� ASCII 'A'-'Z' ��Ӧ
	if (key >= Qt::Key_A && key <= Qt::Key_Z)
		return static_cast<WORD>(key);

	// ���֣�Qt �� Key_0 ~ Key_9 �� ASCII '0'-'9' ��Ӧ
	if (key >= Qt::Key_0 && key <= Qt::Key_9)
		return static_cast<WORD>(key);

	// ���ܼ���Qt::Key_F1 ~ Qt::Key_F12
	if (key >= Qt::Key_F1 && key <= Qt::Key_F12)
		return VK_F1 + (key - Qt::Key_F1);

	// ��ͷ��
	if (key == Qt::Key_Left)
		return VK_LEFT;
	if (key == Qt::Key_Up)
		return VK_UP;
	if (key == Qt::Key_Right)
		return VK_RIGHT;
	if (key == Qt::Key_Down)
		return VK_DOWN;

	// ���������
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

	// �����ż� OEM ����������ʽ���̣�
	if (key == Qt::Key_Comma)
		return VK_OEM_COMMA;
	if (key == Qt::Key_Period)
		return VK_OEM_PERIOD;
	if (key == Qt::Key_Slash)
		return VK_OEM_2; // '/' ��
	if (key == Qt::Key_Semicolon)
		return VK_OEM_1; // ';:' ��
	if (key == Qt::Key_Apostrophe)
		return VK_OEM_7; // ������/˫���ż�
	if (key == Qt::Key_BracketLeft)
		return VK_OEM_4; // '[' ��
	if (key == Qt::Key_BracketRight)
		return VK_OEM_6; // ']' ��
	if (key == Qt::Key_Backslash)
		return VK_OEM_5; // '\' ��

	// ���ּ��̣������Ҫ�����������Լ�� Qt::KeypadModifier ��ר�ü�ֵ��
	// �˴�ʡ�Դ���ͨ�����͵����ּ�Ϊ��׼��������

	// ����δ���ǵļ����� 0 ��ʾ�޷�ӳ��
	return 0;
}

void RemoteInputSimulator::handleKeyboardEvent(int protoKey, bool pressed)
{

	HWND targetHwnd = GetForegroundWindow();
	if (targetHwnd) {
		// ���Լ���ǰ̨����
		if (!SetForegroundWindow(targetHwnd)) {
			LogWidget::instance()->addLog("Failed to set foreground window in handleMouseEvent", LogWidget::Warning);
		}
	}
	else {
		LogWidget::instance()->addLog("No foreground window found in handleMouseEvent", LogWidget::Warning);
	}

	// �� protoKey ת��Ϊ proto ö�����ͣ�ControlKey��
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

