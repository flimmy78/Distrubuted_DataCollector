#include "stdafx.h"
#include "DLMS47.h"

CDLMS47::CDLMS47()
{
}

CDLMS47::~CDLMS47()
{
}

bool CDLMS47::EncodePacket(void* pTask)
{
	return true;
}

bool CDLMS47::DecodePacket(const char* pRecvData, UInt32 nDataLen, void* pTask)
{
	return true;
}