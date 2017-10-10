#pragma once


class CDLMS47
{
public:
	CDLMS47();
	~CDLMS47();

	bool EncodePacket(void* pTask);

	bool DecodePacket(const char* pRecvData, UInt32 nDataLen, void* pTask);

private:

};

