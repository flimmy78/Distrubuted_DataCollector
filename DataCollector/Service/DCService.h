#pragma once

/*
	@ ����ʱ�䣺2017.7.10

	@ ���˵����

	1�������������̣߳���֯��������->������������->�ȴ�����->�жϳ�ʱ->�������->����ָ��״̬->��һ����������

	2���泭�������̣߳���֯��������->������������->ѭ��ִ��ǰ����̡�

	3���泭��������̣߳��ȴ�����->�жϳ�ʱ->�������->����ָ��״̬->ѭ��ִ��ǰ����̡�

	4����CTaskManage���ȡ�������泭����

	5����DeviceManage���ж��豸�Ƿ����ߡ�

	6��ProtocalInterFace������Э��ı���ͽ��롣

	7����ʱ��������������SOCKET���ӣ���ν��ʱ����û���κ���������ʱ�ŷ����������������ӱ����

	@ �޸ļ�¼��
*/

#ifdef WIN32
#include "service_base.h"
#endif

#include "../DeviceManage.h"
#include "../TaskManage.h"
#include "../../include/DBInterFace.h"
#include "../../include/ProtocalInterFace.h"
#include "../config.h"
#include <map>
using namespace WSProtocalInterFace;
typedef bool(*PCreateProtocalObject)(IProtocalObject** pProtocalObject);

class DCService : public ServiceBase
{
public:

#ifdef WIN32

#ifdef _DEBUG
	DCService(const CString& name) 
		: ServiceBase(name,
		name,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		SERVICE_ACCEPT_STOP
		)
	{

	}
#else
	DCService(const CString& name) 
		: ServiceBase(name,
		name,
		SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL,
		SERVICE_ACCEPT_STOP
		)
	{

	}
#endif
	~DCService(void)
	{
		
	}
#endif
	bool Run(LPCTSTR param = "");

private:
	void OnStart(DWORD argc, TCHAR* argv[]) override;
	void OnStop() override;
	DISABLE_COPY_AND_MOVE(DCService);

	/* �������ļ��л�ȡ������Ϣ */
	bool LoadFromconfig();

	// ����Э�鴦��ģ��
	void LoadProtocalFromDLL();

	// �泭�������������շ��̺߳���	
	static void commonTaskSendThreadFunc(void * pParam);
	static void commonTaskRecvThreadFunc(void * pParam);

	// �����������������շ��̺߳���(�ա��¶��ᡢ��������)
	static void specialTaskSendThreadFunc(void * pParam);
	static void specialTaskRecvThreadFunc(void * pParam);

	// ��鶨�泭�����״̬���Ƿ�ʱ���Ƿ��Ѿ�������ȣ�
	static void specialTaskStatusThreadFunc(void* pParam);
	static void commonTaskStatusThreadFunc(void* pParam);

	// ���������ϱ����ݵ��̺߳���
	static void reportingThreadFunc(void* pParam);

	// �����̺߳���(�泭)
	void ProcessCommonTaskRecvThreadFunc();
	void ProcessCommonTaskSendThreadFunc();
	void ProcessCommonTaskStatusThreadFunc();

	// �����̺߳���(����)
	void ProcessSpecialTaskSendThreadFunc();
	void ProcessSpecialTaskRecvThreadFunc();
	void ProcessSpecialTaskStatusThreadFunc();

	// �����ϱ���������
	void ProcessReportingThreadFunc();
	// �����յ���֡��������Э�����ͣ�ֻ�������ϱ�������
	unsigned char DetectProtocal(const char* pFrame, int nFrameLen, unsigned char& nMeterType);

	// �����ƻ�����
	void ProcessScheduleTask(const string& strScheduleType);

	// ����ǰ�÷��������������泭����һ�����ӣ�
	bool ConnectServer();

	// �������������ж��µ�һ����µ�һ�µ���
	bool IsNewMonth();
	bool IsNewDay();
	bool IsTimeRange();

