#include "fastcgi_listener.h"

#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

#ifdef Q_OS_UNIX

#include <QLocalServer>
#include <QLocalSocket>
#endif

#include "fastcgi_connection.h"
#include <QDebug>
#include <QTimer>

namespace FastCGI
{

Listener::Listener(QObject *parent) : QObject(parent),
    mTcpServer(nullptr),
    mLocalServer(nullptr),
    mMaxConnections(-1),
    mConnectionCount(0)
{
}

bool Listener::listenTcp(const QHostAddress &bindAddr, quint16 port)
{
#ifdef Q_OS_UNIX
	if (mLocalServer)
	{
		mLocalServer->close();
		mLocalServer->deleteLater();
		mLocalServer = nullptr;
	}
#endif
	if (mTcpServer)
		mTcpServer->close();
	else
		mTcpServer = new QTcpServer(this);

	mTcpServer->listen(bindAddr, port);
	connect(mTcpServer, &QTcpServer::newConnection, this, &Listener::onNewTcpConnection);
	return mTcpServer->isListening();
}

bool Listener::listenLocalSocket(QString path)
{
	if (mTcpServer)
	{
		mTcpServer->close();
		mTcpServer->deleteLater();
		mTcpServer = nullptr;
	}

#ifdef Q_OS_UNIX
	if (mLocalServer)
		mLocalServer->close();
	else
		mLocalServer = new QLocalServer(this);
	mLocalServer->listen(path);
	connect(mLocalServer, &QLocalServer::newConnection, this, &Listener::onNewLocalConnection);
	return mLocalServer->isListening();
#else
	return false;
#endif
}

bool Listener::isListening() const
{
	if (mLocalServer)
		return mLocalServer->isListening();
	if (mTcpServer)
		return mTcpServer->isListening();
	return false;
}

int Listener::maxConnections() const
{
	return mMaxConnections;
}

void Listener::setMaxConnections(int newMaxConnections)
{
	mMaxConnections = newMaxConnections;
}

//you can use the socket object to e.g. check the remote host's address
//if a nullptr is passed, ignore the socket (only eval connection count).
bool Listener::connectionPermitted(QTcpSocket*) const
{
	if ((mMaxConnections>0) && (mConnectionCount>=mMaxConnections))
		return false;
	return true;
}

void Listener::onNewTcpConnection()
{
	while(mTcpServer->hasPendingConnections())
	{
		QTcpSocket * socket = mTcpServer->nextPendingConnection();
		if (!socket)
			break;
		auto conn = new Connection(socket, this);
		++mConnectionCount;
		connect(conn, &Connection::connectionClosed,
		        this, &Listener::onConnectionClosed);
		emit newConnection(conn);

		if ((mMaxConnections>0) && (mConnectionCount>=mMaxConnections))
			mTcpServer->pauseAccepting();
	}
}

void Listener::onNewLocalConnection()
{
	while(mLocalServer->hasPendingConnections())
	{
		QLocalSocket * socket = mLocalServer->nextPendingConnection();

		if (!socket)
			break;

		auto conn = new Connection(socket, this);
		++mConnectionCount;
		connect(conn, &Connection::connectionClosed,
		        this, &Listener::onConnectionClosed);
		emit newConnection(conn);
	}
}

void Listener::onConnectionClosed()
{
	if (mConnectionCount>0)
		--mConnectionCount;
	if (mConnectionCount < mMaxConnections)
	{
		if (mTcpServer)
			mTcpServer->resumeAccepting();
	}
}

void Listener::reCheckPendingConns()
{

}

}
