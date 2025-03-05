#include "MessageProcessor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>

const int REG_TIMEOUT_MS = 30000;      // 注册超时 30 秒
const int REG_PK_INTERVAL_MS = 6000;     // 6 秒内累计限制
const QString SERVER_LICENSE_KEY = "my_license_key";

MessageProcessor::MessageProcessor(QObject* parent) : QObject(parent)
{
}

QString MessageProcessor::encodeAddress(const QHostAddress& addr, quint16 port) {
	return addr.toString() + ":" + QString::number(port);
}

void MessageProcessor::sendRegisterPkResponse(const QString& result, QTcpSocket* tcpSocket, bool isUDP,
	const QHostAddress& sender, quint16 senderPort) {
	QJsonObject response;
	response.insert("type", "RegisterPkResponse");
	response.insert("result", result);
	QJsonDocument doc(response);
	QByteArray data = doc.toJson();
	emit sendResponse(data, tcpSocket, isUDP, sender, senderPort);
}

void MessageProcessor::processMessage(const QByteArray& data, QTcpSocket* tcpSocket, bool isUDP,
	const QHostAddress& sender, quint16 senderPort) {
	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError) {
		qWarning() << "JSON parse error:" << parseError.errorString();
		return;
	}
	if (!doc.isObject()) {
		qWarning() << "JSON is not an object";
		return;
	}
	QJsonObject obj = doc.object();
	QString type = obj.value("type").toString();
	if (type == "RegisterPeer") {
		processRegisterPeer(obj, tcpSocket, isUDP, sender, senderPort);
	}
	else if (type == "RegisterPk") {
		processRegisterPk(obj, tcpSocket, isUDP, sender, senderPort);
	}
	else if (type == "PunchHoleRequest") {
		processPunchHoleRequest(obj, tcpSocket, isUDP, sender, senderPort);
	}
	else if (type == "RelayResponse") {
		processRelayResponse(obj, tcpSocket, isUDP, sender, senderPort);
	}
	else {
		qWarning() << "Unknown message type:" << type;
	}
}

void MessageProcessor::processRegisterPeer(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
	const QHostAddress& sender, quint16 senderPort) {
	QString id = obj.value("id").toString();
	if (id.isEmpty()) {
		qWarning() << "RegisterPeer id is empty";
		return;
	}
	Peer* peer = peerManager.getOrCreate(id);
	QString senderAddr = encodeAddress(sender, senderPort);
	peer->lastRegTime = QDateTime::currentDateTime();
	peer->socketAddr = senderAddr;
	peer->info.insert("ip", sender.toString());
	bool request_pk = peer->pk.isEmpty();
	QJsonObject response;
	response.insert("type", "RegisterPeerResponse");
	response.insert("request_pk", request_pk);
	QJsonDocument doc(response);
	emit sendResponse(doc.toJson(), tcpSocket, isUDP, sender, senderPort);
	qDebug() << "Processed RegisterPeer, id:" << id << "sender:" << senderAddr;
}

