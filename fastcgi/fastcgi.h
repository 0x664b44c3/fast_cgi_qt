#ifndef FASTCGI_H
#define FASTCGI_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QHostAddress;
QT_END_NAMESPACE

namespace FastCGI
{

class Request;
class Request;
class Connection;
class Listener;

class FastCGI : public QObject
{
	Q_OBJECT
public:
	explicit FastCGI(QObject *parent = nullptr);

	bool listenTcp(const QHostAddress &a, quint16 port);
	bool listenLocal(QString path);
	bool isListening() const;
signals:
	void newRequest(Request * req);
	void requestStdinClosed(Request * req);
	void requestFinished(quint16 requestId);
private:
	Listener * mListener;
private slots:
	void onNewConnection(Connection*);
	void onConnectionClosed();

	void onNewRequest(Request * req);
	void onRequestStdinClosed(Request * req);
	void onRequestFinished(quint16 requestId);

};
}
#endif // FASTCGI_H
