#pragma once

class CDLMS46
{
public:
	CDLMS46();
	~CDLMS46();

	bool EncodePacket(void* pTask);

	bool DecodePacket(const char* pRecvData, UInt32 nDataLen, void* pTask);

private:

};

