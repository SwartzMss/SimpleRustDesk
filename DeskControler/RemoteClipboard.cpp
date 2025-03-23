#include "RemoteClipboard.h"
#include "LogWidget.h"
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QFile>
#include <QDir>
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


void RemoteClipboard::onClipboardMessageReceived(const ClipboardEvent& clipboardEvent)
{
	LogWidget::instance()->addLog("onClipboardMessageReceived", LogWidget::Error);
	switch (clipboardEvent.event_case()) {
	case ClipboardEvent::kText: {
		// �����ı�����
		QString text = QString::fromStdString(clipboardEvent.text().text_data());
		QApplication::clipboard()->setText(text);
		LogWidget::instance()->addLog("RemoteClipboard: Updated clipboard with text data", LogWidget::Info);
		break;
	}
	case ClipboardEvent::kFile: {
		// �����ļ�����
		const FileContent& fileContent = clipboardEvent.file();
		QString fileName = QString::fromStdString(fileContent.file_name());
		// �����ļ�����ʱĿ¼
		QString tempPath = QDir::tempPath() + "/" + fileName;
		QFile file(tempPath);
		if (file.open(QIODevice::WriteOnly)) {
			file.write(QByteArray::fromStdString(fileContent.file_data()));
			file.close();
			LogWidget::instance()->addLog(QString("RemoteClipboard: File saved to %1").arg(tempPath), LogWidget::Info);
			// ���¼�����Ϊ�ļ� URL��ʹ��ճ���������Ի�ȡ���ļ�
			QMimeData* mimeData = new QMimeData;
			QList<QUrl> urls;
			urls.append(QUrl::fromLocalFile(tempPath));
			mimeData->setUrls(urls);
			QApplication::clipboard()->setMimeData(mimeData);
		}
		else {
			LogWidget::instance()->addLog("RemoteClipboard: Failed to save file", LogWidget::Error);
		}
		break;
	}
	default:
		LogWidget::instance()->addLog("RemoteClipboard: Received unknown clipboard event", LogWidget::Warning);
		break;
	}
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
		// ���������¼�
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			if (ctrlPressed && pKeyboard->vkCode == 'C') {
				QWidget* activeWnd = QApplication::activeWindow();
				if (m_remoteWindow && activeWnd == m_remoteWindow) {
					// ����������Զ�����棬�򲻴��� Ctrl+C������Զ�����洦��
					LogWidget::instance()->addLog("Remote window active, ignoring Ctrl+C on control side", LogWidget::Info);
					return CallNextHookEx(s_hook, nCode, wParam, lParam);
				}
				// ��ȡ���������ݣ������� ClipboardEvent
				ClipboardEvent eventMsg;
				QClipboard* clipboard = QApplication::clipboard();
				const QMimeData* mimeData = clipboard->mimeData();
				if (mimeData) {
					// �������������ļ� URL�������ȴ����ļ�����
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
					// �������ı�����
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
				// ʹ�� queued ��ʽ������������źţ�ȷ���� Qt ���߳��д���
				QMetaObject::invokeMethod(this, "ctrlCPressed", Qt::QueuedConnection,
					Q_ARG(ClipboardEvent, eventMsg));
				LogWidget::instance()->addLog("RemoteClipboard: Global Ctrl+C detected and clipboard data captured", LogWidget::Info);
				// ��ѡ�����ش��¼����򷵻� CallNextHookEx ��������
			}
		}
	}
	return CallNextHookEx(s_hook, nCode, wParam, lParam);
}