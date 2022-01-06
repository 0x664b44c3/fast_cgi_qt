#pragma once
#ifndef FASTCGI_TYPES_H
#define FASTCGI_TYPES_H
#include <QtGlobal>
namespace FastCGI
{

enum
{
	MAX_STDIN_BUFFER=65536
};

struct RecordHeader
{
	quint8  version;
	quint8  type;
	quint16 requestId;
	quint16 contentLength;
	quint8  paddingLength;
	quint8  reserved;
};

enum RecordTypes
{
	recBEGIN_REQUEST=1,
	recABORT_REQUEST=2,
	recEND_REQUEST=3,
	recPARAMS=4,
	recSTDIN=5,
	recSTDOUT=6,
	recSTDERR=7,
	recDATA=8,
	recGET_VALUES=9,
	recGET_VALUES_RESULT=10,
	recUNKNOWN_TYPE=11,
	recMAXTYPE=recUNKNOWN_TYPE
};

enum RequestRoles
{
	roleUndefined  = 0,
	roleRESPONDER  = 1,
	roleAUTHORIZER = 2,
	roleFILTER     = 3
};

enum ProtocolStatusCodes {
	statREQUEST_COMPLETE=0,
	statCANT_MPX_CONN=1,
	statOVERLOADED=2,
	statUNKNOWN_ROLE=3
};


//functions for reading integer data types from bytestream
int readFcgiVarLength(const QByteArray data, int offset, quint32*value=0);
int readFcgiUint16(const QByteArray data, int offset, quint16 *value=0);
bool readHeader(QByteArray data, int offset, RecordHeader* d);
QByteArray serialzizeHeader(const RecordHeader & h);

}
#endif // FASTCGI_TYPES_H
