#include "fastcgi_types.h"
#include "fcgi.h"
#include <QByteArray>
namespace FastCGI
{

int readFcgiVarLength(const QByteArray data, int offset, quint32 *value)
{
	if ((offset + 3) >= data.size())
	{
		return -1;
	}
	quint32 val;
	val = (unsigned char) data.at(offset++);
	if (val & 0x80)
	{
		val&=0x7f;
		val <<=8;
		val |= (unsigned char) data.at(offset++);
		val <<=8;
		val |= (unsigned char) data.at(offset++);
		val <<=8;
		val |= (unsigned char) data.at(offset++);
	}
	if (value)
		*value = val;
	return offset;
}

int readFcgiUint16(const QByteArray data, int offset, quint16 *value)
{
	if ((offset + 1) >= data.size())
	{
		return -1;
	}
	quint16 val=0;
	val  = (unsigned char) data.at(offset++);
	val <<=8;
	val |= (unsigned char) data.at(offset++);
	if (value)
		*value = val;
	return offset;
}

bool readHeader(QByteArray data, int offset, RecordHeader *d)
{
	if ((offset + FCGI_HEADER_LEN) > data.size())
	{
		return false;
	}
	if (!d)
	{
		return true;
	}
	d->version = (unsigned char)data.at(offset++);
	d->type    = (unsigned char)data.at(offset++);
	offset = readFcgiUint16(data, offset, &(d->requestId));
	if (offset<0)
		return false;
	offset = readFcgiUint16(data, offset, &(d->contentLength));
	if (offset<0)
		return false;
	d->paddingLength = (unsigned char)data.at(offset++);
	d->reserved      = (unsigned char)data.at(offset++);
	if (offset<0)
		return false;
	return true;
}

QByteArray serialzizeHeader(const RecordHeader & h)
{
	unsigned char buffer[FCGI_HEADER_LEN];
	buffer[0] = h.version;
	buffer[1] = h.type;
	buffer[2] = (h.requestId >> 8) & 0xff;
	buffer[3] = h.requestId & 0xff;
	buffer[4] = (h.contentLength>> 8) & 0xff;
	buffer[5] = h.contentLength & 0xff;
	buffer[6] = h.paddingLength;
	buffer[7] = h.reserved; //reserved
	return QByteArray((char*)buffer, 8);
}
}
