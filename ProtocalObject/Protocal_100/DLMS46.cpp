#include "stdafx.h"
#include "DLMS46.h"

CDLMS46::CDLMS46()
{
}

CDLMS46::~CDLMS46()
{
}

bool CDLMS46::EncodePacket(void* pTask)
{
	return true;
}

bool CDLMS46::DecodePacket(const char* pRecvData, UInt32 nDataLen, void* pTask)
{
	return true;
}