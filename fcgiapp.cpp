#include "fcgiapp.h"
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include "fastcgi_request.h"
#include "fastcgi_request_wrapper.h"
#include "fcgiapp.h"

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

void fcgiApp::onNewRequest(FastCGI::Request *request)
{

    FastCGI::RequestWrapper req(request);
    QString scriptUrl = req.scriptUri();
    bool found = false;

    Http::Method method = Http::Unknown;

    QString methodString = req.method().toUpper();

    if (methodString == "GET")
        method = Http::Get;
    else if (methodString == "PUT")
        method = Http::Put;
    else if (methodString == "DELETE")
        method = Http::Delete;
    else if (methodString == "POST")
        method = Http::Post;
    else if (methodString == "HEAD")
        method = Http::Head;
    else if (methodString == "OPTIONS")
        method = Http::Options;
    else if (methodString == "PATCH")
        method = Http::Patch;
    else if (methodString == "CONNECT")
        method = Http::Connect;
    else if (methodString == "TRACE")
        method = Http::Trace;
    else
        method = Http::Unknown;

    req.setBufferResponse(true);

    int ret=404;
    auto handler = mHandlers.value(scriptUrl, nullptr);
    if (handler)
    {
        found = true;
        ret = handler->handleRequest(scriptUrl, method, &req);
    }
    else
    {
        qDebug()<<"script uri not handled:"<<scriptUrl;
    }

    if (request->isFinished())
        return;

    if (found)
    {
        if (!req.headers().contains("Status"))
            req.setHeader("Status", ret);
    } else {
        qDebug()<<"unhandled endpoint"<<scriptUrl;
        QMap<QString, QString> vars;
        req.setHeader("Status", "404");
        req.writeResponse(errorPage(404, "Not found", scriptUrl.toHtmlEscaped()));
        req.flush();
        request->finishRequest(404);
    }

    if (!req.headersSent())
        req.sendHeaders();
    req.flush();
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

QByteArray fcgiApp::errorPage(int code, QString error, QString message)
{
    QMap<QString, QString> vars;
    vars.insert("code", QString::number(code));
    vars.insert("title", QString("%1 - %2").arg(code).arg(error));
    vars.insert("error", error);
    vars.insert("message", message);
    return getPage("error_page.html", vars);
}

QByteArray fcgiApp::getPage(QString page, QMap<QString, QString> variables)
{
    page = page.replace("..", "");
    QFile file(":/" + page);
    if (!file.open(QFile::ReadOnly))
    {
        return QByteArray();
    }
    QString pageData = QString::fromUtf8(file.readAll());
    for(auto it = variables.begin(); it!=variables.end(); ++it)
    {
        pageData = pageData.replace("%"+it.key()+"%", it.value());
    }
    return pageData.toUtf8();
}
