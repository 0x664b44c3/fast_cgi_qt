#ifndef FASTCGI_REQUEST_H
#define FASTCGI_REQUEST_H

#include <QObject>
#include <QByteArray>
#include <QMap>
namespace FastCGI {


struct RecordHeader;
class Connection;

class Request : public QObject
{
	friend class Connection;
	Q_OBJECT
public:
	explicit Request(quint16 requestId, Connection *connection= nullptr);
	quint16 role() const;
	const QByteArray &getStdInBuffer() const;

	int writeStdOut(const char *);
	int writeStdOut(const QByteArray &);
	int writeStdOut(const char * , int length);

	int writeStdErr(const char *);
	int writeStdErr(const QByteArray &);
	int writeStdErr(const char * , int length);

	enum requestState
	{
		rsNew=0,
		rsParametersDone,
		rsStdInDone,
		rsFinished
	};

	requestState getState() const;

	const QByteArray parameter(QString) const;
	QStringList parameterNames() const;
	const QMap<QString, QByteArray> &parameters() const;

	bool isFinished() const;

signals:
	/** is emitted once the request has been fully initialized */
	void created(quint16);

	/** is emitted if the request is cancelled by the server */
	void aborted(quint16);

	/** is emited once all parameters have been received */
	void parametersReady(quint16);

	/** is emitted whenever the stdin stream receives data
	 *  @param requestId is provided for convenience
	 */
	void readyRead(quint16)
	;
	/** is emitted once the stdin channel has finished */
	void stdInFinished(quint16);

	void finished(quint16);

public slots:
	/** will signal the webserver that this request is finished by the cgi app */
	void finishRequest(quint32 retval=0);

	/** @brief flushes both output streams' buffers */
	void flush();
	/** flush stdout */
	void flushStdout();
	/** flush stderr */
	void flushStderr();

protected:
	bool parseRecord(RecordHeader *, const QByteArray &);

private:


	quint16 mRequestId;
	Connection *mConnection;
	quint16 mRole;
	quint8 mFlags;

	requestState mReqState;
	bool mBufferedOut;


	QByteArray mStdinBuffer;
	QByteArray mStdoutBuffer;
	QByteArray mStderrBuffer;
	QMap<QString, QByteArray> mParameters;

	bool parseBeginRequest(const QByteArray & data);
	bool parseParams(const QByteArray & data);
	bool parseStdIn(const QByteArray &data);
};

} // namespace FastCGI

#endif // FASTCGI_REQUEST_H
