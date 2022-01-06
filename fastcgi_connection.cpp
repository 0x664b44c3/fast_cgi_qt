#include "fastcgi_connection.h"
#include <QIODevice>
#include "fcgi.h"
#include "fastcgi_types.h"
#include <QDebug>
#include "fastcgi_request.h"

//512 kbyte of buffer space max (plus one chunk)
//normally less will be used
#define BUFFER_MAX 512 * 1024
#define MAX_CHUNK 16384

struct fcgiRequest
{
	quint16 requestId;
	quint16 contentLength;

};
namespace FastCGI
{

Connection::Connection(QIODevice *dev, QObject *parent) : QObject(parent), mIoDev(dev)
{
	if (!mIoDev)
		return;
	connect(mIoDev, &QIODevice::readyRead, this, &Connection::onReadyRead);
}

void Connection::consumeBuffer()
{
	int old_offset=-1;
	int offset=0;
	while (offset<mBuffer.size())
	{
		if (old_offset == offset)
			break;
		old_offset=offset;
		if ((mBuffer.size() - offset )>= FCGI_HEADER_LEN)
		{
			RecordHeader hdr;
			if (readHeader(mBuffer, offset, &hdr))
			{
				int recEnd = offset + FCGI_HEADER_LEN + hdr.contentLength + hdr.paddingLength;
				if (recEnd >  mBuffer.size())
					break;
				offset+=FCGI_HEADER_LEN;
				QByteArray recordData = mBuffer.mid(offset, hdr.contentLength);
				unsigned int type = hdr.type;
				if (hdr.requestId)
				{
					Request * request = 0;
					if ((type==recBEGIN_REQUEST) && (!mRequests.contains(hdr.requestId)))
					{
						request = new Request(hdr.requestId, this);
						mRequests.insert(hdr.requestId, request);
						connect(request, &Request::parametersReady,
						        this, &Connection::onRequestParamsDone);
						connect(request, &Request::stdInFinished,
						        this, &Connection::onRequestStdinDone);
						connect(request, &Request::finished,
						        this, &Connection::onRequestFinished);
					}
					else
					{
						request = mRequests.value(hdr.requestId, nullptr);
					}
					if (request)
					{
						if (!request->parseRecord(&hdr, recordData))
						{
							qWarning()<<"Could not parse record type"<<type
							         << "for request id"<<hdr.requestId;
						}
					}
					else
					{
						qWarning()<<"Unknown request ID:"<<hdr.requestId;
					}
				}
				else
				{ //management record, requestId=0


				}
				offset = recEnd;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	mBuffer.remove(0, offset);
}
void Connection::onReadyRead()
{
	while (!mIoDev->atEnd())
	{
		if (mBuffer.size() < BUFFER_MAX)
		{
			QByteArray rd = mIoDev->read(MAX_CHUNK);
			mBuffer.append(rd);
		}
		consumeBuffer();
		if (mBuffer.size() > BUFFER_MAX)
			break;
	}
}

void Connection::onRequestParamsDone(quint16 rid)
{
	auto request = mRequests.value(rid, nullptr);
	if (request)
		emit newRequest(request);
}

void Connection::onRequestStdinDone(quint16 rid)
{
	auto request = mRequests.value(rid, nullptr);
	if (request)
	{
		emit requestStdinDone(request);
	}
}

void Connection::onRequestFinished(quint16 id)
{
	auto request = mRequests.value(id, nullptr);
	if (request)
	{
		mRequests.remove(id);
		emit requestFinished(id);
		request->deleteLater();
	}
}

void Connection::sendRecord(quint16 requestId, unsigned char type, const QByteArray & payload)
{
//	qDebug()<<"send record"<<requestId<<type<<payload.size();
	RecordHeader hdr;
	hdr.version = FCGI_VERSION_1;
	hdr.type = type;
	hdr.requestId = requestId;
	hdr.contentLength = payload.size();
	hdr.paddingLength=0;
	hdr.reserved = 0;
	mIoDev->write(serialzizeHeader(hdr).append(payload));
}

void Connection::write(const QByteArray & data)
{
	mIoDev->write(data);
}

void Connection::closeConnection()
{
	mIoDev->close();
	emit connectionClosed();
}
}
