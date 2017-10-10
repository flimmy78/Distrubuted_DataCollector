#pragma once
/**********************************************************************************************

**********************************************************************************************/
#define PROTOCAL_INTERFACE class
#if defined(WIN32)
#define __PROTOCAL_INTERFACE_API __declspec(dllexport)
#else
#define __PROTOCAL_INTERFACE_API
#endif

namespace WSProtocalInterFace
{
#if defined(WIN32) || defined(WIN64)
	//
	// Windows/Visual C++
	//
	typedef signed char            Int8;
	typedef unsigned char          UInt8;
	typedef signed short           Int16;
	typedef unsigned short         UInt16;
	typedef int					   Int32;
	typedef unsigned int           UInt32;
	typedef signed __int64         Int64;
	typedef unsigned __int64       UInt64;
#else
	//
	// Unix/GCC
	//
	typedef signed char            Int8;
	typedef unsigned char          UInt8;
	typedef signed short           Int16;
	typedef unsigned short         UInt16;
	typedef int					   Int32;
	typedef unsigned int           UInt32;
	typedef signed long            IntPtr;
	typedef signed long long	   Int64;
	typedef unsigned long long	   UInt64;
#endif

	PROTOCAL_INTERFACE IProtocalObject
	{
	public:
		
		virtual bool InitObject(void* pDB) = 0;

		/*	����Э�����ͷ��

			@ pData: ��ӦtagTASKPacket�ṹ��

			@ ret���ɹ���1 ʧ�ܣ�0

			ע��ͨ��pData->szRecvData�����롣�������м����ݴ���pData->szReturnData

			������֡�շ���ʱ��ͨ���м�֡���������б��롣
		*/
		virtual bool EncodePacket(void* pTask) = 0;

		/* ���롣
			
			@ pData ��tagTask�����豸���ص������⣬��Ҫ�����ն˵�SN�룬���Ƶ�SN�룬�����SN��

			@ ret���ɹ���1 ʧ�ܣ�0

			ע��ͨ��pData->szRecvData�����롣�������м����ݴ���pData->szSendData
			
			pData->szReturnData�������յĽ�����ݡ�������֡�շ���ʱ�����Ҫ�����м�֡��
		*/
		virtual bool DecodePacket(void* pData, UInt32 nDataLen, void* pTask) = 0;

	};
	
};

extern "C"
{
	__PROTOCAL_INTERFACE_API bool CreateProtocalObject(WSProtocalInterFace::IProtocalObject** pProtocalObject);
}