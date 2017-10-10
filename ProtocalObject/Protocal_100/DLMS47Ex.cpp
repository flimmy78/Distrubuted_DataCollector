#include "stdafx.h"
#include "DLMS47Ex.h"

CDLMS47Ex::CDLMS47Ex()
{
	m_strLinkXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\
<COSEM_OPEN_REQ>\
<ACSE_Protocol_Version Value=\"80\"/>\
<Application_Context_Name Value=\"1\"/>\
<Sender_Acse_Requirement Value=\"80\"/>\
<Security_Mechanism_Name Value=\"1\"/>\
<Calling_Authentication_Value Value=\"33333333\"/>\
<DLMS_Version_Number Value=\"6\"/>\
<DLMS_Conformance Value=\"007E1F\"/>\
<Client_Max_Receive_PDU_Size Value=\"512\"/>\
<Service_Class Value=\"1\"/>\
</COSEM_OPEN_REQ>";

	m_strGetMtrXml ="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<GET_REQ>\
<Invoke_Id Value=\"0\"/>\
<Service_Class Value=\"1\"/>\
<Request_Type Value=\"1\"/>\
<Priority Value=\"0\"/>\
<COSEM_Attribute_Descriptor>\
<COSEM_Class_Id Value=\"26\"/>\
<COSEM_Instance_Id Value=\"0000411001FF\"/>\
<COSEM_Attribute_Id Value=\"4\"/>\
<Selective_Access_Parameters>\
<access_selector Value=\"2\"/>\
<access_parameters>\
<index_access Type=\"structure\">\
<Pro_Index Type=\"array\" value=\"1\">\
<L0 Type=\"long-unsigned\" Value=\"1\"/>\
</Pro_Index>\
<index_count Type=\"long-unsigned\" Value=\"0\"/>\
</index_access>\
</access_parameters>\
</Selective_Access_Parameters>\
</COSEM_Attribute_Descriptor>\
</GET_REQ>";

	m_strSetMtrXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<SET_REQ>\
<Invoke_Id Value=\"0\"/>\
<Service_Class Value=\"1\"/>\
<Request_Type Value=\"1\"/>\
<Priority Value=\"0\"/>\
<COSEM_Attribute_Descriptor>\
<COSEM_Class_Id Value=\"26\"/>\
<COSEM_Instance_Id Value=\"0000411001FF\"/>\
<COSEM_Attribute_Id Value=\"4\"/>\
<Selective_Access_Parameters>\
<access_selector Value=\"2\"/>\
<access_parameters>\
<index_access Type=\"structure\">\
<Pro_Index Type=\"array\" value=\"1\">\
<L0 Type=\"long-unsigned\" Value=\"1\"/>\
</Pro_Index>\
<index_count Type=\"long-unsigned\" Value=\"0\"/>\
</index_access>\
</access_parameters>\
</Selective_Access_Parameters>\
</COSEM_Attribute_Descriptor>\
<Value>\
<Name Type=\"octet-string\" Value=\"\"/>\
</Value>\
</SET_REQ>";

	m_strLoadDocXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<SET_REQ>\
<Invoke_Id Value=\"0\"/>\
<Service_Class Value=\"1\"/>\
<Request_Type Value=\"1\"/>\
<Priority Value=\"0\"/>\
<COSEM_Attribute_Descriptor>\
<COSEM_Class_Id Value=\"17\"/>\
<COSEM_Instance_Id Value=\"0000290000FF\"/>\
<COSEM_Attribute_Id Value=\"2\"/>\
</COSEM_Attribute_Descriptor>\
<Value>\
<L0 Type=\"array\" Value=\"0\"/>\
</Value>\
</SET_REQ>";

	m_strGetDocXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\
<GET_REQ>\
<Invoke_Id Value=\"0\"/>\
<Service_Class Value=\"1\"/>\
<Request_Type Value=\"1\"/>\
<Priority Value=\"0\"/>\
<COSEM_Attribute_Descriptor>\
<COSEM_Class_Id Value=\"17\"/>\
<COSEM_Instance_Id Value=\"0000290000FF\"/>\
<COSEM_Attribute_Id Value=\"2\"/>\
</COSEM_Attribute_Descriptor>\
</GET_REQ>";

	m_strGetDataTbl = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\
<GET_REQ>\
<Invoke_Id Value=\"0\"/>\
<Service_Class Value=\"1\"/>\
<Request_Type Value=\"1\"/>\
<Priority Value=\"0\"/>\
<COSEM_Attribute_Descriptor>\
<COSEM_Class_Id Value=\"26\"/>\
<COSEM_Instance_Id Value=\"0000411000FF\"/>\
<COSEM_Attribute_Id Value=\"4\"/>\
</COSEM_Attribute_Descriptor>\
</GET_REQ>";

	m_strSetDataTbl = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<SET_REQ>\
<Invoke_Id Value=\"0\"/>\
<Service_Class Value=\"1\"/>\
<Request_Type Value=\"1\"/>\
<Priority Value=\"0\"/>\
<COSEM_Attribute_Descriptor>\
<COSEM_Class_Id Value=\"26\"/>\
<COSEM_Instance_Id Value=\"0000411000FF\"/>\
<COSEM_Attribute_Id Value=\"4\"/>\
</COSEM_Attribute_Descriptor>\
<Value>\
<Name Type=\"octet-string\" Value=\"\"/>\
</Value>\
</SET_REQ>";

}

