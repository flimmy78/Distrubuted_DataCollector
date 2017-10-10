#pragma once

/************************************************************************
	@ ����ʱ�䣺2017.7.10

	@ ���˵����
	
		***������ֻ����һ���̣߳��������������շ�***

		***����֧�ֶ��̰߳�ȫ***

		1����ȡע���豸�б���

		2���ж��豸�����ߣ��豸��������ǰ�÷������ݰ����жϣ���

		3����ȡ�豸״̬�������У�����ʹ���У�ÿ���豸ֻ���շ�һ�������

		4���ڰ��������ã��Ƿ��֪ǰ����Щ�豸�����ӣ���Щ��������������

		5����ǰ�ñ������ӣ��������
		
		---------------------------------------------------------------
		���ݿ���ر���T_CODE_CST_TYPE		T_RUN_CST 
		---------------------------------------------------------------
		��Ҫ���ֶ�		CST_ADDR			SN_PROTOCAL				
		---------------------------------------------------------------
	@ �޸ļ�¼��

************************************************************************/
#include "../include/dbinterface.h"
#include "../include/common.h"
#include "./LOG/log4z.h"

#include "TCP.h"
#include <thread>
#include <map>
using namespace std;
using namespace WSDBInterFace;
using namespace WSCOMMON;
using namespace WSNET;
using namespace zsummer::log4z;

#ifdef WIN32
#include <windows.h>
#endif

#define SYNCPACKETLEN	1500

#define BATCHREADSQL	"select A.CST_ADDR, B.SN_PROTOCAL, A.CST_ID from T_RUN_CST A INNER JOIN T_CODE_CST_TYPE B ON A.SN_CST_TYPE = B.SN"

#define BATCHINSERTSQL 

class CDeviceManage
{
	// �ն��豸���ݽṹ
	typedef struct tagTerminalDeviceDB
	{
		string			strDevice_sn		= "";	// �豸��Ψһ����
		string			strDevice_name		= "";	// �ն˱�ʶ��
		struct tm		sDevice_OnTime		= {0};	// �豸���ߵ�ʱ��		
		struct tm		sDevice_OffTime		= {0};	// �豸���ߵ�ʱ��		
		unsigned long	nDevice_IP			= 0;	// �ն�IP��ַ����еĻ�
		unsigned int	nDevice_Port		= 0;	// �ն˶˿ں�����еĻ�
		unsigned char	cDevice_Protocal	= 0;	// �ն�Э��
		int				nDeviceStatus       = 0;	// �ն�����״̬

	}TERMINALDEVICEDB, PTERMINNALDEVICEDB;

public:
	CDeviceManage();
	~CDeviceManage();

	/************************************************************************
		@ ��ʼ�������¼�������

		1: �������ݿ���� ��2�������ݿ⵼��ע���豸��

		3����ǰ�ý���TCP���ӡ�4�������豸�ڰ�������

		5�������߳̽����������ݣ��豸�����ߣ�

		@ pDBObj : ���ݿ����

		@ strIP : ǰ�ö�Ӧ��IP��ַ

		@ nPort : ǰ�ö�Ӧ�Ķ˿� 
	************************************************************************/
	bool Init(IDBObject* pDBObj, const string &strDBTag, const string& strLocalIP, 
		const string& strServerIP, UInt32 nServerPort, UInt32 nLogLevel = 0);

	/*************************************************************************
		@ ����ʼ�������¼�������

		1: �������ݿ���� ��

		2����ձ���ע���豸�����ݡ�

		3���ر�TCP���ӡ�

		4����ռ����豸�ڰ�������
	*************************************************************************/
	bool UnInit();

	/*************************************************************************
		@ ��鵱ǰ�豸��״̬�����¼�������ֵ��

		-1���豸���߲�����

		0���豸��������ʹ��

		1���豸δע��

		2���豸����ʹ����
	*************************************************************************/
	Int32 CheckDeviceStatus(const string& strDeviceName);

	Int32 UpdateDeviceStatus(const string& strDeviceName, Int32 nDeviceStatus);

private:

	// �����ݿ��ȡע���豸(�����ڰ�����)
	bool LoadDeviceFromDB();

	// ����������
	int  SendHeartBeat();		

	// TCP���ӷ�����
	bool ConnectServer();		

	// �������ӷ�����
	bool ReConnectServer();		

	// ���������շ��̺߳���
	static void RecvThreadFunc(void * pParam);
	static void SendThreadFunc(void * pParam);

	// �����̺߳���
	void ProcessRecvThreadFunc();
	void ProcessSendThreadFunc();

	// ��ǰ�ý�������
	int	ReceiveFromServer();

private:
	// ���ݿ��������
	IDBObject*						m_pDBObject						= nullptr;
	// ����ţ���Ȼ��
	UInt32							m_nPackIdx						= 0;
	// ǰ�û���Ӧ�ļ���IP��ַ
	string							m_strServerIP					= "";
	// ǰ�û���Ӧ�ļ���PORT�˿�
	UInt32							m_nServerPort					= 0;
	// ����IP��ַ
	string							m_strLocalIP					= "";
	// ʱ��TICK��������������ʱ
	UInt32							m_nTickCount					= 0;
	// �Ƿ�ͬ������(һ��������ǲ���Ҫͬ����)
	bool							m_bSyncTCPHeader				= false;
	// ���TCP���ݻ��ң�����TCPͬ���Ļ�����
	char							m_szSyncBuf[SYNCPACKETLEN]		= { 0 };
	// ���TCP���ݻ��ң�����TCPͬ���Ļ�����
	UInt32							m_nReadIdx						= 0;
	// ����������ͬ��ҵ���в�ͬ��������
	unsigned char m_szHeatBeat[32]									= { 0 };
	// ���ݿ�ı�ʶ��
	string							m_strDBTag						= "";
	string							m_strDBConTag					= "";

	// �˳��շ����������̵߳ı�ʶ
	bool							m_bQuitSendThread				= false;
	bool							m_bQuitRecvThread				= false;

	// ��־
	ILog4zManager					*m_pLog							= nullptr;
	LoggerId						m_loggerId						=0;

	// ��¼ע���豸����Ϣ/*name*/
	map<string, TERMINALDEVICEDB>	m_mapDeviceInfo;
	// ����һ���߳̽�����������
	std::thread						m_netRecvThread;
	// ����һ���̷߳�����������
	std::thread						m_netSendThread;
	// ǰ��ͬ���������豸��Ϣ
	TERMINALDEVICE					m_sDeviceInfo[20];
	// TCP����ṹ��Ϣ
	TCPPACKET						m_sTCPHeader;
	// �������ݴ���
	CClientTCP						m_tcpClient;
	// �������ݿ��ֶ�device name
	vector<string>					m_vecout1;
	// �������ݿ��ֶ�device proto
	vector<string>					m_vecout2;
	// �������ݿ��ֶ�device SN
	vector<string>					m_vecout3;

#ifdef WIN32
	// ��д��
	SRWLOCK	 m_srwDevice;
#endif
};