void MessageProcessor::processRegisterPk(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
	const QHostAddress& sender, quint16 senderPort) {
	QString id = obj.value("id").toString();
	QString uuid = obj.value("uuid").toString();
	QString pk = obj.value("pk").toString();
	if (uuid.isEmpty() || pk.isEmpty()) {
		qWarning() << "RegisterPk: uuid or pk is empty";
		return;
	}
	if (id.length() < 6) {
		sendRegisterPkResponse("UUID_MISMATCH", tcpSocket, isUDP, sender, senderPort);
		return;
	}
	Peer* peer = peerManager.getOrCreate(id);
	QString senderIp = sender.toString();
	bool changed = false;
	bool ipChanged = false;
	if (peer->uuid.isEmpty()) {
		changed = true;
	}
	else {
		if (peer->uuid == uuid) {
			if (peer->info.value("ip").toString() != senderIp && peer->pk != pk) {
				qWarning() << "Peer" << id << "ip or pk mismatch";
				sendRegisterPkResponse("UUID_MISMATCH", tcpSocket, isUDP, sender, senderPort);
				return;
			}
		}
		else {
			qWarning() << "Peer" << id << "uuid mismatch";
			sendRegisterPkResponse("UUID_MISMATCH", tcpSocket, isUDP, sender, senderPort);
			return;
		}
		ipChanged = (peer->info.value("ip").toString() != senderIp);
		changed = (peer->uuid != uuid || peer->pk != pk || ipChanged);
	}
	qint64 elapsed = peer->regPkTime.msecsTo(QDateTime::currentDateTime());
	if (elapsed > REG_PK_INTERVAL_MS) {
		peer->regPkCount = 0;
	}
	if (peer->regPkCount > 2) {
		sendRegisterPkResponse("TOO_FREQUENT", tcpSocket, isUDP, sender, senderPort);
		return;
	}
	peer->regPkCount++;
	peer->regPkTime = QDateTime::currentDateTime();
	if (ipChanged) {
		peer->info.insert("ip", senderIp);
	}
	if (changed) {
		peer->uuid = uuid;
		peer->pk = pk;
	}
	sendRegisterPkResponse("OK", tcpSocket, isUDP, sender, senderPort);
	qDebug() << "Processed RegisterPk, id:" << id << "result: OK";
}

void MessageProcessor::processPunchHoleRequest(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
	const QHostAddress& sender, quint16 senderPort) {
	QString licenceKey = obj.value("licence_key").toString();
	if (!SERVER_LICENSE_KEY.isEmpty() && licenceKey != SERVER_LICENSE_KEY) {
		QJsonObject response;
		response.insert("type", "PunchHoleResponse");
		response.insert("failure", "LICENSE_MISMATCH");
		QJsonDocument doc(response);
		emit sendResponse(doc.toJson(), tcpSocket, isUDP, sender, senderPort);
		return;
	}
	QString id = obj.value("id").toString();
	Peer* peer = peerManager.get(id);
	if (peer) {
		qint64 elapsed = peer->lastRegTime.msecsTo(QDateTime::currentDateTime());
		if (elapsed >= REG_TIMEOUT_MS) {
			QJsonObject response;
			response.insert("type", "PunchHoleResponse");
			response.insert("failure", "OFFLINE");
			QJsonDocument doc(response);
			emit sendResponse(doc.toJson(), tcpSocket, isUDP, sender, senderPort);
			return;
		}
		QJsonObject response;
		response.insert("type", "PunchHole");
		QString encodedAddr = encodeAddress(sender, senderPort);
		response.insert("socket_addr", encodedAddr);
		response.insert("nat_type", "SYMMETRIC");
		QJsonDocument doc(response);
		QByteArray msgData = doc.toJson();
		if (peer->tcpSocket && peer->tcpSocket->state() == QAbstractSocket::ConnectedState) {
			peer->tcpSocket->write(msgData);
			qDebug() << "Forwarded PunchHole to peer:" << peer->socketAddr;
		}
		else {
			emit sendResponse(msgData, tcpSocket, isUDP, sender, senderPort);
		}
		qDebug() << "Processed PunchHoleRequest, id:" << id;
	}
	else {
		QJsonObject response;
		response.insert("type", "PunchHoleResponse");
		response.insert("failure", "ID_NOT_EXIST");
		QJsonDocument doc(response);
		emit sendResponse(doc.toJson(), tcpSocket, isUDP, sender, senderPort);
	}
}

void MessageProcessor::processRelayResponse(const QJsonObject& obj, QTcpSocket* tcpSocket, bool isUDP,
	const QHostAddress& sender, quint16 senderPort) {
	// RelayResponse 的处理在这里仅简单发出回复，实际转发逻辑由 RendezvousServer 实现
	QJsonDocument doc(obj);
	emit sendResponse(doc.toJson(), tcpSocket, isUDP, sender, senderPort);
}