CDLMS47Ex::~CDLMS47Ex()
{
}

bool CDLMS47Ex::EncodePacket(void* pTask, void* pDB)
{
	bool bRet = false;
	int nRet = -1;
	WSCOMMON::PTASKPACKET pPacket = (WSCOMMON::PTASKPACKET)pTask;
	switch (pPacket->cStatus)
	{
	case 0:
	case 1:
		nRet = BuildLink(pTask);
		break;
	case 2:
		nRet = Fill47ExData(pTask, pDB);
		break;
	case 3:
		nRet = BuildRelease(pTask);
		break;
	default:
		break;
	}
	return bRet = nRet==0? true:false;
}

bool CDLMS47Ex::DecodePacket(const char* pRecvData, UInt32 nDataLen, void* pTask)
{
	WSCOMMON::PTASKPACKET pPacket = (WSCOMMON::PTASKPACKET)pTask;

	CMarkup xml;
	unsigned char* pOutData = NULL;
	unsigned int nOutDataLen = 0;
	int nResult = ProcessFrame(&pOutData, nOutDataLen, UDPIP, 
		(unsigned char*)m_szIDStr, (const unsigned char*)pRecvData, nDataLen);

	string strFrame;

	if (0 != nResult)
	{
		goto ERRRET;
	}

	bool bRet = xml.SetDoc((const char*)pOutData);
	if (!bRet != 0)
	{
		goto ERRRET;
	}

	xml.IntoElem();
	xml.FindElem("Root");
	xml.IntoElem();
	bRet = xml.FindElem("Frame");
	if (bRet)
	{
		strFrame = xml.GetAttrib("Value");

		if (strFrame.length() > 2048)
		{
			goto ERRRET;
		}

		HexStrToByteArray(strFrame.c_str(), 
			(unsigned char*)pPacket->szReturnData, 
			(unsigned long)pPacket->nSendDataLen);

		if (0 == pPacket->nSendDataLen)
		{
			goto ERRRET;
		}
		goto MULTIFRAM;
	}
	else
	{
		xml.IntoElem(); xml.IntoElem();
		xml.IntoElem();
		bRet = xml.FindElem("Result");
		if (!bRet)
		{
			goto ERRRET;
		}
		else
		{
			if (2 == m_nIsReadOrSet) // set
			{
				xml.IntoElem();
				xml.FindElem();
				string strResult = xml.GetAttrib("Value");
				strcpy_s(pPacket->szReturnData, strResult.c_str());
				if ("0" != strResult)
				{					
					goto ERRRET;
				}
				else
				{
					if (pPacket->cStatus == 3)
					{
						goto RELEASERET;
					}
					else if (pPacket->cStatus == 1)
					{
						goto MULTIFRAM;
					}
					else
					{
						goto SUCCERET;
					}
				}
			}
			else if (1 == m_nIsReadOrSet)  // read
			{
				// TODO...
				//m_DocReg.SaveFile("Get_Cnf.xml");
				//const char* pDataName = pElmt->Value();
				if (0 == strcmp("Data", ""))
				{
					if (2 == m_nCmdTpye)
					{
						//AnsiString strData = pElmt->FirstChildElement()->Attribute("Value");
						//return DecodeData47_nml(strData);
					}
					else if (1 == m_nCmdTpye)
					{
						//return DecodeData47_Doc(pElmt->FirstChildElement());
					}
					else if (3 == m_nCmdTpye)
					{
						//return DecodeData47_DtTbl(pElmt->FirstChildElement());
					}
					else
					{
						//sprintf(pTask->taskError, "error 47 command type:%d", m_nCmdTpye);
						//SendMessage(ghMainWnd, CM_DISPLAY_DATA_MESSAGE, (WPARAM)pTask->taskError, index);
						//return false;
					}

				}
				else
				{
					goto ERRRET;
				}
			}
		}
	}

MULTIFRAM:
	pPacket->nTaskStatus = 2;
	pPacket->cStatus = 2;
	ReleaseMemory(&pOutData);
	return false;

RELEASERET:
	pPacket->nTaskSuccess = 1;
	pPacket->nTaskStatus = 2;
	pPacket->cStatus = 3;
	ReleaseMemory(&pOutData);
	return true;

SUCCERET:
	pPacket->nTaskSuccess = 1;
	pPacket->nTaskStatus = 2;
	pPacket->cStatus = 4;
	ReleaseMemory(&pOutData);
	return true;

ERRRET:
	pPacket->nTaskSuccess = 0;
	pPacket->nTaskStatus = 3;
	pPacket->cStatus = 4;
	ReleaseMemory(&pOutData);
	return false;
}

