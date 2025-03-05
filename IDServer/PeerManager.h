#ifndef PEERMANAGER_H
#define PEERMANAGER_H

#include "Peer.h"
#include <QHash>
#include <QString>

class PeerManager {
public:
	PeerManager();
	~PeerManager();
	Peer* getOrCreate(const QString& id);
	Peer* get(const QString& id);
private:
	QHash<QString, Peer*> peers;
};

#endif // PEERMANAGER_H
