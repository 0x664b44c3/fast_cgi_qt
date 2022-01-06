#ifndef FASTCGI_CONNECTION_H
#define FASTCGI_CONNECTION_H

#include "fastcgi_types.h"
#include <QObject>
#include <QByteArray>
#include <QMap>
//#include <QSharedPointer>
QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace FastCGI
{

struct RecordHeader;
class Request;

class Connection : public QObject
{
	Q_OBJECT
	friend class Request;
public:
	explicit Connection(QIODevice * dev, QObject *parent = nullptr);

signals:
	void connectionClosed();
	void newRequest(Request *);
	void requestStdinDone(Request *);
	void requestFinished(quint16 requestId);

private:
	QIODevice * mIoDev;
	QByteArray mBuffer;
	void consumeBuffer();

	QMap<quint16, Request*> mRequests;
private slots:
	void onReadyRead();
	void onRequestParamsDone(quint16);
	void onRequestStdinDone(quint16);
	void onRequestFinished(quint16);

protected:
	void sendRecord(quint16 requestId, unsigned char type, const QByteArray &payload);
	void write(const QByteArray &);
	void closeConnection();

};

} //end namespace
#endif // FASTCGI_CONNECTION_H
