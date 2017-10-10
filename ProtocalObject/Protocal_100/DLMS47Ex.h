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
	ADDR m_Adr; // 47Э���ַ
	char m_szIDStr[ID_LEN]; // ID��ʶ
	int m_nIsReadOrSet = -1; // ���û��ȡ�ı�ʶ
	int m_nCmdTpye = -1; // �������� 1 �µ��� 2 ��ͨ���ݶ�ȡ 3 ����table
	int m_nInvokeID = 1;
};


// to do
// 1:ȷ������ֵ��XML������¼���أ��ǳ����أ���ȡ���أ����÷��ص�ֵ
// 2:������ȡ�ͼ��ص�ʵ�֣���Ҫ��ȡ���ݿ⣬��ȡ��Ƶ�����1p��3p
// 3:�˶���������������DPDC��Ŀ������REB��Ŀ
// 4:��־�����
