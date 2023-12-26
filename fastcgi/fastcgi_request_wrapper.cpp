
#include "fastcgi_request_wrapper.h"
#include "fastcgi_request.h"
#include <iostream>
#include <ctype.h>
#include <QDebug>
QMap<QString, QString> parseHeaderFields(const QByteArray hdr)
{
	QMap<QString, QString> data;
	bool inQuote=false;
	QString name;
	QString value;
	int idx=0;
	int start=0;
	while(idx<hdr.length())
	{
		while((idx<hdr.length()) && (isspace(hdr[idx]) || (hdr[idx] == ';')))
			++idx;
		start=idx;
		idx = hdr.indexOf('=', idx);
		if (idx<0)
		{
            qDebug()<<"no = found";
			break;
		}
		name = QString::fromLatin1(hdr.mid(start, idx - start));
		//scan for ; or end of line
		++idx;
		start=idx;
		while(idx<hdr.length())
		{
			if (hdr[idx] == '"')
				inQuote = !inQuote;
			if ((inQuote==false) && (hdr[idx] == ';'))
				break;
			++idx;
		}
		QString value = QString::fromLatin1(hdr.mid(start, idx - start));
		if (value.startsWith('"'))
		{
			value = value.mid(1).chopped(1);
		}
		data.insert(name, value);
	}
	return data;
}

