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
	quint64 writeResponse(const QByteArray &);
	qint64 writeResponse(const char * d, qint64 len);
	QString scriptUri() const;
	QString method() const;
	void sendHeaders();
	QMap<QString, QByteArray> parseFormData();
private:
	Request * mRequest;
	bool mHeadersSent;
	QString mScriptUri, mRequestMethod;
	QMap<QString, QString> mHeaders;
};
}
#endif // CGIREQUEST_H
