#include "fastcgi.h"
#include "fastcgi_connection.h"
#include "fastcgi_listener.h"
#include "fastcgi_request.h"
#include "fastcgi_request_wrapper.h"

#include <QDebug>
namespace FastCGI
{

FastCGI::FastCGI(QObject *parent) :
    QObject(parent), mListener(new Listener(this))
{
	connect(mListener, &Listener::newConnection,
	        this, &FastCGI::onNewConnection);
}

bool FastCGI::listenTcp(const QHostAddress &a, quint16 port)
{
	return mListener->listenTcp(a, port);
}

bool FastCGI::listenLocal(QString path)
{
	return mListener->listenLocalSocket(path);
}

bool FastCGI::isListening() const
{
    return mListener->isListening();
}

void FastCGI::onNewConnection(Connection * conn)
{
	connect(conn, &Connection::connectionClosed,
	        this, &FastCGI::onConnectionClosed);
	connect(conn, &Connection::newRequest,
	        this, &FastCGI::onNewRequest);
	connect(conn, &Connection::requestStdinDone,
	        this, &FastCGI::onRequestStdinClosed);
	connect(conn, &Connection::requestFinished,
	        this, &FastCGI::onRequestFinished);
}

void FastCGI::onConnectionClosed()
{
	QObject * sender = QObject::sender();
	if(sender)
		sender->deleteLater();
}

void FastCGI::onNewRequest(Request *req)
{
	emit newRequest(req);
}

void FastCGI::onRequestStdinClosed(Request *req)
{
	emit requestStdinClosed(req);
}

void FastCGI::onRequestFinished(quint16 requestId)
{
	emit requestFinished(requestId);
}

}
