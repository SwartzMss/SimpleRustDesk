#include "RemoteClipboard.h"
#include "LogWidget.h"
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QtCore/QMimeData>

HHOOK RemoteClipboard::s_hook = nullptr;
RemoteClipboard* RemoteClipboard::s_instance = nullptr;

RemoteClipboard::RemoteClipboard(QObject* parent)
	: QObject(parent)
{
	s_instance = this;
}

RemoteClipboard::~RemoteClipboard()
{
	stop();
	s_instance = nullptr;
}

bool RemoteClipboard::start()
{
	if (!s_hook) {
		s_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
		if (!s_hook) {
			LogWidget::instance()->addLog("RemoteClipboard: Failed to install global keyboard hook", LogWidget::Error);
			return false;
		}
		LogWidget::instance()->addLog("RemoteClipboard: Global keyboard hook installed", LogWidget::Info);
	}
	return true;
}

void RemoteClipboard::stop()
{
	if (s_hook) {
		UnhookWindowsHookEx(s_hook);
		s_hook = nullptr;
		LogWidget::instance()->addLog("RemoteClipboard: Global keyboard hook removed", LogWidget::Info);
	}
}

void RemoteClipboard::setRemoteWindow(QWidget* remoteWindow)
{
	m_remoteWindow = remoteWindow;
}

LRESULT CALLBACK RemoteClipboard::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (s_instance) {
		return s_instance->handleKeyEvent(nCode, wParam, lParam);
	}
	return CallNextHookEx(s_hook, nCode, wParam, lParam);
}

LRESULT RemoteClipboard::handleKeyEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
		// 仅处理按下事件
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			if (ctrlPressed && pKeyboard->vkCode == 'C') {
				QWidget* activeWnd = QApplication::activeWindow();
				if (m_remoteWindow && activeWnd == m_remoteWindow) {
					// 如果激活窗口是远程桌面，则不处理 Ctrl+C，交由远程桌面处理
					LogWidget::instance()->addLog("Remote window active, ignoring Ctrl+C on control side", LogWidget::Info);
					return CallNextHookEx(s_hook, nCode, wParam, lParam);
				}
				// 读取剪贴板数据，并构造 ClipboardEvent
				ClipboardEvent eventMsg;
				QClipboard* clipboard = QApplication::clipboard();
				const QMimeData* mimeData = clipboard->mimeData();
				if (mimeData) {
					// 如果剪贴板包含文件 URL，则优先处理文件数据
					if (mimeData->hasUrls() && !mimeData->urls().isEmpty()) {
						QString filePath = mimeData->urls().first().toLocalFile();
						QFile file(filePath);
						if (file.open(QIODevice::ReadOnly)) {
							QByteArray data = file.readAll();
							file.close();
							FileContent* fileContent = eventMsg.mutable_file();
							fileContent->set_file_data(data.toStdString());
							QFileInfo fileInfo(filePath);
							fileContent->set_file_name(fileInfo.fileName().toStdString());
							LogWidget::instance()->addLog(QString("RemoteClipboard: Captured file data: %1").arg(filePath), LogWidget::Info);
						}
						else {
							LogWidget::instance()->addLog(QString("RemoteClipboard: Failed to open file: %1").arg(filePath), LogWidget::Error);
						}
					}
					// 否则处理文本数据
					else if (mimeData->hasText()) {
						TextContent* textContent = eventMsg.mutable_text();
						textContent->set_text_data(mimeData->text().toStdString());
						LogWidget::instance()->addLog("RemoteClipboard: Captured text data", LogWidget::Info);
					}
					else {
						LogWidget::instance()->addLog("RemoteClipboard: Unsupported clipboard data", LogWidget::Warning);
					}
				}
				else {
					LogWidget::instance()->addLog("RemoteClipboard: Clipboard is empty", LogWidget::Warning);
				}
				// 使用 queued 方式发射带参数的信号，确保在 Qt 主线程中处理
				QMetaObject::invokeMethod(this, "ctrlCPressed", Qt::QueuedConnection,
					Q_ARG(ClipboardEvent, eventMsg));
				LogWidget::instance()->addLog("RemoteClipboard: Global Ctrl+C detected and clipboard data captured", LogWidget::Info);
				// 可选择拦截此事件，或返回 CallNextHookEx 继续传递
			}
		}
	}
	return CallNextHookEx(s_hook, nCode, wParam, lParam);
}
