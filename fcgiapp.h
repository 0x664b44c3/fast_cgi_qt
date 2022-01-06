#ifndef FCGIAPP_H
#define FCGIAPP_H

#include <QObject>
#include <QMap>

#include "fastcgi/fastcgi.h"
class fcgiApp;
namespace FastCGI {
class RequestWrapper;
}
class cgiHandler
{
public:
	explicit cgiHandler(){};
	~cgiHandler(){};
	virtual void registerEndpoints(fcgiApp *, QString =""){};
	virtual int handleRequest(QString url, QString method, FastCGI::RequestWrapper *) = 0;
};

class fcgiApp : public QObject
{
	Q_OBJECT
public:
	explicit fcgiApp(FastCGI::FastCGI * fcgi, QObject *parent = nullptr);
	void registerHandler(QString url, cgiHandler * handler);
	void deleteHandler(QString url);


	bool debug() const;
	void setDebug(bool newDebug);

signals:
	void failedToStart(int errorCode, QString message);
public slots:
	void startCgi();
private slots:
	void onNewRequest(FastCGI::Request *request);

private:
	FastCGI::FastCGI *mFCgi;
	QMap<QString, cgiHandler *> mHandlers;
	bool mDebugMode;

};

#endif // FCGIAPP_H
