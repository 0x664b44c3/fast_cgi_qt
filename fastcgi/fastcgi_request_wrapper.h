#ifndef CGIREQUEST_WRAPPER_H
#define CGIREQUEST_WRAPPER_H
#include <QByteArray>
#include <QString>
#include <QMap>

namespace FastCGI
{
class Request;

class RequestWrapper
{
public:
	RequestWrapper(Request* req);
	QByteArray parameter(QString key);
	QMap<QString, QByteArray> parameters() const;
	void setHeader(QString name, QString data);
	QMap<QString, QString> & headers();
    QString header(QString key) const;
	quint64 writeResponse(const QByteArray &);
	qint64 writeResponse(const char * d, qint64 len);
	QString scriptUri() const;
    QString method() const;
	void sendHeaders();
    void flush();
    bool headersSent() const;
    QMap<QString, QByteArray> parseFormData();

    static QMap<QString, QByteArray> cookies(Request *);
    QMap<QString, QByteArray> cookies();
    Request *request() const;

    bool bufferResponse() const;
    void setBufferResponse(bool newBufferResponse);

    void setHeader(QString name, int n);
    void setHeader(QString name, const char *d);
    void setHeader(QString name, const QByteArray d);
private:
	Request * mRequest;
	bool mHeadersSent;
	QString mScriptUri, mRequestMethod;
	QMap<QString, QString> mHeaders;
    bool mBufferResponse;

    QByteArray mStdOutBuffer;
};
}
#endif // CGIREQUEST_H
