#include "RemoteClipboard.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include "LogWidget.h"

RemoteClipboard::RemoteClipboard(QObject* parent)
	: QObject(parent)
{
}

void RemoteClipboard::onClipboardMessageReceived(const ClipboardEvent& clipboardEvent)
{
	LogWidget::instance()->addLog("onClipboardMessageReceived", LogWidget::Error);
	switch (clipboardEvent.event_case()) {
	case ClipboardEvent::kText: {
		// 处理文本数据
		QString text = QString::fromStdString(clipboardEvent.text().text_data());
		QApplication::clipboard()->setText(text);
		LogWidget::instance()->addLog("RemoteClipboard: Updated clipboard with text data", LogWidget::Info);
		break;
	}
	case ClipboardEvent::kFile: {
		// 处理文件数据
		const FileContent& fileContent = clipboardEvent.file();
		QString fileName = QString::fromStdString(fileContent.file_name());
		// 保存文件到临时目录
		QString tempPath = QDir::tempPath() + "/" + fileName;
		QFile file(tempPath);
		if (file.open(QIODevice::WriteOnly)) {
			file.write(QByteArray::fromStdString(fileContent.file_data()));
			file.close();
			LogWidget::instance()->addLog(QString("RemoteClipboard: File saved to %1").arg(tempPath), LogWidget::Info);
			// 更新剪贴板为文件 URL，使得粘贴操作可以获取该文件
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
