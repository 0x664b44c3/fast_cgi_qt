#ifndef FASTCGI_LISTENER_H
#define FASTCGI_LISTENER_H

#include <QObject>
QT_BEGIN_NAMESPACE
class QTcpServer;
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
signals:
	void newConnection(Connection*);
private:
	QTcpServer *mTcpServer;
	QLocalServer *mLocalServer;
private slots:
	void onNewTcpConnection();
	void onNewLocalConnection();
};
}

#endif // FASTCGI_LISTENER_H
