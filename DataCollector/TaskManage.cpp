#include "stdafx.h"
#include "TaskManage.h"

CTaskManage::CTaskManage()
{
}

CTaskManage::~CTaskManage()
{
}

bool CTaskManage::Init(IDBObject * pDBObj, const string & strDBTag)
{
	bool bRet = false;
	if (pDBObj != nullptr)
	{
		m_pDBOBject = pDBObj;
		m_strDBTag = strDBTag;

		m_pDBOBject->BeginExcuteSQL(strDBTag, m_strExcuteSqlConTag);

		bRet = m_pDBOBject->BeginProcedure(strDBTag, m_strGetCommonTaskData,
			m_strTagName_GetCommonTaskData, 1, 24, true);

		if (bRet)
		{
			// cursor游标
			m_pDBOBject->AddParamOut(strDBTag, m_strTagName_GetCommonTaskData);

			// 一共返回24个字段N行的数据
			m_pDBOBject->ReadProcedureResult(m_vecout1,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout2,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout3,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout4,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout5,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout6,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout7,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout8,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout9,  m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout10, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout11, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout12, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout13, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout14, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout15, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout16, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout17, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout18, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout19, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout20, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout21, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout22, m_strDBTag, m_strTagName_GetCommonTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout23, m_strDBTag, m_strTagName_GetCommonTaskData);
		}

		bRet = m_pDBOBject->BeginProcedure(strDBTag, m_strGetScheduleTaskData,
			m_strTagName_GetScheduleTaskData, 1, 24, true);

		if (bRet)
		{
			// cursor游标
			m_pDBOBject->AddParamOut(strDBTag, m_strTagName_GetScheduleTaskData);

			// 一共返回24个字段N行的数据
			m_pDBOBject->ReadProcedureResult(m_vecout1, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout2, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout3, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout4, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout5, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout6, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout7, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout8, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout9, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout10, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout11, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout12, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout13, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout14, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout15, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout16, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout17, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout18, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout19, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout20, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout21, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout22, m_strDBTag, m_strTagName_GetScheduleTaskData);
			m_pDBOBject->ReadProcedureResult(m_vecout23, m_strDBTag, m_strTagName_GetScheduleTaskData);
		}
	}
		
	return bRet;
}

bool CTaskManage::InertTaskMonth(const string& strKey, UInt32 year, UInt32 month, UInt32 monthBack)
{
	bool bRet = false;
	if (m_pDBOBject != nullptr)
	{
		string strConTag = "";
		bRet = m_pDBOBject->BeginProcedure(m_strDBTag, "SP_INSERT_TASK_MONTH", strConTag, 4);

		m_pDBOBject->AddParamIn(strKey, m_strDBTag, strConTag);
		m_pDBOBject->AddParamIn(year, m_strDBTag, strConTag);
		m_pDBOBject->AddParamIn(month, m_strDBTag, strConTag);
		m_pDBOBject->AddParamIn(monthBack, m_strDBTag, strConTag);

		if (bRet)
		{
			string strErrOut = "";
			bRet = m_pDBOBject->EndProcedure(m_strDBTag, strConTag, strErrOut);
		}
	}

	return bRet;
}

bool CTaskManage::InertTaskDay(const string& strKey, UInt32 year, UInt32 month, UInt32 day, UInt32 dayBack)
{
	bool bRet = false;
	if (m_pDBOBject != nullptr)
	{
		string strConTag = "";
		bRet = m_pDBOBject->BeginProcedure(m_strDBTag, "SP_INSERT_TASK_DAY", strConTag, 5);

		m_pDBOBject->AddParamIn(strKey, m_strDBTag, strConTag);
		m_pDBOBject->AddParamIn(year, m_strDBTag, strConTag);
		m_pDBOBject->AddParamIn(month, m_strDBTag, strConTag);
		m_pDBOBject->AddParamIn(day, m_strDBTag, strConTag);
		m_pDBOBject->AddParamIn(dayBack, m_strDBTag, strConTag);

		if (bRet)
		{
			string strErrOut = "";
			bRet = m_pDBOBject->EndProcedure(m_strDBTag, strConTag, strErrOut);
		}
	}

	return bRet;
}

bool CTaskManage::GenerateTask(const string& strScheduleName, const string& strSNCommand, const string& strType)
{
	bool bRet = false;
	if (m_pDBOBject != nullptr)
	{
		string strConTag = "";
		bRet = m_pDBOBject->BeginProcedure(m_strDBTag, strScheduleName, strConTag, 2);
		m_pDBOBject->AddParamIn(m_strDBTag, strConTag, strSNCommand);
		m_pDBOBject->AddParamIn(m_strDBTag, strConTag, strType);

		if (bRet)
		{
			string strErrOut = "";
			bRet = m_pDBOBject->EndProcedure(m_strDBTag, strConTag, strErrOut);
		}	
	}

	return false;
}

