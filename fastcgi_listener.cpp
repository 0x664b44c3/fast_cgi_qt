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
namespace FastCGI
{

Listener::Listener(QObject *parent) : QObject(parent),
    mTcpServer(nullptr),
    mLocalServer(nullptr)
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

void Listener::onNewTcpConnection()
{
	while(mTcpServer->hasPendingConnections())
	{
//		qDebug()<<"new connection";
		QTcpSocket * socket = mTcpServer->nextPendingConnection();
		if (!socket)
			break;
		auto conn = new Connection(socket, this);
		emit newConnection(conn);
	}
}

void Listener::onNewLocalConnection()
{
	while(mLocalServer->hasPendingConnections())
	{
//		qDebug()<<"new connection";
		QLocalSocket * socket = mLocalServer->nextPendingConnection();
		if (!socket)
			break;
		auto conn = new Connection(socket, this);
		emit newConnection(conn);
	}
}

}
