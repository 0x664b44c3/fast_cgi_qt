#include "fcgiapp.h"
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include "fastcgi_request.h"
#include "fastcgi_request_wrapper.h"

fcgiApp::fcgiApp(FastCGI::FastCGI *fcgi, QObject *parent) :
    QObject(parent), mFCgi(fcgi), mDebugMode(false)
{
	connect(mFCgi, &FastCGI::FastCGI::requestStdinClosed, this, &fcgiApp::onNewRequest);


}

void fcgiApp::registerHandler(QString url, cgiHandler *handler)
{
	mHandlers.insert(url, handler);
}

void fcgiApp::deleteHandler(QString url)
{
	mHandlers.remove(url);
}

void fcgiApp::startCgi()
{
//	mFCgi->start();
//	if (!mFCgi->isStarted())
//	{
//		emit failedToStart(1, mFCgi->errorString());
//		qCritical()<< "CGI process would not start: " << mFCgi->errorString();
//		QCoreApplication::instance()->exit(1);
//	}
}

void fcgiApp::onNewRequest(FastCGI::Request *request)
{
	qDebug()<<"request stdin closed.";
	FastCGI::RequestWrapper req(request);
	QString scriptUrl = req.scriptUri();
	bool found = false;

	int ret=404;
	request->writeStdOut("X-Status: OK\r\n");
	auto handler = mHandlers.value(scriptUrl, nullptr);
	if (handler)
	{
		found = true;
		ret = handler->handleRequest(scriptUrl, req.method().toUpper(), &req);
	}
	else
	{
		qDebug()<<"script uri not handled:"<<scriptUrl;
	}

	if (request->isFinished())
		return;

	if (!found)
	{
		req.setHeader("Status", "404");
		req.writeResponse("404\r\nNot found.");
		request->finishRequest(0);
	}

	//safe to call twice, subsequent calls are ignored once the request has been marked as finished
	request->finishRequest(ret);
}

bool fcgiApp::debug() const
{
	return mDebugMode;
}

void fcgiApp::setDebug(bool newDebug)
{
	mDebugMode = newDebug;
}

