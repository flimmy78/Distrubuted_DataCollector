#pragma once

#ifdef WIN32
#include <windows.h>
#include <time.h>
#endif

#include <vector>
#include <string>
using namespace std;
/**********************************************************************************************

	TCP封包格式
	-------------------------------------------------------------------------------
	| 4B (HEADER) |  1B(COMMAND)  |  4B (SEQ) | 2B (LENGTH) | nB (DATA) | 1 (END) |
	-------------------------------------------------------------------------------

	@ HEADER = 0xFE,0xFE,0xFE,0xFE

	@ COMMAND = 0xA0 设备管理心跳包		0xA1 设备掉线		0xA2 设备上线		0xA3 非法设备

				0xB0 定抄任务心跳包		0xB1 定抄任务包	

				0xC0 随抄任务心跳包		0xC1 随抄任务包

				0xD0 主动上报心跳包		0xD1 报警数据包

	@ SEQ = 不重复的发包序号，发送与返回的数据包这个字段要一致

	@ LENGTH = DATA数据的实际长度

	@ DATA = 负载数据

	@ END = 0xF0

	----------------------------------------------------------------------------------------------

	1: 设备管理的网传交互过程
			
			采集														前置

	@1		第一次心跳（相当于注册）			--->

	@2											<---			终端设备上下线信息 

	@3		未注册的终端设备					--->

	@4		周期性心跳							--->

	@5											<---			终端设备上下线信息

	@6		心跳失败goto1						--->

	设备数据包内存结构： tagTCPPacket + tagTerminalDevice + tagTerminalDevice(可以负载多个设备信息）

	----------------------------------------------------------------------------------------------

	2：主动上报的网传交互过程

			采集														前置

	@1		第一次心跳（相当于注册）			--->

	@2											<---			主动上报的报警数据

	@3		周期性心跳							--->

	@4		心跳失败goto1						--->

	主动上报的数据为前置透传过来的报警数据，根据不同的协议类型直接解码就行

	----------------------------------------------------------------------------------------------

	3：定抄随抄的网传交互过程（定抄任务，随抄任务交互过程一致）

			采集														前置

	@1		第一次心跳（相当于注册）			--->

	@2		周期性任务数据						--->

	@3											<---				任务处理结果

	@3		周期性心跳							--->

	@4		心跳失败goto1						--->

	任务数据包内存结构： tagTCPPacket + payload

	----------------------------------------------------------------------------------------------

**********************************************************************************************/

#define		TCPPACKETEND			0xF0		//TCP封包结束符号
#define		HEATBEATTIME			10*1000		//每10秒发送一个心跳包		

namespace WSCOMMON
{

// 以下结构体均按4字节的方式对齐
#pragma pack(4)

	typedef union _documnet
	{
		// 2011中燃项目燃气表档案
		typedef  struct  _StdDocument
		{
			char	sequence_number[5];
			char	measure_point_number[3];
			unsigned char speed_and_port[2];
			char	protocal_type[2];
			char	address[9];
			char	wireless_logic_address[5];
			char	access_password[7];
			char	reserved1;
			char	reserved2;
			char	collector_address[7];
			unsigned char user_macro_vicro_type[2];

		}StdDocument, *PStdDocument;


		// 698规约威胜燃气表档案中燃项目
		typedef  struct  MeterDocument698GAS
		{
			unsigned int  sequence_number;
			int		measuring_point_number;
			int		communicate_speed;
			int		communicate_port;
			int		communicate_protocal_type;
			char	communicate_address[17];
			char	communicate_password[13];
			char	wireless_logic_address[9];
			char	reserved1;
			char	reserved2;
			char	collector_address[13];
			int		primary_user_type;
			int		slavery_user_type;

		}TMeterDocument698GAS, *PMeterDocument698GAS;

		// 698规约威胜燃气表档案
		typedef  struct  MeterDocument698GASGS
		{
			unsigned int  sequence_number;
			int		measuring_point_number;
			int		communicate_speed;
			int		communicate_port;
			int		communicate_protocal_type;
			char	communicate_address[17];
			char	communicate_password[13];
			char	wireless_logic_address[9];
			char	reserved1;
			char	reserved2;
			char	collector_address[13];
			int		primary_user_type;
			int		slavery_user_type;

		}TMeterDocument698GASGS, *PMeterDocument698GASGS;


		// 698规约威胜燃气表档案
		typedef  struct  MeterDocument698Elec
		{
			unsigned int  sequence_number;
			int		measuring_point_number;
			int		communicate_speed;
			int		communicate_port;
			int		communicate_protocal_type;
			char	communicate_address[17];
			char	communicate_password[13];
			int		tax_rate_countl;
			int		power_int_bits;
			int		power_float_bits;
			char	collector_address[13];
			int		primary_user_type;
			int		slavery_user_type;

		}TMeterDocument698Elec, *PMeterDocument698Elec;

		// 698规约威胜燃气表档案2012
		typedef  struct  MeterDocument698Remix
		{
			unsigned int  sequence_number;
			int		measuring_point_number;
			int		communicate_speed;
			int		communicate_port;
			int		communicate_protocal_type;
			char	communicate_address[17];
			char	communicate_password[13];
			char	wireless_logic_address[9];
			char	reserved1;
			char	reserved2;
			char	collector_address[13];
			int		primary_user_type;
			int		slavery_user_type;

		}TMeterDocument698Remix, *PMeterDocument698Remix;

		// 2016_DLMS47档案
		typedef  struct  _METER_DOCUMENT_DLMS47_ELEC
		{
			int		m_MeasurPtNum;
			int		m_MeterType;
			char	m_szMeterAddr[25];    // actually 20

		}METER_DOCUMENT_DLMS47_ELEC, *pMETER_DOCUMENT_DLMS47_ELEC;

	}DOCUMENT;

