#include "stdafx.h"
#include "DeviceManage.h"


CDeviceManage::CDeviceManage()
{
#ifdef WIN32
	InitializeSRWLock(&m_srwDevice);
#endif

	TCPPACKET tcpPack;
	tcpPack.cCommand = 0xA0;
	tcpPack.nLength = 0;
	tcpPack.nSeq = 0;

	memcpy(m_szHeatBeat, &tcpPack, sizeof(TCPPACKET));
	memset(m_szHeatBeat + sizeof(TCPPACKET), 240, 1);
}

CDeviceManage::~CDeviceManage()
{

}

bool CDeviceManage::Init(IDBObject * pDBObj, const string &strDBTag, const string& strLocalIP, 
	const string& strServerIP, UInt32 nServerPort, UInt32 nLogLevel)
{
	if (m_pLog == nullptr)
	{
		m_pLog = ILog4zManager::GetInstance();
		m_loggerId = m_pLog->CreateLogger("deviceManage", LOG4Z_DEFAULT_PATH, nLogLevel);
		m_pLog->Start();
	}

	bool bRet = false;
	if ( pDBObj != nullptr && !strServerIP.empty() &&
		!strLocalIP.empty() && nServerPort > 0 )
	{
		m_pDBObject = pDBObj;
		m_strServerIP = strServerIP;
		m_strLocalIP = strLocalIP;
		m_nServerPort = nServerPort;
		m_strDBTag = strDBTag;

		// 获取注册设备，TCP连接前置机
		if (LoadDeviceFromDB() && ConnectServer())
		{
			// 都成功的情况下创建一个线程收发网络数据 
			m_netRecvThread = std::thread(RecvThreadFunc, this);
			if (m_netRecvThread.joinable()) 
				m_netRecvThread.detach();

			m_netSendThread = std::thread(SendThreadFunc, this);
			if (m_netSendThread.joinable()) 
				m_netSendThread.detach();

			bRet = true;
			LOG_DEBUG(m_loggerId, "CDeviceManage::Init() succeed");
		}	
	}
	else
	{
		LOG_ERROR(m_loggerId, "CDeviceManage::Init() failed, invalid para");
	}
	return bRet;
}

bool CDeviceManage::UnInit()
{
	bool bRet = false;
	m_bQuitRecvThread = true;
	m_bQuitSendThread = true;
	m_tcpClient.CloseSocket(true);
	m_mapDeviceInfo.clear();
	
	return bRet;
}

Int32 CDeviceManage::CheckDeviceStatus(const string & strDeviceName)
{
#ifdef WIN32
	AcquireSRWLockShared(&m_srwDevice);
#endif
	typedef map<string, TERMINALDEVICEDB>::iterator mapIter;
	mapIter iter = m_mapDeviceInfo.find(strDeviceName);

	if (iter != m_mapDeviceInfo.end())
	{
#ifdef WIN32
		ReleaseSRWLockShared(&m_srwDevice);
#endif
		return iter->second.nDeviceStatus;
	}
	
#ifdef WIN32
	ReleaseSRWLockShared(&m_srwDevice);
#endif
	return Int32(-1);
}

Int32 CDeviceManage::UpdateDeviceStatus(const string & strDeviceName, Int32 nDeviceStatus)
{
#ifdef WIN32
	AcquireSRWLockExclusive(&m_srwDevice);
#endif
	typedef map<string, TERMINALDEVICEDB>::iterator mapIter;
	mapIter iter = m_mapDeviceInfo.find(strDeviceName);

	if (iter != m_mapDeviceInfo.end())
	{
		if (nDeviceStatus == 0xA1)
		{
			// 设备下线（前置机通知）
			iter->second.nDeviceStatus = -1;
		}
		else if (nDeviceStatus == 0xA2)
		{
			// 设备上线（前置机通知）
			iter->second.nDeviceStatus = 0;
		}
		else
		{
			// 设置设备状态值
			iter->second.nDeviceStatus = nDeviceStatus;
		}
#ifdef WIN32
		ReleaseSRWLockExclusive(&m_srwDevice);
#endif
		return Int32(nDeviceStatus);
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwDevice);
#endif
	return Int32(-1);
}

