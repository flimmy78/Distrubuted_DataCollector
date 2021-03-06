#pragma once

/************************************************************************
	@ 创建时间：2017.7.10

	@ 设计说明：
	
		***本对象只创建一个线程，处理网络数据收发***

		***函数支持多线程安全***

		1：获取注册设备列表。

		2：判断设备上下线（设备上下线由前置发送数据包来判断）。

		3：获取设备状态（空闲中，正在使用中，每个设备只能收发一个命令）。

		4：黑白名单设置（是否告知前置哪些设备可连接，哪些不被允许？）。

		5：与前置保持链接，心跳保活。
		
		---------------------------------------------------------------
		数据库相关表：T_CODE_CST_TYPE		T_RUN_CST 
		---------------------------------------------------------------
		需要的字段		CST_ADDR			SN_PROTOCAL				
		---------------------------------------------------------------
	@ 修改记录：

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
	// 终端设备数据结构
	typedef struct tagTerminalDeviceDB
	{
		string			strDevice_sn		= "";	// 设备的唯一编码
		string			strDevice_name		= "";	// 终端标识符
		struct tm		sDevice_OnTime		= {0};	// 设备上线的时间		
		struct tm		sDevice_OffTime		= {0};	// 设备下线的时间		
		unsigned long	nDevice_IP			= 0;	// 终端IP地址如果有的话
		unsigned int	nDevice_Port		= 0;	// 终端端口号如果有的话
		unsigned char	cDevice_Protocal	= 0;	// 终端协议
		int				nDeviceStatus       = 0;	// 终端在线状态

	}TERMINALDEVICEDB, PTERMINNALDEVICEDB;

public:
	CDeviceManage();
	~CDeviceManage();

	/************************************************************************
		@ 初始化做如下几件工作

		1: 保存数据库对象 。2：从数据库导入注册设备。

		3：与前置建立TCP链接。4：加载设备黑白名单。

		5：创建线程接收网络数据（设备上下线）

		@ pDBObj : 数据库对象

		@ strIP : 前置对应的IP地址

		@ nPort : 前置对应的端口 
	************************************************************************/
	bool Init(IDBObject* pDBObj, const string &strDBTag, const string& strLocalIP, 
		const string& strServerIP, UInt32 nServerPort, UInt32 nLogLevel = 0);

	/*************************************************************************
		@ 反初始化做如下几件工作

		1: 保存数据库对象 。

		2：清空本地注册设备的数据。

		3：关闭TCP链接。

		4：清空加载设备黑白名单。
	*************************************************************************/
	bool UnInit();

	/*************************************************************************
		@ 检查当前设备的状态，如下几个返回值：

		-1：设备掉线不可用

		0：设备可以正常使用

		1：设备未注册

		2：设备正在使用中
	*************************************************************************/
	Int32 CheckDeviceStatus(const string& strDeviceName);

	Int32 UpdateDeviceStatus(const string& strDeviceName, Int32 nDeviceStatus);

private:

	// 从数据库获取注册设备(包括黑白名单)
	bool LoadDeviceFromDB();

	// 发送心跳包
	int  SendHeartBeat();		

	// TCP连接服务器
	bool ConnectServer();		

	// 重新连接服务器
	bool ReConnectServer();		

	// 网络数据收发线程函数
	static void RecvThreadFunc(void * pParam);
	static void SendThreadFunc(void * pParam);

	// 处理线程函数
	void ProcessRecvThreadFunc();
	void ProcessSendThreadFunc();

	// 从前置接收数据
	int	ReceiveFromServer();

private:
	// 数据库操作对象
	IDBObject*						m_pDBObject						= nullptr;
	// 包序号，自然数
	UInt32							m_nPackIdx						= 0;
	// 前置机对应的监听IP地址
	string							m_strServerIP					= "";
	// 前置机对应的监听PORT端口
	UInt32							m_nServerPort					= 0;
	// 本地IP地址
	string							m_strLocalIP					= "";
	// 时间TICK，用来给心跳计时
	UInt32							m_nTickCount					= 0;
	// 是否同步数据(一般情况下是不需要同步的)
	bool							m_bSyncTCPHeader				= false;
	// 如果TCP数据混乱，用于TCP同步的缓冲区
	char							m_szSyncBuf[SYNCPACKETLEN]		= { 0 };
	// 如果TCP数据混乱，用于TCP同步的缓冲区
	UInt32							m_nReadIdx						= 0;
	// 心跳包，不同的业务有不同的心跳包
	unsigned char m_szHeatBeat[32]									= { 0 };
	// 数据库的标识符
	string							m_strDBTag						= "";
	string							m_strDBConTag					= "";

	// 退出收发网络数据线程的标识
	bool							m_bQuitSendThread				= false;
	bool							m_bQuitRecvThread				= false;

	// 日志
	ILog4zManager					*m_pLog							= nullptr;
	LoggerId						m_loggerId						=0;

	// 记录注册设备的信息/*name*/
	map<string, TERMINALDEVICEDB>	m_mapDeviceInfo;
	// 创建一个线程接收网络数据
	std::thread						m_netRecvThread;
	// 创建一个线程发送网络数据
	std::thread						m_netSendThread;
	// 前置同步过来的设备信息
	TERMINALDEVICE					m_sDeviceInfo[20];
	// TCP封包结构信息
	TCPPACKET						m_sTCPHeader;
	// 网络数据处理
	CClientTCP						m_tcpClient;
	// 保存数据库字段device name
	vector<string>					m_vecout1;
	// 保存数据库字段device proto
	vector<string>					m_vecout2;
	// 保存数据库字段device SN
	vector<string>					m_vecout3;

#ifdef WIN32
	// 读写锁
	SRWLOCK	 m_srwDevice;
#endif
};