int CDLMS47Ex::BuildRelease(void* pTask)
{
	WSCOMMON::PTASKPACKET pPacket = (WSCOMMON::PTASKPACKET)pTask;

	unsigned char* pOutData = NULL;
	unsigned int OutDataLen;
	char szXMLRelease[128] = { 0 };
	strcpy(szXMLRelease, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><COSEM_RELEASE_REQ />");
	int nResult = ProcessServicePrimitive(&pOutData, OutDataLen, UDPIP,
		(unsigned char*)m_szIDStr, m_Adr, (unsigned char*)szXMLRelease, strlen(szXMLRelease));

	CMarkup xml;
	bool bRet = xml.SetDoc((const char*)pOutData);
	ReleaseMemory(&pOutData);
	
	if (!bRet)
	{
		PhyAbort(UDPIP, (unsigned char*)m_szIDStr, m_Adr);
		return  -1;
	}

	xml.IntoElem();
	xml.IntoElem();
	bRet = xml.FindElem("Frame");
	string strFrame;
	if (bRet)
	{
		strFrame = xml.GetAttrib("Value");
	}

	unsigned char szByte[1024] = { 0 };
	if (strFrame.length() > 2048)
	{
		PhyAbort(UDPIP, (unsigned char*)m_szIDStr, m_Adr);
		return -1;
	}

	HexStrToByteArray(strFrame.c_str(), 
		(unsigned char*)pPacket->szSendData, 
		pPacket->nSendDataLen);

	if (0 == pPacket->nSendDataLen)
	{
		PhyAbort(UDPIP, (unsigned char*)m_szIDStr, m_Adr);
		return -1;
	}
	return 0;
}

int CDLMS47Ex::BuildLink(void* pTask)
{
	CMarkup xml;
	int b = 0, e = 0;
	unsigned char* pOutData = NULL;
	unsigned int OutDataLen;

	WSCOMMON::PTASKPACKET pPacket = (WSCOMMON::PTASKPACKET)pTask;
	string params = pPacket->szParameter;
	string param = pPacket->szParameter;
	string strFrame;

	while (true)
	{
		e = params.find(',');
		if (e <= 0)
		{
			if (params.length() > 0)
				m_vecParamter.push_back(params);
			break;
		}
		else
		{
			m_vecParamter.push_back(params.substr(b, e - b - 1));
			params = params.substr(e + 1, params.length() - e);
		}
	}

	if (pPacket->nInputParamCnt != m_vecParamter.size())
	{
		// 输入参数与设置的参数个数不符
		goto ERRRET;
	}

	m_Adr.UDPIP_ADDR.Server_wPort = pPacket->nMeasuring_point;
	int nResult = ProcessServicePrimitive(&pOutData, OutDataLen, UDPIP, 
		(unsigned char*)m_szIDStr, m_Adr, (unsigned char*)m_strLinkXml.c_str(), 
		m_strLinkXml.length());

	if (0 != nResult)
	{
		goto ERRRET;
	}


	bool bRet = xml.SetDoc((const char*)pOutData);

	if (!bRet)
	{
		goto ERRRET;
	}

	xml.IntoElem();
	xml.FindElem("Root");
	xml.IntoElem();
	xml.FindElem("Frame");
	strFrame = xml.GetAttrib("Value");

	if (strFrame.length() > 2048)
	{
		goto ERRRET;
	}

	unsigned long nSize = 0;
	HexStrToByteArray(strFrame.c_str(), 
		(unsigned char*)pPacket->szSendData, pPacket->nSendDataLen);

	if (0 == pPacket->nSendDataLen)
	{
		goto ERRRET;
	}

	pPacket->nTaskStatus = 2;
	pPacket->cStatus = 1;
	ReleaseMemory(&pOutData);
	return 0;

ERRRET:
	pPacket->nTaskSuccess = 0;
	pPacket->nTaskStatus = 3;
	pPacket->cStatus = 4;
	ReleaseMemory(&pOutData);
	return -1;
}

int CDLMS47Ex::Fill47ExData(void* pTask, void* pDB)
{
	string strXMLReg;
	WSCOMMON::PTASKPACKET pPacket = (WSCOMMON::PTASKPACKET)pTask;
	if (0 == strncmp("0.0.41.0.0.255", pPacket->szAFN, strlen(pPacket->szAFN)))
	{
		m_nCmdTpye = 1;
		if (!Fill47ExDocument(pTask, pDB))
		{
			return -1;
		}
	}
	else if (0 == strncmp("0.0.65.16.0.255", pPacket->szAFN, strlen(pPacket->szAFN)))
	{
		m_nCmdTpye = 3;
		if (!Fill47ExTable(pTask, pDB))
		{
			return -1;
		}
	}
	else
	{
		m_nCmdTpye = 2;
		if (strlen(pPacket->szTransparent_command) <= 0)
		{
			return -1;
		}

		if ((20 != pPacket->cMeterType) && (21 != pPacket->cMeterType))
		{
			return -1;
		}

		// obtain read-write attribute; true->read false->set
		for (int i = 0; i < 1; i++)
		{
			if (20 == pPacket->cMeterType)     // single phase
			{
				if (0 == strcmp("F002", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F019", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F020", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F021", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("D0E0", pPacket->szTransparent_command)) { m_nIsReadOrSet = 2; break; }
				if (0 == strcmp("F111", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F121", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F131", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F141", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F151", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F161", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C100", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C103", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C111", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C001", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("D700", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("D053", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C030", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C031", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C032", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C033", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C034", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }

				if (0 == strcmp("FF11", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("FF12", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("FF13", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("FF14", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("FF15", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("FF16", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }

				return -1;
			}
			else       //  21 == pTask->m_nMtrTp   // three phase
			{
				if (0 == strcmp("F020", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F610", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F620", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F630", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F640", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F650", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F660", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F019", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F021", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("F100", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("D0E0", pPacket->szTransparent_command)) { m_nIsReadOrSet = 2; break; }

				if (0 == strcmp("C103", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C100", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C104", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C101", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C105", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C102", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C115", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C111", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C116", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C112", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C117", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C113", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C114", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C110", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }

				if (0 == strcmp("C001", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("D700", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C501", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C030", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C031", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C032", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C033", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("C034", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }
				if (0 == strcmp("FFF6", pPacket->szTransparent_command)) { m_nIsReadOrSet = 1; break; }

				return -1;
			}
		}

		char szHexStrOBIS[50] = { 0 };
		if (!OBISToHexStr(pPacket->szAFN, szHexStrOBIS))
		{
			return -1;
		}

		// get xml
		CMarkup prtReg;	
		if (1 == m_nIsReadOrSet)
		{
			prtReg.SetDoc(m_strGetMtrXml);
			prtReg.IntoElem();
			prtReg.FindElem("Invoke_Id");
			prtReg.SetAttrib("Value", GetNewInvokeID());
			prtReg.FindElem("COSEM_Attribute_Descriptor");
			prtReg.IntoElem();
			prtReg.FindElem("COSEM_Instance_Id");
			prtReg.SetAttrib("Value", szHexStrOBIS);
			prtReg.FindElem("Selective_Access_Parameters");
			prtReg.IntoElem();
			prtReg.FindElem("L0");
			prtReg.SetAttrib("Value", pPacket->szFN);

			strXMLReg = prtReg.GetDoc();
		}
		else       // 2 == m_nIsReadOrSet
		{
			char szValue[1050] = { 0 };
			UINT nLen = m_vecParamter.size();
			if (nLen >= 1024 / 2)
			{
				return -1;
			}
			if (0 == nLen)
			{
				return -1;
			}

			szValue[0] = '2';
			szValue[1] = '8';
			StrToHEXStr(m_vecParamter[0].c_str(), &(szValue[2]), (unsigned long&)nLen);
			szValue[nLen + 2] = '2';
			szValue[nLen + 3] = '9';
			szValue[nLen + 4] = 0;

			prtReg.SetDoc(m_strSetMtrXml);
			prtReg.IntoElem();prtReg.FindElem("Invoke_Id");		
			prtReg.SetAttrib("Value", GetNewInvokeID());

			prtReg.FindElem("COSEM_Attribute_Descriptor");
			prtReg.IntoElem();prtReg.FindElem("COSEM_Instance_Id");
			prtReg.SetAttrib("Value", szHexStrOBIS);

			prtReg.FindElem("Selective_Access_Parameters");
			prtReg.IntoElem();prtReg.FindElem("L0");
			prtReg.SetAttrib("Value", pPacket->szFN);

			prtReg.OutOfElem();prtReg.OutOfElem();		
			prtReg.FindElem("Value");prtReg.IntoElem();
			
			prtReg.FindElem("Name");
			prtReg.SetAttrib("Value", szValue);

			strXMLReg = prtReg.GetDoc();
		}
	}

	//==================================
	// 62056 ProcessServicePrimitive_c
	unsigned char* pOutData = NULL;
	unsigned int OutDataLen;
	int nResult = ProcessServicePrimitive(&pOutData, OutDataLen, UDPIP, 
		(unsigned char*)m_szIDStr, m_Adr, (unsigned char*)strXMLReg.c_str(), strXMLReg.length());
	if (0 != nResult)
	{
		return -1;
	}

	CMarkup xml;
	bool bRet = xml.SetDoc((const char*)pOutData);
	ReleaseMemory(&pOutData);
	if (bRet != false)
	{
		return -1;
	}

	xml.IntoElem();
	xml.FindElem("Root");
	xml.IntoElem();
	xml.FindElem("Frame");
	string strFrame = xml.GetAttrib("Value");

	HexStrToByteArray(strFrame.c_str(), 
		(unsigned char*)pPacket->szSendData, 
		pPacket->nSendDataLen);

	if (0 == pPacket->nSendDataLen)
	{
		return -1;
	}
	return 0;
}

int CDLMS47Ex::Fill47ExDocument(void* pTask, void* pDB)
{
	WSCOMMON::PTASKPACKET pPacket = (WSCOMMON::PTASKPACKET)pTask;

	string strErr = "";
	if (pDB != nullptr)
	{
		vector<string> vecout1, vecout2, vecout3;
		IDBObject* pDBTemp = (IDBObject*)pDB;
		string strSQL = "select check_point_number, sn_meter_type, address  from CFG_DOCUMENT_2016ELE_DPDC where SN_DEVICE = '";
		strSQL += pPacket->szSNTerminal;
		strSQL += "' order by check_point_number ASC";

		UInt32 nRowNum = 0;
		string strDBConTag;
		if (!pDBTemp->BeginBatchRead("", strSQL, 3, strDBConTag))
			return -1;

		pDBTemp->SetReadColData(vecout1, "", strDBConTag);
		pDBTemp->SetReadColData(vecout2, "", strDBConTag);
		pDBTemp->SetReadColData(vecout3, "", strDBConTag);

		pDBTemp->ExcuteBatchRead("", strDBConTag, strErr);
		pDBTemp->EndBatchRead("", strDBConTag, strErr);

		CMarkup xmlDoc;
		xmlDoc.SetDoc(m_strLoadDocXml);
		xmlDoc.IntoElem();xmlDoc.IntoElem();		
		xmlDoc.FindElem("Value");
		xmlDoc.IntoElem();
		xmlDoc.FindElem("L0");
		xmlDoc.IntoElem();
		for (UInt32 i = 0; i < vecout1.size(); i++)
		{
			int nPoint = -1;
			string strPoint("");
			string strMtrTpTag("");
			string strMtrAddr("");

			// check point number
			strPoint = vecout1[i];
			nPoint = atoi(strPoint.c_str());		
			if (nPoint < 0) return -1;

			xmlDoc.InsertElem("L1");
			xmlDoc.SetAttrib("Type", "structure");
			xmlDoc.SetAttrib("Value", "2");
			xmlDoc.IntoElem();
			xmlDoc.InsertElem("L2");
			xmlDoc.SetAttrib("Type", "long-unsigned");
			xmlDoc.SetAttrib("Value", nPoint);

			// sn_meter_type
			string strMtrTp = vecout2[i];
			if (strMtrTp == "1")
			{
				strMtrTpTag = "1p.";
			}
			else if (strMtrTp == "2")
			{
				strMtrTpTag = "3p.";
			}
			else
			{
				strErr = "unknown sn_meter_type:" + strMtrTp + ", check_point_number:" + strPoint;
				return -1;
			}

			// address
			strMtrAddr = vecout3[i];
			UINT nAddrLen = strMtrAddr.length();
			if (!StrIsDigit((char*)strMtrAddr.c_str(), nAddrLen)) return -1;

			strMtrAddr = strMtrTpTag + strMtrAddr;
			unsigned long nMtrAddrSize = strMtrAddr.length();
			char szMtrTpTag[128] = { 0 };
			StrToHEXStr(strMtrAddr.c_str(), szMtrTpTag, nMtrAddrSize);

			xmlDoc.InsertElem("L2");
			xmlDoc.SetAttrib("Type", "octet-string");
			xmlDoc.SetAttrib("Value", szMtrTpTag);

			// append XmlElement
			xmlDoc.OutOfElem();
		}

		string strXMLReg = xmlDoc.GetDoc();
		unsigned char* pOutData = NULL;
		unsigned int OutDataLen;
		int nResult = ProcessServicePrimitive(&pOutData, OutDataLen, UDPIP, 
			(unsigned char*)m_szIDStr, m_Adr, (unsigned char*)strXMLReg.c_str(), strXMLReg.length());
		if (0 != nResult)
		{
			return -1;
		}

		CMarkup xmlResult;
		bool bRet = xmlResult.SetDoc((const char*)pOutData);
		ReleaseMemory(&pOutData);
		if (!bRet)
		{
			return -1;
		}

		xmlResult.IntoElem(); xmlResult.IntoElem();
		xmlResult.FindElem("Frame");
		string strFrame = xmlResult.GetAttrib("Value");

		unsigned long nlongReg = 0;
		HexStrToByteArray(strFrame.c_str(), (unsigned char*)pPacket->szSendData, pPacket->nSendDataLen);
		if (0 == pPacket->nSendDataLen)
		{
			return -1;
		}
		return 0;
	}

	return -1;
}

int CDLMS47Ex::Fill47ExTable(void* pTask, void* pDB)
{
	WSCOMMON::PTASKPACKET pPacket = (WSCOMMON::PTASKPACKET)pTask;

	string strErr = "";
	if (pDB != nullptr)
	{
		vector<string> vecoutsn_a, vecoutobis_a, vecoutid_a, vecoutnum_a;
		vector<string> vecoutsn_b, vecoutindex_b, vecoutcmd_b, vecoutlen_b;
		IDBObject* pDBTemp = (IDBObject*)pDB;
		string strSQL = 
"select a.sn, a.obis, a.table_id, a.data_num, b.sn, \
b.item_index, b.item_cmd, b.item_len from CFG_OBIS_TABLE_DPDC a \
full join CFG_OBIS_TABLE_DETAIL_DPDC b \
on a.obis=b.obis order by a.table_id, b.item_index";

		UInt32 nRowNum = 0;
		string strDBConTag;
		if (!pDBTemp->BeginBatchRead("", strSQL, 3, strDBConTag))
			return -1;

		pDBTemp->SetReadColData(vecoutsn_a, "", strDBConTag);
		pDBTemp->SetReadColData(vecoutobis_a, "", strDBConTag);
		pDBTemp->SetReadColData(vecoutid_a, "", strDBConTag);
		pDBTemp->SetReadColData(vecoutnum_a, "", strDBConTag);
		pDBTemp->SetReadColData(vecoutsn_b, "", strDBConTag);
		pDBTemp->SetReadColData(vecoutindex_b, "", strDBConTag);
		pDBTemp->SetReadColData(vecoutcmd_b, "", strDBConTag);
		pDBTemp->SetReadColData(vecoutlen_b, "", strDBConTag);

		pDBTemp->ExcuteBatchRead("", strDBConTag, strErr);
		pDBTemp->EndBatchRead("", strDBConTag, strErr);

		CMarkup xmlDoc;
		xmlDoc.SetDoc(m_strSetDataTbl);
		xmlDoc.IntoElem(); xmlDoc.IntoElem();
		xmlDoc.FindElem("Value");
		xmlDoc.IntoElem();
		xmlDoc.FindElem("L0");
		xmlDoc.IntoElem();

		char szHexObisReg[100] = { 0 };
		bool bIsNewTable = false;
		UInt32 nTableNum = 0;
		UInt32 nTblItemNum = 0;
		string strSN_A(""), strSN_B("");
		int nTableIDOld = -1, nTableIDNew = -1;
		string strPerObisOld, strPerObisNew;
		int nDataNum = -1;
		int nItemIndex = -1;
		string strItemIdx("");
		string strItemCmd;
		int nItemLen = -1;
		map<int, string> mapItemIDX;
		map<string, string> mapObisIDX;
		string strPerObisValue;
		string strPerItemValue;
		string strValue;

		for (UInt32 i = 0; i < vecout1.size(); i++)
		{
			strSN_A = vecoutsn_a[i];
			strSN_B = vecoutsn_b[i];

			// table id
			nTableIDNew = atoi(vecoutid_a[i].c_str());
			if ((nTableIDNew < 1) || (nTableIDNew > 65535))
			{
				return -1
			}

			if (nTableIDOld != nTableIDNew)
			{
				if ((-1 != nTableIDOld) && (nDataNum != nTblItemNum))          // check data_num
				{
					return -1;
				}

				bIsNewTable = true;
				mapItemIDX.clear();
				nTblItemNum = 0;
				nTableNum++;
				nTableIDOld = nTableIDNew;
			}

			// OBIS
			strPerObisNew = vecoutobis_a[i];
			if (strPerObisOld != strPerObisNew)     // new obis
			{
				if (mapObisIDX.find(strPerObisNew))  // check unique
				{
					return -1;
				}
				else
				{
					mapObisIDX.insert(strPerObisNew);
				}

				if (!bIsNewTable)
				{
					return -1;
				}
				strPerObisOld = strPerObisNew;
			}

			if (!OBISToHexStr((char*)strPerObisNew.c_str(), szHexObisReg))
			{
				return -1;
			}

			// data num
			nDataNum = vecoutnum_a[i].c_str();
			if ((nDataNum < 1) || (nDataNum > 255))
			{
				return -1;
			}

			// ITEM_INDEX
			strItemIdx = vecoutindex_b[i].c_str();
			nItemIndex = atoi(strItemIdx.c_str());
			if (nItemIndex < 1)
			{
				return -1;
			}

			if (mapItemIDX.find(strItemIdx) != mapItemIDX.end())
			{
				return -1;
			}
			else
			{
				mapItemIDX.insert(strItemIdx);
				nTblItemNum++;
			}

			// ITEM_CMD
			strItemCmd = vecoutcmd_b[i];
			if (4 != strItemCmd.length())
			{
				return -1;
			}

			// ITEM_LEN
			nItemLen = atoi(vecoutlen_b[i].c_str()); 
			if ((nItemLen < 0) || (nItemLen > 65535))
			{
				return -1;
			}

			// get frame
			if (bIsNewTable)
			{
				char szBuf[256] = {0};
				sprintf_s(szBuf, "%s02030906%s12%04X01%02X", strPerItemValue.c_str(), szHexObisReg, nTableIDNew, nDataNum);
				strPerObisValue = szBuf;
				strPerItemValue = "";
				strValue += strPerObisValue;
			}
			//strPerItemValue.cat_printf("020212%s12%04X", szItemCmd, nItemLen);
		}
	}
	return -1;
}