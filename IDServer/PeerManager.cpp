#include "PeerManager.h"
#include <QtDebug>

PeerManager::PeerManager()
{
}

PeerManager::~PeerManager() {
	qDeleteAll(peers);
	peers.clear();
}

Peer* PeerManager::getOrCreate(const QString& id) {
	if (peers.contains(id))
		return peers.value(id);
	Peer* peer = new Peer();
	peer->id = id;
	peers.insert(id, peer);
	return peer;
}

Peer* PeerManager::get(const QString& id) {
	return peers.contains(id) ? peers.value(id) : nullptr;
}