namespace FastCGI
{
RequestWrapper::RequestWrapper(Request *req) : mRequest(req), mHeadersSent(false), mBufferResponse(false)
{
	mScriptUri = QString::fromLocal8Bit(req->parameter("SCRIPT_NAME"));
	mRequestMethod = QString::fromLocal8Bit(req->parameter("REQUEST_METHOD"));

}

QByteArray RequestWrapper::parameter(QString key)
{
	return mRequest->parameter(key);
}

QMap<QString, QByteArray> RequestWrapper::parameters() const
{
	return mRequest->parameters();
}

void RequestWrapper::setHeader(QString name, const char * d)
{
    setHeader(name, QString::fromLocal8Bit(d));
}

void RequestWrapper::setHeader(QString name, const QByteArray d)
{
    setHeader(name, QString::fromLocal8Bit(d));
}

void RequestWrapper::setHeader(QString name, QString data)
{
    if (mHeadersSent)
    {
        qWarning()<<"Warning: headers already sent!";
    }
	mHeaders.insert(name, data);
}

void RequestWrapper::setHeader(QString name, int n)
{
    if (mHeadersSent)
        qWarning()<<"Warning: headers already sent!";
    mHeaders.insert(name, QString("%1").arg(n));
}


QMap<QString, QString> &RequestWrapper::headers()
{
    return mHeaders;
}

QString RequestWrapper::header(QString key) const
{
    return mHeaders.value(key);
}

quint64 RequestWrapper::writeResponse(const QByteArray & d)
{
    if (mBufferResponse)
    {
        mStdOutBuffer.append(d);
        return d.size();
    }
    else
    {
        if (!mHeadersSent)
        {
            sendHeaders();
        }
        return mRequest->writeStdOut(d);
    }
}

qint64 RequestWrapper::writeResponse(const char *d, qint64 len)
{
    if (mBufferResponse)
    {
        mStdOutBuffer.append(d,len);
        return len;
    }
    else
    {
        if (!mHeadersSent && mHeaders.size())
        {
            sendHeaders();
        }
        return mRequest->writeStdOut(d, len);
    }
}

QString RequestWrapper::scriptUri() const
{
	return mScriptUri;
}

QString RequestWrapper::method() const
{
	return mRequestMethod;
}

void RequestWrapper::sendHeaders()
{
	for(auto it=mHeaders.begin(); it!=mHeaders.end(); ++it)
	{
		QString headerLine("%1: %2\r\n");
		mRequest->writeStdOut(headerLine
		           .arg(it.key())
		           .arg(it.value())
		           .toLocal8Bit());
	}
	mRequest->writeStdOut("\r\n");
    mHeadersSent = true;
}

void RequestWrapper::flush()
{
    if (!headersSent())
        sendHeaders();
    mRequest->writeStdOut(mStdOutBuffer);
    mStdOutBuffer.clear();
    mBufferResponse = false;
}

bool RequestWrapper::headersSent() const
{
    return mHeadersSent;
}

QMap<QString, QByteArray> RequestWrapper::parseFormData()
{
	QString contentType = QString::fromLocal8Bit(mRequest->parameter("HTTP_CONTENT_TYPE"));
	QByteArray in_data =  mRequest->getStdInBuffer();
	QMap<QString, QByteArray> formData;
	QString ct_lower = contentType.toLower();
	if (ct_lower == "application/x-www-form-urlencoded")
	{
		QList<QByteArray> formDataFields = in_data.split('&');
		for(QByteArray entry: formDataFields)
		{
			int vs = entry.indexOf('=');

			QByteArray fieldName, fieldValue;
			if (vs<0)
				vs=entry.size();
			fieldName = QByteArray::fromPercentEncoding(entry.left(vs));
			fieldValue = QByteArray::fromPercentEncoding(entry.mid(vs+1));
			formData.insert(QString::fromLocal8Bit(fieldName), fieldValue);
		}
		return formData;
	}
	if (ct_lower.startsWith("multipart/form-data"))
	{
		QByteArray ct_raw = mRequest->parameter("HTTP_CONTENT_TYPE");
		QByteArray boundary("---");
		int boundaryIdx = ct_raw.indexOf("boundary=");
		if (boundaryIdx)
			boundary = ct_raw.mid(boundaryIdx + 9);

		QList<QByteArray> parts;
		qint64 idx = in_data.indexOf(boundary);
		qint64 lastBoundaryEnd=-1;

		while(idx>=0)
		{
			//find end of line
			qint64 boundary_start = idx;
			while(boundary_start>0)
			{
				char leftOfBoundary = in_data.at(boundary_start-1);
				if (leftOfBoundary == '\n')
					break;
				if (leftOfBoundary == '\r')
					break;
				--boundary_start;
			}
			idx+=boundary.size();
			while(idx<in_data.size())
			{
				if ((in_data.at(idx)=='\n') || (in_data.at(idx)=='\r'))
					break;
				++idx;
			}
			while((in_data.at(idx)=='\n') || (in_data.at(idx)=='\r'))
			{
				++idx;
				if (idx>=in_data.size())
					break;
			}

			if (lastBoundaryEnd>=0)
			{
				parts.push_back(in_data.mid(lastBoundaryEnd, boundary_start-lastBoundaryEnd));
			}
            if (idx>=in_data.size())
                break;
			lastBoundaryEnd = idx;
			idx = in_data.indexOf(boundary, lastBoundaryEnd);
		}

		foreach(QByteArray part, parts)
        {
			QString fieldName;
			bool isFieldData=false;
			bool headersDone=false;
			int start=0;
			int next=part.indexOf('\n');
			QByteArray fieldData;
			while(next>=0)
			{
				QByteArray line = part.mid(start, next-start+1);
				start=next+1;
				next = part.indexOf('\n', start);
				if ((!isFieldData) && (line.startsWith("Content-Disposition:")))
				{ //20 is length of header name
					int idx = line.indexOf(';', 20);
					if (idx<0)
						continue;
					QString disposition = QString::fromLocal8Bit( line.mid(20, idx-20).trimmed());
					auto headers = parseHeaderFields(line.mid(idx).trimmed());
					if (!headers.contains("name")) // parts with no name are ignored.
						break;
					isFieldData = true;
					fieldName = headers.value("name");
					continue;
				}
				if (isFieldData && line.trimmed().isEmpty())
				{
					headersDone = true;
					continue;
				}
				if ((isFieldData) && (headersDone))
				{
					fieldData.append(line);
				}
			}
			if (!fieldName.isEmpty())
				formData.insert(fieldName, fieldData);
		}
		return formData;
	}

	formData.clear();
    return formData;
}

QMap<QString, QByteArray> RequestWrapper::cookies(Request * req)
{
        QList<QByteArray> cookies = req->parameter("HTTP_COOKIE").split(';');
        QMap<QString, QByteArray> cookieJar;
        for(QByteArray s: qAsConst(cookies))
        {
            s=s.trimmed();
            if (s.isEmpty())
                continue;

            int sep = s.indexOf('=');
            if (sep<1)
                cookieJar.insert(QString::fromLocal8Bit(s),"");
            else
                cookieJar.insert(QString::fromLocal8Bit(s.left(sep)), s.mid(sep+1));
        }
        return cookieJar;
}

QMap<QString, QByteArray> RequestWrapper::cookies()
{
    return cookies(mRequest);
}

Request *RequestWrapper::request() const
{
    return mRequest;
}

bool RequestWrapper::bufferResponse() const
{
    return mBufferResponse;
}

void RequestWrapper::setBufferResponse(bool newBufferResponse)
{
    mBufferResponse = newBufferResponse;
}
}
