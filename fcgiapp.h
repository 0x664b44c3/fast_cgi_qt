#ifndef FCGIAPP_H
#define FCGIAPP_H

#include <QObject>
#include <QMap>

#include "fastcgi/fastcgi.h"
class fcgiApp;
namespace FastCGI {
class RequestWrapper;
class Request;
}

namespace Http {
enum Method
{
    Unknown = 0,
    Get      = 0x0001,
    Put      = 0x0002,
    Delete   = 0x0004,
    Post     = 0x0008,
    Head     = 0x0010,
    Options  = 0x0020,
    Patch    = 0x0040,
    Connect  = 0x0080,
    Trace    = 0x0100,
    AnyKnown = 	0x01ff,
};}

class cgiHandler
{
public:
	explicit cgiHandler(){};
	~cgiHandler(){};
	virtual void registerEndpoints(fcgiApp *, QString =""){};
    virtual int handleRequest(QString url, Http::Method method, FastCGI::RequestWrapper *) = 0;
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

	QByteArray errorPage(int code, QString error, QString message);
	QByteArray getPage(QString page, QMap<QString, QString> variables);

signals:
	void failedToStart(int errorCode, QString message);
public slots:

private slots:
	void onNewRequest(FastCGI::Request *request);

private:
	FastCGI::FastCGI *mFCgi;
	QMap<QString, cgiHandler *> mHandlers;
	bool mDebugMode;

};

#endif // FCGIAPP_H