	// ���������ա��²�������RT_TASK_MONTHS, RT_TASK_DAYS�����ű���
	void InsertMonthAndDayTask(UInt32 nYear, UInt32 nMonth, 
		UInt32 nDay, UInt32 nMonthBackword=0, UInt32 nDayBackword=0);

	void InsertScheduleTaskData(const string& strTableName, const string& strColName, 
		UInt32 nTableColCnt, map<UInt8, vector<string>> mapInsertData);

private:
	/* ���ݿ���ز��� */
	string m_strDBIP				= "";
	string m_strDBName				= "";
	string m_strDBPassWord			= "";
	string m_strDBSID				= "";
	string m_strDBType				= "";
	unsigned long m_nDBPort			= 0 ;
	string m_strDBTag				= "";
	
	/* ��ǰ��ͨѶ����ز���*/
	string m_strServerIP			= "";
	unsigned long m_nServerPort		= 0 ;
	string m_strLocalIP				= "";

	/* ����ģ������豸������ */
	CDeviceManage m_deviceManage;

	/* �������ģ�� */
	CTaskManage m_taskManage;

	// �泭�������ݴ���
	CClientTCP	m_tcpCommClient;

	// �����������ݴ���
	CClientTCP	m_tcpSpeciaClient;

	// �����ϱ��������ݴ���
	CClientTCP	m_tcpReportingClient;

	IDBObject* m_pDBObject			= nullptr;
	common::Config*	m_pConf			= nullptr;

	// ϵͳ����ʱ��,��Ҫ�����ж��µ�һ��
	struct tm m_startTime;

	// �����������������
	int m_nDaysBackword = 0;
	// �����������������
	int m_nMonthBackword = 0;
	// ����ʱʱ��
	UInt32 m_nTaskTimeout = 30;

	// ��������Ŀ�ʼʱ��ͽ���ʱ��
	struct tm m_ScheduleTask_STime;
	struct tm m_ScheduleTask_ETime;

/***********************************************************************************
	
	�߳����
*/
	// ����ǰ���������ݡ����롢�����߳�
	std::thread	m_commonTaskRecvThread;
	
	// ��ȡ�泭���񡢱��롢���͵��߳�
	std::thread	m_commonTaskSendThread;

	// �泭����״̬�ж��߳�
	std::thread	m_commonTaskStatusThread;

	// ��ȡ�����������¶��ᡢ���ߵȣ������롢���͵��߳�
	std::thread	m_specialTaskSendThread;

	// ����ǰ���������ݡ����롢�����߳�(����)
	std::thread	m_specialTaskRecvThread;

	// ��������״̬�ж��߳�
	std::thread	m_specialTaskStatusThread;

	// �˳��̵߳ı�ʶ
	bool	m_bQuitCommonTaskRecvThread		= false;
	bool	m_bQuitCommonTaskSendThread		= false;
	bool	m_bQuitCommonTaskStatusThread	= false;

	bool	m_bQuitSpecialTaskSendThread	= false;
	bool	m_bQuitSpecialTaskRecvThread	= false;
	bool	m_bQuitSpecialTaskStatusThread	= false;

	bool	m_bQuitReportingThread			= false;	
/***********************************************************************************/

	// Э���Ӧ�Ľӿ�ʵ��
	map<int, IProtocalObject*> m_mapProtocalOBJ;
	// �泭����
	map<unsigned int, TASKPACKET>	m_mapCommTask;
	// ��������
	map<unsigned int, TASKPACKET>	m_mapSpecialTask;
	// ��Ҫ����������������ݿ��ȡ��AFN+FN, PRODUCENAME TABLENAME, TYPE, TABLEPROPERTY
	map<string, tuple<string, string, string, string, string, map<UInt8, string>>> m_mapSpecialTaskDefine;

#ifdef WIN32
	SRWLOCK	 m_srwCommTask;
	SRWLOCK	 m_srwSpeciaTask;
#endif


};