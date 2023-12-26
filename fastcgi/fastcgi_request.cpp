#include "fastcgi_request.h"
#include "fastcgi_connection.h"
#include "fastcgi_types.h"
#include "fcgi.h"
#include <QDebug>

#define MAX_STREAM_CHUNK 4096

namespace FastCGI {

Request::Request(quint16 requestId, Connection *connection) :
    QObject(connection),
    mRequestId(requestId),
    mConnection(connection),
    mReqState(rsNew),
    mBufferedOut(true)

{
	mFlags=0;
	mRole = 0;

}

bool Request::parseRecord(RecordHeader * hdr, const QByteArray & data)
{

	bool ret=false;
	//not for us, how did we get here?
	if (hdr->requestId!=mRequestId)
		return true;
//	qDebug()<<"process record"<<hdr->type<<data.size();
	switch(hdr->type)
	{
		case recBEGIN_REQUEST:
			ret = parseBeginRequest(data);
			if (ret)
				emit created(mRequestId);
			break;
		case recABORT_REQUEST:
			emit aborted(mRequestId);
			finishRequest(0);
			break;
		case recPARAMS:
			ret = parseParams(data);
			break;
		case recSTDIN:
			ret = parseStdIn(data);
			break;
		case recDATA: //secondary input stream, we ignore this for now
			ret = true;
			break;

		default:
			return false;
			break;
	}

	return ret;
}

const QMap<QString, QByteArray> &Request::parameters() const
{
	return mParameters;
}

bool Request::isFinished() const
{
	return (mReqState == rsFinished);
}

const QByteArray &Request::getStdInBuffer() const
{
	return mStdinBuffer;
}

int Request::writeStdOut(const char * data)
{
	if (mReqState == rsFinished)
		return 0;
	QByteArray data_array(data);
	mStdoutBuffer.push_back(data_array);
	if ((!mBufferedOut) || (mStdoutBuffer.size() > (MAX_STREAM_CHUNK / 2)))
		flushStdout();
	return data_array.size();
}

int Request::writeStdOut(const QByteArray & data)
{
	if (mReqState == rsFinished)
		return 0;
	mStdoutBuffer.push_back(data);
	if ((!mBufferedOut) || (mStdoutBuffer.size() > (MAX_STREAM_CHUNK / 2)))
		flushStdout();
	return data.size();
}

int Request::writeStdOut(const char * data, int length)
{
	if (mReqState == rsFinished)
		return 0;
	QByteArray data_array(data, length);
	mStdoutBuffer.push_back(data_array);
	if ((!mBufferedOut) || (mStdoutBuffer.size() > (MAX_STREAM_CHUNK / 2)))
		flushStdout();
	return data_array.size();
}

int Request::writeStdErr(const char * data)
{
	if (mReqState == rsFinished)
		return 0;
	QByteArray data_array(data);
	mStderrBuffer.push_back(data_array);
	if ((!mBufferedOut) || (mStderrBuffer.size() > (MAX_STREAM_CHUNK / 2)))
		flushStderr();
	return data_array.size();
}

int Request::writeStdErr(const QByteArray & data)
{
	if (mReqState == rsFinished)
		return 0;
	mStderrBuffer.push_back(data);
	if ((!mBufferedOut) || (mStderrBuffer.size() > (MAX_STREAM_CHUNK / 2)))
		flushStderr();
	return data.size();
}

int Request::writeStdErr(const char * data, int length)
{
	if (mReqState == rsFinished)
		return 0;
	QByteArray data_array(data, length);
	mStderrBuffer.push_back(data_array);
	if ((!mBufferedOut) || (mStderrBuffer.size() > (MAX_STREAM_CHUNK / 2)))
		flushStderr();
	return data_array.size();
}

Request::requestState Request::getState() const
{
	return mReqState;
}

const QByteArray Request::parameter(QString key) const
{
	return mParameters.value(key);
}

QStringList Request::parameterNames() const
{
	return mParameters.keys();
}

void Request::finishRequest(quint32 retval)
{

	if (mReqState == rsFinished)
		return;

	//make sure all stdout/stderr buffers have been sent
	flush();
	//close both output streams with an empty message
	mConnection->sendRecord(mRequestId, recSTDOUT, QByteArray());
	mConnection->sendRecord(mRequestId, recSTDERR, QByteArray());

	FCGI_EndRequestBody message;
	message.protocolStatus = statREQUEST_COMPLETE;
	message.appStatusB0 = retval & 0xff;
	retval<<=8;
	message.appStatusB1 = retval & 0xff;
	retval<<=8;
	message.appStatusB2 = retval & 0xff;
	retval<<=8;
	message.appStatusB3 = retval & 0xff;

	mReqState = rsFinished;


	mConnection->sendRecord(mRequestId, recEND_REQUEST, QByteArray((char*)&message, sizeof(FCGI_EndRequestBody)));

	//if the server has asked us to close the connection, once we are done, this is the time
	if ((mFlags & FCGI_KEEP_CONN) == 0)
	{
		mConnection->closeConnection();
	}
	emit finished(mRequestId);
}

void Request::flush()
{
	//flush stderr first
	flushStderr();
	//flush stdout next
	flushStdout();

}

void Request::flushStdout()
{
	//we discard data once the request has been marked "finished"
	if (mReqState == rsFinished)
		return;
	int offset=0;
	while(offset<mStdoutBuffer.size())
	{
		int bytes = std::min(mStdoutBuffer.size(), MAX_STREAM_CHUNK);
		mConnection->sendRecord(mRequestId,
		                        recSTDOUT,
		                        mStdoutBuffer.mid(offset, bytes));
		offset+=bytes;
	}
	mStdoutBuffer.remove(0, offset);
}

void Request::flushStderr()
{
	//we discard data once the request has been marked "finished"
	if (mReqState == rsFinished)
		return;
	int offset=0;
	while(offset<mStderrBuffer.size())
	{
		int bytes = std::min(mStderrBuffer.size(), MAX_STREAM_CHUNK);
		mConnection->sendRecord(mRequestId,
		                        recSTDERR,
		                        mStderrBuffer.mid(offset, bytes));
		offset+=bytes;
	}
	mStderrBuffer.remove(0, offset);
}

bool Request::parseBeginRequest(const QByteArray &data)
{
	if (data.size() < ((int) sizeof(FCGI_BeginRequestBody)))
		return false;
	int offset=0;
	offset = readFcgiUint16(data, 0, &mRole);
	if (offset<0)
		return false;
	mFlags = data.at(offset);
	return true;
}

bool Request::parseStdIn(const QByteArray &data)
{
	//we discard data once the request has been marked "finished"
	if (mReqState == rsFinished)
		return false;
	if (data.size())
	{
		if (mStdinBuffer.size() < MAX_STDIN_BUFFER)
		{
			mStdinBuffer.push_back(data);
			emit readyRead(mRequestId);
		}
	}
	else
	{
		mReqState = rsStdInDone;
		emit stdInFinished(mRequestId);
	}
	return true;
}

bool Request::parseParams(const QByteArray &data)
{
	int offset=0;
	if (data.size() == 0)
	{
		mReqState = rsParametersDone;
		emit parametersReady(mRequestId);
	}

	while(offset<data.size())
	{
		quint32 nameSize, valueSize;
		offset = readFcgiVarLength(data, offset, &nameSize);
		if (offset<0)
			break;
		offset = readFcgiVarLength(data, offset, &valueSize);
		if (offset<0)
			break;
		int recEnd = offset + nameSize + valueSize;
		if (recEnd > data.size())
			break;
		QString key = QString::fromLocal8Bit(data.mid(offset, nameSize));
		offset+=nameSize;
		QByteArray value= data.mid(offset, valueSize);
		offset+=valueSize;
		mParameters.insert(key, value);
	}
	return true;
}

quint16 Request::role() const
{
	return mRole;
}

} // namespace FastCGI