bool CDeviceManage::LoadDeviceFromDB()
{
	string strErr = "";
	if (m_pDBObject != nullptr)
	{

		UInt32 nRowNum = 0;
		if (m_strDBConTag.empty())
		{
			if (!m_pDBObject->BeginBatchRead(m_strDBTag, BATCHREADSQL, 3, m_strDBConTag))
				return false;

			m_pDBObject->SetReadColData(m_vecout1, m_strDBTag, m_strDBConTag);
			m_pDBObject->SetReadColData(m_vecout2, m_strDBTag, m_strDBConTag);
			m_pDBObject->SetReadColData(m_vecout3, m_strDBTag, m_strDBConTag);
		}

		m_vecout1.clear(); m_vecout2.clear(); m_vecout3.clear();
		m_pDBObject->ExcuteBatchRead(m_strDBTag, m_strDBConTag, strErr);
		for (UInt32 i = 0; i < m_vecout1.size(); i++)
		{
#ifdef WIN32
			AcquireSRWLockExclusive(&m_srwDevice);
#endif
			TERMINALDEVICEDB deviceInfo;
			deviceInfo.strDevice_name = m_vecout1[i];
			deviceInfo.strDevice_sn = m_vecout3[i];
			deviceInfo.cDevice_Protocal = atoi(m_vecout2[i].c_str());
			deviceInfo.nDeviceStatus = 0;//-1;
			typedef map<string, TERMINALDEVICEDB>::value_type devicedata;
			m_mapDeviceInfo.insert(devicedata(deviceInfo.strDevice_name, deviceInfo));
#ifdef WIN32
			ReleaseSRWLockExclusive(&m_srwDevice);
#endif
		}

		return true;
	}

	LOGFMT_ERROR(m_loggerId, "CDeviceManage::LoadDeviceFromDB failed, %s", strErr.c_str());

	return false;
}

int CDeviceManage::SendHeartBeat()
{
	unsigned int nSeq = 0;
	int nRet = m_tcpClient.SendContent(0xA0, nullptr, 0, nSeq);
	return nRet;
}

bool CDeviceManage::ConnectServer()
{
	bool bRet = false;
	if (m_tcpClient.InitializeWinsockIfNecessary() > 0)
	{
		int nRet = m_tcpClient.SetupStreamSocket(m_strLocalIP.c_str());
		if (nRet >= 0) nRet = m_tcpClient.connectServer(m_strServerIP.c_str(), m_nServerPort);

		if (nRet >= 0)
		{
			m_tcpClient.SetSendBufferTo(65535);
			m_tcpClient.SetReceiveBufferTo(65535);
			m_tcpClient.MakeSocketBlocking();
		}
		// 网络初始化成功
		bRet = nRet >= 0 ? true : false;
	}

	if (!bRet) LOG_ERROR(m_loggerId, "CDeviceManage::ConnectServer() failed");

	return bRet;
}

bool CDeviceManage::ReConnectServer()
{
	bool bRet = false;
	m_tcpClient.CloseSocket(true);
	bRet = ConnectServer();
	return bRet;
}

void CDeviceManage::RecvThreadFunc(void * pParam)
{
	CDeviceManage* pDevice = (CDeviceManage*)pParam;
	pDevice->ProcessRecvThreadFunc();
}

void CDeviceManage::SendThreadFunc(void * pParam)
{
	CDeviceManage* pDevice = (CDeviceManage*)pParam;
	pDevice->ProcessSendThreadFunc();
}

void CDeviceManage::ProcessRecvThreadFunc()
{
	while (!m_bQuitRecvThread)
	{
		// 循环接收网络数据
		if (ReceiveFromServer() < 0)
		{
			// 数据接收失败则重新连接
			ReConnectServer();
		}
	}
}

void CDeviceManage::ProcessSendThreadFunc()
{
	while (!m_bQuitSendThread)
	{
		// 循环发送网络数据
		if ((GetTickCountEX() - m_nTickCount) > HEATBEATTIME)
		{
			if (SendHeartBeat() < 0)
			{
				// 心跳发送失败则重新连接
				//ReConnectServer();
				//continue;
			}
			m_nTickCount = GetTickCountEX();
		}
		else
		{
			this_thread::sleep_for(chrono::microseconds(1));
		}
	}
}

int CDeviceManage::ReceiveFromServer()
{
	int nRet = -1;
	unsigned char cCommand = 0x0;
	unsigned int nSeq = 0;
	unsigned int nRecvlen = 0;
		
	// 根据TCP封包结构，读出设备信息
	// 这里可能有多个设备信息（和前置约定最多20个信息）
	nRet = m_tcpClient.RecvContent(cCommand, (char*)&m_sDeviceInfo, nRecvlen, nSeq);
	if (nRet > 0)
	{
		/*static DWORD dwTick = GetTickCountEX();
		static DWORD nPacketCnt = 1;
		DWORD dwPercent = nPacketCnt / ((GetTickCountEX()+1000 - dwTick)/1000);
		nPacketCnt++;
		char szDebug[64] = { 0 };
		sprintf_s(szDebug, "%d packets per sencond \r\n", dwPercent);
		OutputDebugString(szDebug);*/

		if (cCommand == 0XA0)
		{
			int a = 1;
		}
		else if (cCommand == 0XA1 || 
			cCommand == 0XA2)
		{
			// 设备上下线（如果是批量上下线，则需要批量属性相同才行）
			for (UInt32 i = 0; i < nRecvlen / sizeof(TERMINALDEVICE); i++)
			{
				// 同步设备状态信息
				UpdateDeviceStatus(m_sDeviceInfo[i].szDevice_name, cCommand);
			}
		}
	}

	return nRet;
}