bool CTaskManage::GetScheduleTaskContext(vector<TASKPACKET>& vecTaskData)
{
	m_vecout1.clear(); m_vecout2.clear(); m_vecout3.clear();
	m_vecout4.clear(); m_vecout5.clear(); m_vecout6.clear();
	m_vecout7.clear(); m_vecout8.clear(); m_vecout9.clear();
	m_vecout10.clear(); m_vecout11.clear(); m_vecout12.clear();
	m_vecout13.clear(); m_vecout14.clear(); m_vecout15.clear();
	m_vecout16.clear(); m_vecout17.clear(); m_vecout18.clear();
	m_vecout19.clear(); m_vecout20.clear(); m_vecout21.clear();
	m_vecout22.clear(); m_vecout23.clear();

	string strError = "";
	if (m_pDBOBject->ExcuteProcedure(m_strDBTag, m_strTagName_GetScheduleTaskData, strError))
	{
		for (unsigned int i = 0; i < m_vecout1.size(); i++)
		{
			TASKPACKET taskData;

			strcpy_s(taskData.szSNTask, m_vecout23[i].c_str());
			strcpy_s(taskData.szSNMeter, m_vecout2[i].c_str());
			strcpy_s(taskData.szSNTerminal, m_vecout1[i].c_str());
			strcpy_s(taskData.szTransparent_command, m_vecout14[i].c_str());

			strcpy_s(taskData.szAFN, m_vecout10[i].c_str());
			strcpy_s(taskData.szFN, m_vecout11[i].c_str());
			strcpy_s(taskData.szTerminalAddress, m_vecout7[i].c_str());

			taskData.nMeasuring_point = atoi(m_vecout22[i].c_str());
			taskData.cMeterType = atoi(m_vecout15[i].c_str());;
			taskData.cProtocalType = atoi(m_vecout8[i].c_str());

			vecTaskData.push_back(taskData);
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool CTaskManage::GetCommonTaskContext(vector<TASKPACKET>& vecTaskData)
{
	m_vecout1.clear(); m_vecout2.clear(); m_vecout3.clear();
	m_vecout4.clear(); m_vecout5.clear(); m_vecout6.clear();
	m_vecout7.clear(); m_vecout8.clear(); m_vecout9.clear();
	m_vecout10.clear(); m_vecout11.clear(); m_vecout12.clear();
	m_vecout13.clear(); m_vecout14.clear(); m_vecout15.clear();
	m_vecout16.clear(); m_vecout17.clear(); m_vecout18.clear();
	m_vecout19.clear(); m_vecout20.clear(); m_vecout21.clear();
	m_vecout22.clear(); m_vecout23.clear();

	string strError = "";
	if (m_pDBOBject->ExcuteProcedure(m_strDBTag, m_strTagName_GetCommonTaskData, strError))
	{
		for (unsigned int i = 0; i < m_vecout1.size(); i++)
		{
			TASKPACKET taskData;

			strcpy_s(taskData.szSNTask, m_vecout23[i].c_str());
			strcpy_s(taskData.szSNMeter, m_vecout2[i].c_str());
			strcpy_s(taskData.szSNTerminal, m_vecout1[i].c_str());
			strcpy_s(taskData.szTransparent_command, m_vecout14[i].c_str());

			strcpy_s(taskData.szAFN, m_vecout10[i].c_str());
			strcpy_s(taskData.szFN, m_vecout11[i].c_str());
			strcpy_s(taskData.szTerminalAddress, m_vecout7[i].c_str());

			taskData.nMeasuring_point = atoi(m_vecout22[i].c_str());
			taskData.cMeterType = atoi(m_vecout15[i].c_str());;
			taskData.cProtocalType = atoi(m_vecout8[i].c_str());

			vecTaskData.push_back(taskData);
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool CTaskManage::UpdateCommonTask(TASKPACKET TaskData)
{
	char szSQL[4098] = { 0 };
	string strTableName = "RT_TASK_ONLINE";
	if (strlen(TaskData.szCompleteTime) > 0)
	{
		sprintf_s(szSQL, "update %s set t.IS_EXECUTED=%d,t.IS_SUCCESS=%d,t.RETURN_DATA=%s,t.completion_date=to_date('%s',''yyyy/MM/dd HH24:mi:ss'') where sn= %s", 
			strTableName.c_str(), TaskData.nTaskStatus,
			TaskData.nTaskSuccess, TaskData.szReturnData, TaskData.szCompleteTime, 
			TaskData.szSNTask);
	}
	else
	{
		sprintf_s(szSQL, "update %s set t.IS_EXECUTED=%d,t.IS_SUCCESS=%d,t.RETURN_DATA='%s' where t.sn= '%s'", 
			strTableName.c_str(), TaskData.nTaskStatus, TaskData.nTaskSuccess, 
			TaskData.szReturnData, TaskData.szSNTask);
	}

	string strError = "";
	bool bRet = m_pDBOBject->ExcuteSQL(m_strDBTag, m_strExcuteSqlConTag, szSQL, strError);
	return bRet;
}

bool CTaskManage::UpdateScheduleTask(TASKPACKET TaskData)
{
	char szSQL[4098] = { 0 };
	string strTableName = "RT_TASK_SCHEDULE";
	if (strlen(TaskData.szCompleteTime) > 0)
	{
		sprintf_s(szSQL, "update %s set t.IS_EXECUTED=%d,t.IS_SUCCESS=%d,t.completion_date=to_date('%s',''yyyy/MM/dd HH24:mi:ss'') where sn= %s",
			strTableName.c_str(), TaskData.nTaskStatus, TaskData.nTaskSuccess,TaskData.szCompleteTime, TaskData.szSNTask);
	}
	else
	{
		sprintf_s(szSQL, "update %s set t.IS_EXECUTED=%d,t.IS_SUCCESS=%d where t.sn= '%s'",
			strTableName.c_str(), TaskData.nTaskStatus, TaskData.nTaskSuccess,TaskData.szSNTask);
	}

	string strError = "";
	bool bRet = m_pDBOBject->ExcuteSQL(m_strDBTag, m_strExcuteSqlConTag, szSQL, strError);
	return bRet;
}


/* 获取定抄任务的对应属性 */
bool CTaskManage::GetScheduleTaskProperty(map<string, 
	tuple<string, string, string, string, string, map<UInt8, string>>>& mapSpecialTaskDefine)
{
	bool bRet = false;
	if (m_pDBOBject != nullptr)
	{
		// 定抄计划存储在CFG_TASK_SCHEDULER表中
		// 依次读出AFN FN 对应的存储过程，结果保存的表名
		string strConTag, strError;
		vector<string> vecAFN, vecFN, vecProduce, vecSN;
		vector<string> vecTableName, vecEnable, vecType;
		string strSQL = "select * from cfg_task_schedule";
		m_pDBOBject->BeginBatchRead(m_strDBTag, strSQL, 6, strConTag);
		m_pDBOBject->SetReadColData(vecAFN, m_strDBTag, strConTag);
		m_pDBOBject->SetReadColData(vecFN, m_strDBTag, strConTag);
		m_pDBOBject->SetReadColData(vecProduce, m_strDBTag, strConTag);
		m_pDBOBject->SetReadColData(vecTableName, m_strDBTag, strConTag);
		m_pDBOBject->SetReadColData(vecEnable, m_strDBTag, strConTag);
		m_pDBOBject->SetReadColData(vecType, m_strDBTag, strConTag);
		m_pDBOBject->SetReadColData(vecSN, m_strDBTag, strConTag);
		m_pDBOBject->ExcuteBatchRead(m_strDBTag, strConTag, strError);
		m_pDBOBject->EndBatchRead(m_strDBTag, strConTag, strError);

		map<UInt8, string> mapTableProperty;
		m_pDBOBject->BeginGetTableProperty(m_strDBTag, strConTag);
		for (UInt32 i = 0; i < vecAFN.size(); i++)
		{
			// 通过表名读出表一共有多少列，每列的名称
			// 定抄入库的时候，将根据这些项来自动匹配
			if (vecEnable[i] == "1")
			{
				if (m_pDBOBject->ExcuteGetTablePropertry(m_strDBTag, strConTag,
					vecTableName[i], mapTableProperty, strError))
				{
					// PRODUCENAME TABLENAME, TYPE, TABLEPROPERTY
					mapSpecialTaskDefine.insert(make_pair(vecSN[i],
						make_tuple(vecAFN[i], vecFN[i], vecProduce[i], vecTableName[i],
							vecType[i], mapTableProperty)));
				}
			}
		}
		m_pDBOBject->EndGetTableProperty(m_strDBTag, strConTag);
		bRet = true;
	}
	return bRet;
}

/* 定抄数据入库*/
bool CTaskManage::BatchInsertScheduleData(const string& strTableName, const string& strColName,
	UInt32 nColCnt, map<UInt8, vector<string>>& mapColData)
{
	bool bRet = false;
	if (m_pDBOBject != nullptr)
	{
		string strTagName_InsertScheduleData;
		m_pDBOBject->BeginBatchInsert(m_strDBTag, strTableName, strColName, nColCnt, strTagName_InsertScheduleData);
		for (UInt32 i = 0; i < nColCnt; i++)
		{
			m_pDBOBject->SetInsertColData(mapColData[i], m_strDBTag, strTagName_InsertScheduleData);
		}
		string strErrOut = "";
		bRet = m_pDBOBject->EndBatchInsert(m_strDBTag, strTagName_InsertScheduleData, strErrOut);
	}
	return bRet;
}




