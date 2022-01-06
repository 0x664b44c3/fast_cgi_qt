#ifndef FASTCGI_LISTENER_H
#define FASTCGI_LISTENER_H

#include <QObject>
QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
class QLocalServer;
class QHostAddress;
QT_END_NAMESPACE

namespace FastCGI
{

class Connection;

class Listener : public QObject
{
	Q_OBJECT
public:
	explicit Listener(QObject *parent = nullptr);
	bool listenTcp(const QHostAddress & bindAddr, quint16 port);
	bool listenLocalSocket(QString path);
	bool isListening() const;
	int maxConnections() const;
	void setMaxConnections(int newMaxConnections);

signals:
	void newConnection(Connection*);
private:
	QTcpServer *mTcpServer;
	QLocalServer *mLocalServer;
	int mMaxConnections;
	int mConnectionCount;
	int mRecheckCycles;
	bool connectionPermitted(QTcpSocket *) const;
private slots:
	void onNewTcpConnection();
	void onNewLocalConnection();
	void onConnectionClosed();
	void reCheckPendingConns();
};
}

#endif // FASTCGI_LISTENER_H
