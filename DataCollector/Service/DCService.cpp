
#include "StdAfx.h"

#ifdef WIN32
#include "DCService.h"
#include <DbgHelp.h>
#include <IO.h>
#include <string>

#include "../xml/Markup.h"
using namespace std;



string strXML = "<?xml version=\"1.0\" encoding=\"UTF - 8\" standalone=\"yes\"?><COSEM_OPEN_REQ><ACSE_Protocol_Version Value=\"80\"/><Application_Context_Name Value=\"1\"/><Sender_Acse_Requirement Value=\"80\"><value><Name Type=\"octet - string\" Value=\"1223\"/></value></Sender_Acse_Requirement><Security_Mechanism_Name Value=\"1\"/><Calling_Authentication_Value Value=\"33333333\"/><DLMS_Version_Number Value=\"6\"/><DLMS_Conformance Value=\"007E1F\"/><Client_Max_Receive_PDU_Size Value=\"0\"/><Service_Class Value=\"1\"/></COSEM_OPEN_REQ>";


typedef map<unsigned int, TASKPACKET>::value_type mapTaskPair;

//生产DUMP文件 
LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS lpExceptionInfo)
{
	typedef BOOL(WINAPI * MiniDumpWriteDumpT)(
		HANDLE,
		DWORD,
		HANDLE,
		MINIDUMP_TYPE,
		PMINIDUMP_EXCEPTION_INFORMATION,
		PMINIDUMP_USER_STREAM_INFORMATION,
		PMINIDUMP_CALLBACK_INFORMATION
		);

	MiniDumpWriteDumpT pfnMiniDumpWriteDump = NULL;
	HMODULE hDbgHelp = LoadLibrary("DbgHelp.dll");
	if (hDbgHelp)
	{
		pfnMiniDumpWriteDump = (MiniDumpWriteDumpT)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
	}

	if (pfnMiniDumpWriteDump)
	{
		char szFileName[MAX_PATH] = { 0 };
		::GetModuleFileName(NULL, szFileName, _MAX_PATH);
		string str = szFileName;
		str = str.substr(0, str.length() - 4) + ".dmp";

		HANDLE hFile = ::CreateFile(str.c_str(), GENERIC_WRITE, 0, NULL, 
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION einfo;
			einfo.ThreadId = ::GetCurrentThreadId();
			einfo.ExceptionPointers = lpExceptionInfo;
			einfo.ClientPointers = FALSE;

			pfnMiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), 
				hFile, MiniDumpNormal, &einfo, NULL, NULL);
			::CloseHandle(hFile);
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	if (hDbgHelp != NULL)
	{
		FreeLibrary(hDbgHelp);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif



 void DCService::OnStart(DWORD argc, TCHAR* argv[])
{
	 CMarkup xml;
	 xml.SetDoc(strXML);
	 xml.FindChildElem();

	 string strTag = xml.GetTagName();
	 string strValue = xml.GetAttrib("Value");

	 bool bRet = xml.FindChildElem("Sender_Acse_Requirement");

	 strTag = xml.GetTagName();
	 strValue = xml.GetAttrib("Value");

	 xml.IntoElem();
	 xml.FindChildElem();

	 strTag = xml.GetTagName();
	 strValue = xml.GetAttrib("Value");

#ifdef WIN32
	 SetUnhandledExceptionFilter(ExceptionFilter);
#endif

	 InitializeSRWLock(&m_srwCommTask);
	 InitializeSRWLock(&m_srwSpeciaTask);

	 Getlocaltime(m_startTime);

	 // 初始化日志接口

	 // 初始化数据库模块
	 if (LoadFromconfig() && CreateDBObject(ORCAL_TYPE, &m_pDBObject))
	 {
		 string strError;
		 if (ConnectServer() && m_pDBObject->OpenDB(m_strDBSID, m_strDBName, 
			 m_strDBPassWord, m_strDBIP, m_nDBPort, m_strDBTag, strError))
		 {
			 // 加载协议处理模块
			 LoadProtocalFromDLL();

			 // 初始化设备管理模块 && 初始化任务管理模块
			 if ( m_deviceManage.Init(m_pDBObject, m_strDBTag, m_strLocalIP,
				 m_strServerIP, m_nServerPort) && 			 
				 m_taskManage.Init(m_pDBObject, m_strDBTag) )
			 {
				 // 加载需要处理的定抄数据项（AFN,FN,PROCEDURE_NAME,TABLE_NAME）
				 m_taskManage.GetScheduleTaskProperty(m_mapSpecialTaskDefine);

				 // 创建随抄数据接收线程 
				 m_commonTaskRecvThread = std::thread(commonTaskRecvThreadFunc, this);
				 if (m_commonTaskRecvThread.joinable())
					 m_commonTaskRecvThread.detach();

				 // 创建随抄任务数据发送线程
				 m_commonTaskSendThread = std::thread(commonTaskSendThreadFunc, this);
				 if (m_commonTaskSendThread.joinable())
					 m_commonTaskSendThread.detach();

				 // 创建随抄任务状态判断线程
				 m_commonTaskStatusThread = std::thread(specialTaskStatusThreadFunc, this);
				 if (m_commonTaskStatusThread.joinable())
					 m_commonTaskStatusThread.detach();

				 // 创建定抄任务数据发送线程
				 m_specialTaskSendThread = std::thread(specialTaskSendThreadFunc, this);
				 if (m_specialTaskSendThread.joinable())
					 m_specialTaskSendThread.detach();

				 // 创建定抄任务接收线程
				 m_specialTaskRecvThread = std::thread(specialTaskRecvThreadFunc, this);
				 if (m_specialTaskRecvThread.joinable())
					 m_specialTaskRecvThread.detach();

				 // 创建定抄任务状态判断线程
				 m_specialTaskStatusThread = std::thread(specialTaskStatusThreadFunc, this);
				 if (m_specialTaskStatusThread.joinable())
					 m_specialTaskStatusThread.detach();
			 }
		 }
	 }
}

 void DCService::OnStop()
{

}

 bool DCService::LoadFromconfig()
 {
	 if (m_pConf == nullptr)
	 {
		 m_pConf = new common::Config("config.ini");
	 }

	 if (m_pConf->valid())
	 {
		 m_strDBIP = m_pConf->getProperty("db_ip");
		 m_strDBName = m_pConf->getProperty("db_username");
		 m_strDBPassWord = m_pConf->getProperty("db_password");
		 m_strDBSID = m_pConf->getProperty("db_sid");
		 m_strDBType = m_pConf->getProperty("db_type");
		 m_nDBPort = atoi(m_pConf->getProperty("db_port").c_str());

		 m_strServerIP = m_pConf->getProperty("server_ip");
		 m_strLocalIP = m_pConf->getProperty("local_ip");
		 m_nServerPort = atoi(m_pConf->getProperty("server_port").c_str());

		 m_nDaysBackword = atoi(m_pConf->getProperty("DaysBackword").c_str());
		 m_nMonthBackword = atoi(m_pConf->getProperty("MonthsBackword").c_str());
		 m_nTaskTimeout = atoi(m_pConf->getProperty("TaskTimeout").c_str());

		 string strSTime = m_pConf->getProperty("ScheduleTaskSTime");
		 string strETime = m_pConf->getProperty("ScheduleTaskETime");

		 string strTemp = strSTime.substr(0, strSTime.length() - strSTime.find(':')-1);
		 m_ScheduleTask_STime.tm_hour = atoi(strTemp.c_str());
		 strTemp = strSTime.substr(strSTime.find(':')+1, strSTime.length() - strSTime.find(':'));
		 m_ScheduleTask_STime.tm_min = atoi(strTemp.c_str());
		
		 strTemp = strETime.substr(0, strETime.length() - strETime.find(':')-1);
		 m_ScheduleTask_ETime.tm_hour = atoi(strTemp.c_str());
		 strTemp = strETime.substr(strETime.find(':')+1, strETime.length() - strETime.find(':'));
		 m_ScheduleTask_ETime.tm_min = atoi(strTemp.c_str());

		 return true;
	 }
	 else
	 {
		 return false;
	 }
 }

 void DCService::LoadProtocalFromDLL()
 {
	 if (m_pDBObject != nullptr)
	 {
		 string strErrorInfo = "";
		 string strDBConTag = "";
		 string strSQL = "select SN from cfg_protocal";
		 if (!m_pDBObject->BeginBatchRead(m_strDBTag, strSQL, 1, strDBConTag))
			 return;

		 vector<string> vecSN;
		 m_pDBObject->SetReadColData(vecSN, m_strDBTag, strDBConTag);
		 m_pDBObject->ExcuteBatchRead(m_strDBTag, strDBConTag, strErrorInfo);

		 for (UInt32 i = 0; i < vecSN.size(); i++)
		 {
			 char szProtocalDLLName[256] = {0};
			 int nProtocal = atoi(vecSN[i].c_str());
			 sprintf_s(szProtocalDLLName, "Protocal_%d", nProtocal);
#ifdef WIN32
			 // 动态加载协议号对应的动态库文件（在同级目录下）
			 // 命名规范为"Protocal_" + 协议号(比如Protocal_3)
			 HMODULE hDll = LoadLibrary(szProtocalDLLName);
			 if (hDll != NULL)
			 {
				 // 获取创建协议对象的函数地址
				 PCreateProtocalObject pFunc = (PCreateProtocalObject)\
					 GetProcAddress(hDll, "CreateProtocalObject");

				 if (pFunc != nullptr)
				 {
					 // 通过函数创建协议对象
					 IProtocalObject* pProtocalOBJ = nullptr;
					 pFunc(&pProtocalOBJ);
					 if (pProtocalOBJ != nullptr && pProtocalOBJ->InitObject(m_pDBObject))
					 {
						 // 将协议对象保存起来，以供后续调用
						 typedef map<int, IProtocalObject*>::value_type mapPair;
						 m_mapProtocalOBJ.insert(mapPair(nProtocal, pProtocalOBJ));
						 continue;
					 }					 
				 }
				 // 不合法的协议库则卸载
				 FreeLibrary(hDll);
			 }
#endif
		 }
	 }
 }

 bool DCService::Run(LPCTSTR param)
{
	if (_tcscmp(param, _T("console")) == 0) 
	{

		TCHAR cinCmd[128];
		bool bStart = false;

		while(1) 
		{
			_tprintf(_T("->input \"exit\" quit\r\n"));

			if (!bStart)
			{
				OnStart(0, NULL);

				_tprintf(_T("\r\n========================================\r\n"));
				_tprintf(_T("-> start data collector\r\n"));
				_tprintf(_T("========================================\r\n"));
			}
			bStart = true;

			_tscanf_s(_T("%s"), cinCmd, 128);
			if (_tcsncmp(cinCmd, _T("?"), 1) == 0) 
			{
				_tprintf(_T("========================================\r\n"));
				_tprintf(_T("\"?\"\t  -show cmd help\r\n"));
				_tprintf(_T("\"start\"\t  -start data collector\r\n"));
				_tprintf(_T("\"stop\"\t  -stop data collector\r\n"));
				_tprintf(_T("\"exit\"\t  -exit data collector\r\n"));
				_tprintf(_T("========================================\r\n"));
			}
			else if (_tcsncmp(cinCmd, _T("start"), 5) == 0) 
			{
				if (!bStart) 
				{
					OnStart(0, NULL);

					_tprintf(_T("\r\n========================================\r\n"));
					_tprintf(_T("-> start data collector\r\n"));
					_tprintf(_T("========================================\r\n"));
				}
				bStart = true;
			}
			else if (_tcsncmp(cinCmd, _T("stop"), 4) == 0) 
			{
				if (bStart)
				{
					OnStop();

					_tprintf(_T("\n========================================\n"));
					_tprintf(_T("-> stop data collector\r\n"));
					_tprintf(_T("========================================\n"));
				}

				bStart = false;
			}
			else if (_tcsncmp(cinCmd, _T("exit"), 4) == 0) 
			{

				_tprintf(_T("\r\n========================================\r\n"));
				_tprintf(_T("-> exit data collector\r\n"));
				_tprintf(_T("========================================\r\n"));

				break;
			}
			else 
			{
				_tprintf(_T("invalid cmd\r\n"));
			}
		}

		if (bStart)
			OnStop();

		return true;
	}

	return ServiceBase::Run();
}

 void DCService::commonTaskRecvThreadFunc(void * pParam)
 {
	 DCService* pDevice = (DCService*)pParam;
	 pDevice->ProcessCommonTaskRecvThreadFunc();
 }

 void DCService::commonTaskSendThreadFunc(void * pParam)
 {
	 DCService* pDevice = (DCService*)pParam;
	 pDevice->ProcessCommonTaskSendThreadFunc();
 }

void DCService::commonTaskStatusThreadFunc(void* pParam)
{
	DCService* pDevice = (DCService*)pParam;
	pDevice->ProcessSpecialTaskStatusThreadFunc();
}

 void DCService::specialTaskSendThreadFunc(void * pParam)
 {
	 DCService* pDevice = (DCService*)pParam;
	 pDevice->ProcessSpecialTaskSendThreadFunc();
 }

 void DCService::specialTaskRecvThreadFunc(void * pParam)
 {
	 DCService* pDevice = (DCService*)pParam;
	 pDevice->ProcessSpecialTaskRecvThreadFunc();
 }

 void DCService::specialTaskStatusThreadFunc(void* pParam)
 {
	 DCService* pDevice = (DCService*)pParam;
	 pDevice->ProcessSpecialTaskStatusThreadFunc();
 }

 void DCService::reportingThreadFunc(void* pParam)
 {
	 DCService* pDevice = (DCService*)pParam;
	 pDevice->ProcessReportingThreadFunc();
 }

 // 根据收到的帧数据来判协议类型，只有主动上报会用上
 unsigned char DCService::DetectProtocal(const char* pFrame, int nFrameLen, unsigned char& nMeterType)
 {
	 if (pFrame != nullptr && nFrameLen > 0)
	 {
		 int nProtocal = -1;
		 if (pFrame[0] == 0x7E)
		 {
			 // 标准47协议
			 nProtocal = 100;
			 nMeterType = 23;
		 }
		 else if (pFrame[0] == 0x00)
		 {
			 nProtocal = 100;
			 nMeterType = 22;
		 }
		 else if (pFrame[0] == 0x68)
		 {
			 nProtocal = 4;
			 nMeterType = 0;
		 }
		 return nProtocal;
	 }

	 return -1;
 }

 // 主动上报处理函数
 void DCService::ProcessReportingThreadFunc()
 {
	 while (!m_bQuitReportingThread)
	 {
		 UInt8 cCommand;
		 char szRecvBuf[1500] = { 0 };
		 unsigned int nRecvLen = 0;
		 unsigned int nRecvID = 0;

		 // 获取前置返回的主动上报数据
		 if (m_tcpReportingClient.RecvContent(cCommand, szRecvBuf, nRecvLen, nRecvID) > 0)
		 {
			 if (cCommand == 0xD0)
			 {
				 // 随抄心跳包
			 }
			 else if (cCommand == 0xD1)
			 {
				 // 随抄任务包
				 TASKPACKET taskPacket;
				 unsigned char cProtocalType = DetectProtocal(szRecvBuf, nRecvLen, taskPacket.cMeterType);
				 map<int, IProtocalObject *>::iterator iterProtocal = m_mapProtocalOBJ.find(cProtocalType);
				 if (iterProtocal != m_mapProtocalOBJ.end())
				 {
					 // 解码成功
					 if (iterProtocal->second->DecodePacket(szRecvBuf, nRecvLen, &taskPacket))
					 {

					 }
					 else
					 {
						 // 解码失败
					 }
				 }
			 }
			 else
			 {
				 // 非法数据包
			 }
		 }
	 }
 }


 // 处理线程函数
 void DCService::ProcessCommonTaskRecvThreadFunc()
 {
	 while (!m_bQuitCommonTaskRecvThread)
	 {
		 UInt8 cCommand;
		 char szRecvBuf[1500] = { 0 };
		 unsigned int nRecvLen = 0;
		 unsigned int nRecvID = 0;

		 // 获取前置返回的随抄数据
		 if (m_tcpCommClient.RecvContent(cCommand, szRecvBuf, nRecvLen, nRecvID) > 0)
		 {
#ifdef WIN32
			 AcquireSRWLockShared(&m_srwCommTask);
#endif
			 map<unsigned int, TASKPACKET>::iterator iterComm = m_mapCommTask.find(nRecvID);
			 if (cCommand == 0xC0)
			 {
				 // 随抄心跳包
			 }
			 else if (cCommand == 0xC1 && iterComm != m_mapCommTask.end())
			 {
				 // 随抄任务包
				 unsigned char cProtocalType = iterComm->second.cProtocalType;
				 map<int, IProtocalObject *>::iterator iterProtocal = m_mapProtocalOBJ.find(cProtocalType);
				 if (iterProtocal != m_mapProtocalOBJ.end())
				 {
					 // 解码成功
					 if (iterProtocal->second->DecodePacket(szRecvBuf, nRecvLen, &iterComm->second))
					 {
						 // 解码后还有连续帧
						 if (iterComm->second.nTaskStatus == 2)
						 {
							 // 更新时间戳，防止误判超时
							 iterComm->second.nTickCount = GetTickCountEX();

							 // 继续发送后续帧
							 unsigned int nSeq = 0;
							 m_tcpCommClient.SendContent(0xC1, iterComm->second.szSendData,
								 iterComm->second.nSendDataLen, nSeq);
#ifdef WIN32
							 ReleaseSRWLockShared(&m_srwCommTask);
							 AcquireSRWLockExclusive(&m_srwCommTask);
#endif
							 // 保存至本地map中等待后续处理						 
							 m_mapCommTask.insert(mapTaskPair(nSeq, iterComm->second));

							 // 删除原来的
							 m_mapCommTask.erase(iterComm);
#ifdef WIN32
							 ReleaseSRWLockExclusive(&m_srwCommTask);
#endif
						 }
					 }
					 else
					 {
						 // 解码失败
					 }
				 }
			 }
			 else
			 {
				 // 非法数据包
			 }
#ifdef WIN32
			 ReleaseSRWLockShared(&m_srwCommTask);
#endif
		 }
	 }
 }

 // 随抄处理函数
 void DCService::ProcessCommonTaskSendThreadFunc()
 {
	 while (!m_bQuitCommonTaskSendThread)
	 {
		 // 获取随抄任务
		 vector<TASKPACKET> vecTaskData;
		 m_taskManage.GetCommonTaskContext(vecTaskData);

		 for (unsigned int i = 0; i < vecTaskData.size(); i++)
		 {
			 unsigned char cProtocalType = vecTaskData[i].cProtocalType;
			 if (m_mapProtocalOBJ.find(cProtocalType) != m_mapProtocalOBJ.end())
			 {
				 // 如果协议处理存在，则开始编码、发送的工作

				 // 判断设备是否在线，设备是否在使用中
				 //if (m_deviceManage.CheckDeviceStatus(vecTaskData[i].taskPacket.szTerminalAddress) == 0)
				 {
					 // 编码成功
					 if (m_mapProtocalOBJ[cProtocalType]->EncodePacket(&vecTaskData[i]))
					 {
						 // 发送编码数据, nSeq网络发送表示
						 unsigned int nSeq = 0;
						 m_tcpCommClient.SendContent(0xC1, vecTaskData[i].szSendData,
							 vecTaskData[i].nSendDataLen, nSeq);

						 // 打时间戳
						 vecTaskData[i].nTickCount = GetTickCountEX();

						 // 更新任务状态
						 vecTaskData[i].cStatus = 2;

						 // 更新设备状态
						 m_deviceManage.UpdateDeviceStatus(vecTaskData[i].szTerminalAddress, 2);

						 // 保存至本地map中等待后续处理	
#ifdef WIN32
						 AcquireSRWLockExclusive(&m_srwCommTask);
#endif
						 m_mapCommTask.insert(mapTaskPair(nSeq, vecTaskData[i]));
#ifdef WIN32
						 ReleaseSRWLockExclusive(&m_srwCommTask);
#endif
					 }
				 }
			 }
		 }
		 this_thread::sleep_for(chrono::microseconds(1));
	 }
 }

 void DCService::ProcessCommonTaskStatusThreadFunc()
 {
	 while (!m_bQuitCommonTaskStatusThread)
	 {
#ifdef WIN32
		 AcquireSRWLockShared(&m_srwCommTask);
#endif
		 typedef map<unsigned int, TASKPACKET>::iterator mapIter;
		 for (mapIter iter = m_mapCommTask.begin(); iter != m_mapCommTask.end(); iter++)
		 {
			 // 是否超时
			 if (iter->second.nTaskStatus == 2)
			 {
				 if ((GetTickCountEX() - iter->second.nTickCount) > (m_nTaskTimeout * 1000))
				 {
					 iter->second.nTaskSuccess = 0;
					 iter->second.nTaskStatus = 1;
					 
					 struct tm curTime = { 0 };
					 Getlocaltime(curTime);

					 sprintf_s(iter->second.szCompleteTime, "%d/%d/%d %d:%d:%d", curTime.tm_year,
						 curTime.tm_mon, curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec);
					 m_taskManage.UpdateCommonTask(iter->second);
				 }
			 }
			 else if (iter->second.nTaskStatus == 3)
			 {
				 // 解码成功，更新数据库
				 iter->second.nTaskSuccess = 0;
				 iter->second.nTaskStatus = 1;

				 struct tm curTime = { 0 };
				 Getlocaltime(curTime);
				 sprintf_s(iter->second.szCompleteTime, "%d/%d/%d %d:%d:%d", curTime.tm_year,
					 curTime.tm_mon, curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec);
				 m_taskManage.UpdateCommonTask(iter->second);
			 }
		 }
#ifdef WIN32
		 ReleaseSRWLockShared(&m_srwCommTask);
#endif
		 // 是否处理成功
		 this_thread::sleep_for(chrono::microseconds(100));
	 }
 }

 void DCService::InsertMonthAndDayTask(UInt32 nYear, UInt32 nMonth, UInt32 nDay, UInt32 nMonthBackword, UInt32 nDayBackword)
 {
	 //（从数据库获取）AFN + FN, PRODUCENAME, TABLENAME, TYPE, TABLEPROPERTY
	 typedef map<string, tuple<string, string, string, string, string, map<UInt8, string>>>::iterator mapIter;
	 for (mapIter iter = m_mapSpecialTaskDefine.begin(); iter != m_mapSpecialTaskDefine.end(); iter++)
	 {
		 string strSNCommand = iter->first;
		 string strType = get<4>(iter->second);

		 if (strType == "month")
		 {
			 m_taskManage.InertTaskMonth(strSNCommand, nYear, nMonth, nMonthBackword);
		 }
		 else if (strType == "day")
		 {
			 m_taskManage.InertTaskDay(strSNCommand, nYear, nMonth, nDay, nDayBackword);
		 }
	 }
 }

 void DCService::ProcessSpecialTaskSendThreadFunc()
 {
	 static bool bRun = false;
	 while (!m_bQuitSpecialTaskSendThread)
	 {
		 if (!bRun)
		 {
			 // 系统启动时产生补抄任务
			 bRun = true;
			 InsertMonthAndDayTask(m_startTime.tm_year, m_startTime.tm_mon, 
				 m_startTime.tm_mday, m_nMonthBackword, m_nDaysBackword);		 
		 }

		 if (IsTimeRange())
		 {
			 // 判断是否符合产生定抄任务的条件
			 if (IsNewMonth())
			 {
				 // 插入月任务 RT_TASK_MONTH(每个月做一次)
				 InsertMonthAndDayTask(m_startTime.tm_year, m_startTime.tm_mon,
					 m_startTime.tm_mday, 0, 0);

				 // 是时候处理定抄任务了(月计划)
				 ProcessScheduleTask("month");
			 }

			 if (IsNewDay())
			 {
				 // 插入日任务 RT_TASK_DAY(每天做一次)
				 InsertMonthAndDayTask(m_startTime.tm_year, m_startTime.tm_mon,
					 m_startTime.tm_mday, 0, 0);

				 // 是时候处理定抄任务了（日计划）
				 ProcessScheduleTask("day");
			 }
		 }

		 this_thread::sleep_for(chrono::microseconds(500));
	 }
 }

void DCService::ProcessSpecialTaskRecvThreadFunc()
{
	while (!m_bQuitSpecialTaskRecvThread)
	{
		while (!m_bQuitCommonTaskRecvThread)
		{
			UInt8 cCommand;
			char szRecvBuf[1500] = { 0 };
			unsigned int nRecvLen = 0;
			unsigned int nRecvID = 0;

			// 获取前置返回的定抄数据
			if (m_tcpSpeciaClient.RecvContent(cCommand, szRecvBuf, nRecvLen, nRecvID) > 0)
			{
#ifdef WIN32
				AcquireSRWLockShared(&m_srwSpeciaTask);
#endif
				map<unsigned int, TASKPACKET>::iterator iterSpecial = m_mapSpecialTask.find(nRecvID);
				if (cCommand == 0xC0)
				{
					// 随抄心跳包
				}
				else if (cCommand == 0xC1 && iterSpecial != m_mapSpecialTask.end())
				{
					// 随抄任务包
					unsigned char cProtocalType = iterSpecial->second.cProtocalType;
					map<int, IProtocalObject *>::iterator iterProtocal = m_mapProtocalOBJ.find(cProtocalType);
					if (iterProtocal != m_mapProtocalOBJ.end())
					{
						// 解码成功
						if (iterProtocal->second->DecodePacket(szRecvBuf, nRecvLen, &iterSpecial->second))
						{
							// 解码后还有连续帧
							if (iterSpecial->second.nTaskStatus == 2)
							{
								// 更新时间戳，防止误判超时
								iterSpecial->second.nTickCount = GetTickCountEX();

								// 继续发送后续帧
								unsigned int nSeq = 0;
								m_tcpCommClient.SendContent(0xC1, iterSpecial->second.szSendData,
									iterSpecial->second.nSendDataLen, nSeq);
#ifdef WIN32
								ReleaseSRWLockShared(&m_srwSpeciaTask);
								AcquireSRWLockExclusive(&m_srwSpeciaTask);
#endif
								// 保存至本地map中等待后续处理						 
								m_mapSpecialTask.insert(mapTaskPair(nSeq, iterSpecial->second));

								// 删除原来的
								m_mapSpecialTask.erase(iterSpecial);
#ifdef WIN32
								ReleaseSRWLockExclusive(&m_srwSpeciaTask);
#endif
							}
						}
						else
						{
							// 解码失败
						}
					}
				}
				else
				{
					// 非法数据包
				}
#ifdef WIN32
				ReleaseSRWLockShared(&m_srwSpeciaTask);
#endif
			}
		}
	}
}

void DCService::ProcessSpecialTaskStatusThreadFunc()
{
	while (!m_bQuitSpecialTaskStatusThread)
	{
#ifdef WIN32
		AcquireSRWLockShared(&m_srwSpeciaTask);
#endif
		typedef map<unsigned int, TASKPACKET>::iterator mapIter;
		for (mapIter iter = m_mapSpecialTask.begin(); iter != m_mapSpecialTask.end(); iter++)
		{
			// 是否超时
			if (iter->second.nTaskStatus == 2)
			{
				if ((GetTickCountEX() - iter->second.nTickCount) > (m_nTaskTimeout * 1000))
				{
					iter->second.nTaskSuccess = 0;
					iter->second.nTaskStatus = 1;

					struct tm curTime = { 0 };
					Getlocaltime(curTime);
					sprintf_s(iter->second.szCompleteTime, "%d/%d/%d %d:%d:%d", curTime.tm_year,
						curTime.tm_mon, curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec);
					m_taskManage.UpdateScheduleTask(iter->second);
				}
			}
			else if (iter->second.nTaskStatus == 3)
			{
				// 解码成功，更新数据库
				iter->second.nTaskSuccess = 0;
				iter->second.nTaskStatus = 1;

				struct tm curTime = { 0 };
				Getlocaltime(curTime);

				sprintf_s(iter->second.szCompleteTime, "%d/%d/%d %d:%d:%d", curTime.tm_year,
					curTime.tm_mon, curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec);
				m_taskManage.UpdateScheduleTask(iter->second);
			}
		}
#ifdef WIN32
		ReleaseSRWLockShared(&m_srwSpeciaTask);
#endif
		// 是否处理成功
		this_thread::sleep_for(chrono::microseconds(100));
	}
}

// 定抄任务处理函数
void DCService::ProcessScheduleTask(const string& strScheduleType)
{
	//（从数据库获取）AFN + FN, PRODUCENAME, TABLENAME, TYPE, TABLEPROPERTY
	typedef map<string, tuple<string, string, string, string, string, map<UInt8, string>>>::iterator mapIter;
	for (mapIter iter = m_mapSpecialTaskDefine.begin(); iter != m_mapSpecialTaskDefine.end(); iter++)
	{
		// 一共有m_mapSpecialTaskDefine.size（）这么多定抄策略要执行
		string strSNCommand = iter->first;
		string strType = get<4>(iter->second);
		string strProduceName = get<2>(iter->second);
		string strTableName = get<3>(iter->second);
		map<UInt8, string> mapTablePro = get<5>(iter->second);
		if (strType != strScheduleType) continue;

		// 数据库表一共有多少列
		UInt32 nTableColCnt = mapTablePro.size();
		string strColName;
		map<UInt8, vector<string>> mapInsertData;
		for (UInt32 i = 0; i < nTableColCnt; i++)
		{
			// 组织列名，用“，”分割
			strColName += mapTablePro[i];
			if (i != mapTablePro.size() - 1)
				strColName += ",";
			
			vector<string> vecColData;
			mapInsertData.insert(make_pair(i, vecColData));
		}

		// 通过对应的存储过程生成任务
		if (m_taskManage.GenerateTask(strProduceName, strSNCommand, strType))
		{
			// 获取定抄任务(每次获取的定抄任务等于最大终端接入数)
			vector<TASKPACKET> vecTaskData;
			while (!m_taskManage.GetScheduleTaskContext(vecTaskData)  
				&& !m_bQuitSpecialTaskSendThread)
			{
				// 发送定抄任务（每次发送的数据包应该等于最大终端接入数）
				// 终端每次只能处理一个数据包
				for (UInt32 i = 0; i < vecTaskData.size(); i++)
				{
					// 如果协议处理存在，则开始编码、发送的工作
					unsigned char cProtocalType = vecTaskData[i].cProtocalType;
					if (m_mapProtocalOBJ.find(cProtocalType) != m_mapProtocalOBJ.end())
					{
						// 判断设备是否在线，设备是否在使用中
						//if (m_deviceManage.CheckDeviceStatus(vecTaskData[i].taskPacket.szTerminalAddress) == 0)
						{
							// 编码成功
							if (m_mapProtocalOBJ[cProtocalType]->EncodePacket(&vecTaskData[i]))
							{
								// 发送编码数据, nSeq网络发送标识
								unsigned int nSeq = 0;
								m_tcpCommClient.SendContent(0xC1, vecTaskData[i].szSendData,
									vecTaskData[i].nSendDataLen, nSeq);

								// 打时间戳
								vecTaskData[i].nTickCount = GetTickCountEX();

								// 更新任务状态
								vecTaskData[i].nTaskStatus = 2;

								// 更新设备状态
								m_deviceManage.UpdateDeviceStatus(vecTaskData[i].szTerminalAddress, 2);

								// 保存至本地map中等待后续处理
#ifdef WIN32
								AcquireSRWLockExclusive(&m_srwSpeciaTask);
#endif
								m_mapSpecialTask.insert(mapTaskPair(nSeq, vecTaskData[i]));
#ifdef WIN32
								ReleaseSRWLockExclusive(&m_srwSpeciaTask);
#endif
							}
						}
					}
				}
				vecTaskData.clear();

				// 这里等待五秒，是给表计返回数据预留时间
				// 因为表计不返回，那么表计的状态就处于使用中
				// 那么即便是马上获取任务，马上下发，表计也
				// 无法处理，因为表计都是串行的方式进行数据处理的
				DWORD dwTickCount = GetTickCountEX();
				while (!m_bQuitSpecialTaskSendThread)
				{
					this_thread::sleep_for(chrono::microseconds(100));
					if ((GetTickCountEX()-dwTickCount) > 5000) break;
				}
			}
			// 等待定抄任务完成（完成后进入到下一个定抄数据的处理）
			InsertScheduleTaskData(strTableName, strColName, nTableColCnt, mapInsertData);
		}
	}
}

void DCService::InsertScheduleTaskData(const string& strTableName, const string& strColName, 
			UInt32 nTableColCnt, map<UInt8, vector<string>> mapInsertData)
{

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSpeciaTask);
#endif
	for (UInt32 j = 0; j < m_mapSpecialTask.size(); j++)
	{
		if (m_mapSpecialTask[j].nTaskSuccess == 1)
		{
			if (m_mapSpecialTask[j].vecReturnData.size() > 0)
			{
				for (UInt32 k = 0; k < m_mapSpecialTask[j].vecReturnData.size(); k++)
				{
					mapInsertData[0].push_back(m_mapSpecialTask[j].szSNTask);
					mapInsertData[1].push_back(m_mapSpecialTask[j].szSNTerminal);
					mapInsertData[2].push_back(m_mapSpecialTask[j].szSNMeter);

					UInt32 nColNumTemp = 0;
					string strTemp = m_mapSpecialTask[j].vecReturnData[k];
					char* pContext = nullptr;
					char* pToken = strtok_s((char*)strTemp.c_str(), "@", &pContext);
					while (pToken != nullptr)
					{
						mapInsertData[3 + nColNumTemp].push_back(pToken);
						pToken = strtok_s(nullptr, "@", &pContext);
						nColNumTemp++;
					}
				}
			}
			else
			{
				mapInsertData[0].push_back(m_mapSpecialTask[j].szSNTask);
				mapInsertData[1].push_back(m_mapSpecialTask[j].szSNTerminal);
				mapInsertData[2].push_back(m_mapSpecialTask[j].szSNMeter);

				UInt32 nColNumTemp = 0;
				string strTemp = m_mapSpecialTask[j].szReturnData;
				char* pContext = nullptr;
				char* pToken = strtok_s((char*)strTemp.c_str(), "@", &pContext);
				while (pToken != nullptr)
				{
					mapInsertData[3 + nColNumTemp].push_back(pToken);
					pToken = strtok_s(nullptr, "@", &pContext);
					nColNumTemp++;
				}
			}
		}
	}
#ifdef WIN32
	ReleaseSRWLockShared(&m_srwSpeciaTask);
#endif

	m_taskManage.BatchInsertScheduleData(strTableName, strColName, nTableColCnt, mapInsertData);
}

bool DCService::ConnectServer()
{
	bool bRet1 = false, bRet2 = false, bRet3 = false;
	if (m_tcpCommClient.InitializeWinsockIfNecessary() > 0)
	{
		// 随抄任务网络处理模块初始化
		int nRet = m_tcpCommClient.SetupStreamSocket(m_strLocalIP.c_str());
		if (nRet >= 0) nRet = m_tcpCommClient.connectServer(m_strServerIP.c_str(), m_nServerPort);

		if (nRet >= 0)
		{
			m_tcpCommClient.SetSendBufferTo(65535);
			m_tcpCommClient.SetReceiveBufferTo(65535);
			m_tcpCommClient.MakeSocketBlocking();
		}
		bRet1 = nRet >= 0 ? true : false;

		// 定抄任务网络处理模块初始化
		nRet = m_tcpSpeciaClient.SetupStreamSocket(m_strLocalIP.c_str());
		if (nRet >= 0) nRet = m_tcpSpeciaClient.connectServer(m_strServerIP.c_str(), m_nServerPort);

		if (nRet >= 0)
		{
			m_tcpSpeciaClient.SetSendBufferTo(65535);
			m_tcpSpeciaClient.SetReceiveBufferTo(65535);
			m_tcpSpeciaClient.MakeSocketBlocking();
		}
		bRet2 = nRet >= 0 ? true : false;

		// 主动上报网络处理模块初始化
		nRet = m_tcpReportingClient.SetupStreamSocket(m_strLocalIP.c_str());
		if (nRet >= 0) nRet = m_tcpReportingClient.connectServer(m_strServerIP.c_str(), m_nServerPort);

		if (nRet >= 0)
		{
			m_tcpReportingClient.SetSendBufferTo(65535);
			m_tcpReportingClient.SetReceiveBufferTo(65535);
			m_tcpReportingClient.MakeSocketBlocking();
		}
		// 网络初始化成功
		bRet3 = nRet >= 0 ? true : false;
	}

	return bRet1 & bRet2;
}

bool DCService::IsNewMonth()
{
	struct tm tmNow = { 0 };
	Getlocaltime(tmNow);

	if (m_startTime.tm_mon != (tmNow.tm_mon+1))
	{
		m_startTime.tm_mon = tmNow.tm_mon + 1;
		return true;
	}
	else
	{
		return false;
	}
}

bool DCService::IsNewDay()
{
	struct tm tmNow = { 0 };
	Getlocaltime(tmNow);

	if (m_startTime.tm_mday != tmNow.tm_mday)
	{
		m_startTime.tm_mday = tmNow.tm_mday;
		return true;
	}
	else
	{
		return false;
	}
}

bool DCService::IsTimeRange()
{
	struct tm curTime = { 0 };
	Getlocaltime(curTime);

	if (curTime.tm_hour > m_ScheduleTask_STime.tm_hour &&
		curTime.tm_hour < m_ScheduleTask_ETime.tm_hour)
	{
		return true;
	}
	else if (curTime.tm_hour == m_ScheduleTask_STime.tm_hour &&
		curTime.tm_min > m_ScheduleTask_STime.tm_min )
	{
		return true;
	}
	else if (curTime.tm_hour == m_ScheduleTask_ETime.tm_hour &&
		curTime.tm_min < m_ScheduleTask_ETime.tm_min)
	{
		return true;
	}

	return false;
}