	typedef struct tagTCPPacket
	{
		unsigned char	szHeader[4] = { 0xFE,0xFE,0xFE,0xFE }; // 包头
		unsigned char	cCommand;			// 发包指令
		unsigned int	nSeq;				// 发包序号
		unsigned short	nLength;			// 负载数据长度

	}TCPPACKET, *PTCPPACKET;

	typedef struct tagTASKPacket
	{
		unsigned long nTickCount = 0;			// 用来判断任务是否已经超时

		unsigned char nTaskStatus = 0;			// 任务当前状态 1未执行 2执行中 3执行完成

		unsigned char nTaskSuccess = 0;			// 任务是否成功

		unsigned char nInputParamCnt = 0;		// 输入参数的个数

		char szCompleteTime[64] = { 0 };		// 任务完成的时间

		char	szSNTask[50] = {0};				// 任务序列号
		char	szSNMeter[50] = {0};			// 表计序列号
		char	szSNTerminal[50] = {0};			// 终端序列号

		char	szTerminalAddress[50] = {0};	// 终端标识符（字符串）
		unsigned short	nMeasuring_point = 0;	// 采集点

		unsigned char	cMeterType = 0;			// 表型号（终端下面可能有多种协议表计）
		unsigned char	cProtocalType = 0;		// 终端协议类型

		char	szAFN[24] = {0};				// 功能码大类（字符串）
		char	szFN[24] = {0};					// 功能码小类（字符串）

		unsigned int	nTime = 0;				// 任务处理时长（前置发送到接收之前的时间差）
		unsigned long	nSendDataLen = 0;		// 下行数据长度(如果任务处理失败，长度为0)
		unsigned char	cStatus = 0;			// 编码状态：0初始化，1登录中，2编码中，3登出中，4已完成

		char	szTransparent_command[256] = {0}; //透传命令
		char	szSendData[1024] = {0};			// 如果长度不为0，则为发送给终端的编码数据
		char	szReturnData[1024] = {0};		// 如果长度不为0，则为任务处理结果数据
		char	szParameter[1024] = { 0 };		// 命令附带的参数

		vector<string> vecReturnData;			// 保存多条记录
	}TASKPACKET, *PTASKPACKET;

	// 终端设备数据结构
	typedef struct tagTerminalDevice
	{
		char			szDevice_name[24];	// 终端标识符
		unsigned long	nDevice_IP; // 终端IP地址如果有的话
		unsigned int	nDevice_Port; // 终端端口号如果有的话
		unsigned char	cDevice_Protocal; // 终端协议
		unsigned char	szReserved[3]; // 预留的三个字节对齐

		tagTerminalDevice()
			: nDevice_IP(0)
			, nDevice_Port(0)
			, cDevice_Protocal(0)
		{
			memset(szDevice_name, 0, sizeof(szDevice_name));
			memset(szReserved, 0, sizeof(szReserved));
		}

	}TERMINALDEVICE,PTERMINNALDEVICE;

#pragma pack()

/************************************************************************/
/*                          公共函数区域                                */
/************************************************************************/

	// 返回自系统开机以来的毫秒数（tick） 
	unsigned long GetTickCountEX(void);

	void Getlocaltime(struct tm& curTime);

	void GenRandomString(int length, char* ouput);

}

