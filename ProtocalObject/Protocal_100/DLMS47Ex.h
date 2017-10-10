#pragma once


#include <string>
#include <vector>
#include <map>
using namespace std;

#include "Markup.h"

class CDLMS47Ex
{
public:
	CDLMS47Ex();
	~CDLMS47Ex();

	bool EncodePacket(void* pTask, void* pDB);
	bool DecodePacket(const char* pRecvData, UInt32 nDataLen, void* pTask);

private:
	int BuildLink(void* pTask);
	int BuildRelease(void* pTask);
	int Fill47ExData(void* pTask, void* pDB);

	int Fill47ExDocument(void* pTask, void* pDB);
	int Fill47ExTable(void* pTask, void* pDB);

	UINT GetNewInvokeID(UINT nStep = 1)
	{
		if (nStep > 16) { nStep = 1; }
		m_nInvokeID += nStep;
		if (m_nInvokeID >= 16)
		{
			m_nInvokeID = 1;
		}
		return m_nInvokeID;
	}

private:
	string m_strLinkXml;
	string m_strGetMtrXml;
	string m_strSetMtrXml;
	string m_strLoadDocXml;
	string m_strGetDocXml;
	string m_strGetDataTbl;
	string m_strSetDataTbl;

	vector<string> m_vecParamter;
	ADDR m_Adr; // 47协议地址
	char m_szIDStr[ID_LEN]; // ID标识
	int m_nIsReadOrSet = -1; // 设置或读取的标识
	int m_nCmdTpye = -1; // 命令类型 1 下档案 2 普通数据读取 3 下载table
	int m_nInvokeID = 1;
};


// to do
// 1:确定返回值的XML串，登录返回，登出返回，读取返回，设置返回的值
// 2:档案读取和加载的实现，需要读取数据库，获取表计档案，1p，3p
// 3:核对三相表的设置命令DPDC项目，而非REB项目
// 4:日志的添加
