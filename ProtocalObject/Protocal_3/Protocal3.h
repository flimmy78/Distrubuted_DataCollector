#pragma once

#include "../../include/common.h"
#include "../../include/ProtocalInterFace.h"
using namespace WSProtocalInterFace;

class CProtocal3 : public IProtocalObject
{
public:

	CProtocal3();
	virtual ~CProtocal3();


	virtual bool InitObject(void* pDB) override;

	/*	����Э�����ͷ��

	@ pData: ��ӦtagTASKPacket�ṹ��

	@ ret���ɹ���1 ʧ�ܣ�0

	ע��ͨ��pData->szRecvData�����롣�������м����ݴ���pData->szReturnData

	������֡�շ���ʱ��ͨ���м�֡���������б��롣
	*/
	virtual bool EncodePacket(void* pTask) override;

	/* ���롣

	@ pData ��tagTask�����豸���ص������⣬��Ҫ�����ն˵�SN�룬���Ƶ�SN�룬�����SN��

	@ ret���ɹ���1 ʧ�ܣ�0

	ע��ͨ��pData->szRecvData�����롣�������м����ݴ���pData->szSendData

	pData->szReturnData�������յĽ�����ݡ�������֡�շ���ʱ�����Ҫ�����м�֡��
	*/
	virtual bool DecodePacket(void* pData, UInt32 nDataLen, void* pTask) override;

};