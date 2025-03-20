#include "RemoteClipboard.h"
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QFile>
#include <QFileInfo>
#include "LogWidget.h"

RemoteClipboard::RemoteClipboard(QObject* parent)
	: QObject(parent)
{
	qApp->installEventFilter(this);
}


bool RemoteClipboard::eventFilter(QObject* /*obj*/, QEvent* event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		if ((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() == Qt::Key_C) {
			QClipboard* clipboard = QApplication::clipboard();
			const QMimeData* mimeData = clipboard->mimeData();
			if (mimeData) {
				LogWidget::instance()->addLog("Control side: Detected Ctrl+C, sending clipboard data", LogWidget::Info);
				sendClipboardData(mimeData);
			}
			else {
				LogWidget::instance()->addLog("Control side: Clipboard is empty", LogWidget::Warning);
			}
			return true;
		}
	}
	return QObject::eventFilter(nullptr, event);
}

void RemoteClipboard::sendClipboardData(const QMimeData* mimeData)
{
	ClipboardEvent eventMsg;
	// 如果剪贴板中包含文件 URL，优先处理文件数据
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
			LogWidget::instance()->addLog(QString("Control side: Copied file: %1").arg(filePath), LogWidget::Info);
		}
		else {
			LogWidget::instance()->addLog(QString("Control side: Failed to open file: %1").arg(filePath), LogWidget::Error);
			return;
		}
	}
	else if (mimeData->hasText()) {
		TextContent* textContent = eventMsg.mutable_text();
		textContent->set_text_data(mimeData->text().toStdString());
		LogWidget::instance()->addLog("Control side: Copied text data", LogWidget::Info);
	}
	else {
		LogWidget::instance()->addLog("Control side: Unsupported clipboard data", LogWidget::Warning);
		return;
	}
	emit clipboardDataReady(eventMsg);
}
