#include "OrcalInterFaceImpl.h"	

COrcalObject::COrcalObject()
{
	m_sessionFactory = shared_ptr<occiwrapper::SessionFactory>(new occiwrapper::SessionFactory("","",1));

#ifdef WIN32
	InitializeSRWLock(&m_srwConnect);
	InitializeSRWLock(&m_srwSessionSQL);
	InitializeSRWLock(&m_srwSessionBatchInsert);
	InitializeSRWLock(&m_srwSessionBatchRead);
	InitializeSRWLock(&m_srwSessionProcedure);
	InitializeSRWLock(&m_srwSessionTablePro);
#endif
}
COrcalObject::~COrcalObject(){;}

/*	打开数据库
*	strDBName: 数据库名
*	strUserName: 登录用户名
*	strPassWord: 登录密码
*	strIP: 数据库IP地址
*	nPort: 数据库端口
*	strErrOut：函数返回失败后的错误信息
*/
bool COrcalObject::OpenDB(const string& strDBName, const string& strUserName, 
		const string& strPassWord, const string& strIP, 
		unsigned int nPort, string& strDBTagOut, string& strErrOut)
{
	if (m_sessionFactory.get() != 0)
	{
		// m_sessionFactory 包含一个连接池，下面所有调用过程，都是分配一个session，每个session
		// 从连接池中获取一个连接，用这个连接完成查询，更新，插入等数据库操作
		occiwrapper::ConnectionInfo conInfo(strIP, nPort, strUserName, strPassWord, strDBName);
		shared_ptr<occiwrapper::Connection> con = m_sessionFactory->CreateConnect(conInfo);
		if (con.get() != 0 && (con->GetValidity() == occiwrapper::VALID))
		{
			strDBTagOut = conInfo.GetHashString();
			m_mapConnection.insert(make_pair(strDBTagOut, con));
			return true;
		}
		strErrOut = con->GetErrMsg();
	}
	return false;
}

	/*	关闭数据库
	*	strErrOut：函数返回失败后的错误信息
	*/
bool COrcalObject::CloseDB(const string& strDBTag, string& strErr)
{

	return true;
}

/*	批量数据插入
*	strTableName：表名
*	strColsName: 列名集合,列名之间用逗号隔开
*	(such as "string_value, date_value, int_value, float_value, number_value")
*	nColNum：一共有多少列
*
*	下面三个函数，为一组函数，不能单独使用，使用规则如下：
*
*	BeginBatchInsert();
*	for (int i = 0; i< nColNum; i++)
*		SetInsertColData();
*	EndBatchInsert();
*	执行完成之后，数据库环境即失效
*/
bool COrcalObject::BeginBatchInsert(const string& strDBTag, const string& strTableName, 
		const string& strColsName, unsigned int nColNum, string& strTagOut)
{
	if (m_mapConnection.size() <= 0) return false;

	// 判断参数的合法性
	UInt32 nColNumTemp = 0;
	string strTemp = strColsName;
	char* pContext = nullptr;
	char* pToken = strtok_s((char*)strTemp.c_str(),",",&pContext);
	while ( pToken != nullptr)
	{
		pToken = strtok_s(nullptr,",",&pContext);
		nColNumTemp ++ ;
	}

	if (nColNumTemp != nColNum)
	{
		return false;
	}

	do 
	{
		std::random_device rd;
		std::mt19937 mt(rd());
		char szRandom[64] = {0};
		sprintf(szRandom, "%u", mt());
		strTagOut = szRandom;

	} while (m_mapSessionBatchInsert.find(strTagOut) != m_mapSessionBatchInsert.end());

	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	//if (m_mapConnection.find(strDBTag) != m_mapConnection.end())
	//{
		typedef shared_ptr<occiwrapper::Session> _ptrSession;
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = _ptrSession(new occiwrapper::Session(con,false,strTagOut));

		string strSQL;
		strSQL = "insert into " + strTableName + " (" + strColsName + ")" + " values ( ";
		for (unsigned int i=1; i<=nColNum; i++)
		{
			char szValueIdx[32] = {0};
			if (i == nColNum)
			{
				sprintf(szValueIdx, ":%d) ", i);
			}
			else
			{
				sprintf(szValueIdx, ":%d, ", i);
			}
			strSQL += szValueIdx;
		}
		*session << strSQL,bRet,strErrMsg;

#ifdef WIN32
		AcquireSRWLockExclusive(&m_srwSessionBatchInsert);
		m_mapSessionBatchInsert.insert(make_pair(strTagOut, make_tuple(session,nColNum,0,0,0)));
		ReleaseSRWLockExclusive(&m_srwSessionBatchInsert);
#endif
	//}

	return true;
}

int COrcalObject::SetInsertColData(const vector<Int32>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<Int64>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<Int16>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<Int8>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<UInt32>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<UInt64>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<UInt16>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<UInt8>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<string>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<char*>& vecCol, const string& strDBTag, const string& strTag)
{
	return -1;
}

int COrcalObject::SetInsertColData(const vector<float>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<double>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

int COrcalObject::SetInsertColData(const vector<bool>& vecCol, const string& strDBTag, const string& strTag)
{
	return -1;
}

int COrcalObject::SetInsertColData(const vector<struct tm>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchInsert);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);
		(get<2>(m_mapSessionBatchInsert[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchInsert[strTag]) <= get<1>(m_mapSessionBatchInsert[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchInsert[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
			*session, batched_use(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchInsert);
#endif
	return nSize;
}

bool COrcalObject::EndBatchInsert(const string& strDBTag,  const string& strTag, string& strErrOut)
{
	if (m_mapConnection.size() <= 0) return false;

	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchInsert.find(strTag) != m_mapSessionBatchInsert.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchInsert[strTag]);

		*session,now,bRet,strErrMsg;
		con->Commit(strTag);

		// 删除session

	}
	return bRet;
}

/*	数据读取
*	strTableName：表名
*	strColsName: 列名集合,列名之间用逗号隔开
*	(such as "string_value, date_value, int_value, float_value, number_value")
*	nColNum：一共有多少列
*
*	下面三个函数，为一组函数，不能单独使用，使用规则如下：
*
*	BeginRead();
*	for (int i = 0; i< nColNum; i++)
*		SetReadColData();
*	EndRead();
*	执行完成之后，数据库环境即失效
*/
bool COrcalObject::BeginBatchRead(const string& strDBTag, const string& strSql, 
		unsigned int nColNum, string& strTagOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockExclusive(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	do 
	{
		std::random_device rd;
		std::mt19937 mt(rd());
		char szRandom[64] = {0};
		sprintf(szRandom, "%u", mt());
		strTagOut = szRandom;

	} while (m_mapSessionBatchRead.find(strTagOut) != m_mapSessionBatchRead.end());

	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	//if (m_mapConnection.find(strDBTag) != m_mapConnection.end())
	//{
		typedef shared_ptr<occiwrapper::Session> _ptrSession;
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = _ptrSession(new occiwrapper::Session(con,true,strTagOut));

		*session << strSql;
		m_mapSessionBatchRead.insert(make_pair(strTagOut, make_tuple(session,nColNum,0,0,0)));
		bRet = true;
	//}
#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockExclusive(&m_srwSessionBatchRead);
#endif
	return bRet;
}

int COrcalObject::SetReadColData(vector<Int32>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<Int64>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<Int16>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<Int8>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<UInt32>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<UInt64>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<UInt16>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<UInt8>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<string>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<char*>& vecCol, const string& strDBTag, const string& strTag)
{
	return -1;
}

int COrcalObject::SetReadColData(vector<float>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<double>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

int COrcalObject::SetReadColData(vector<bool>& vecCol, const string& strDBTag, const string& strTag)
{
	return -1;
}

int COrcalObject::SetReadColData(vector<struct tm>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		(get<2>(m_mapSessionBatchRead[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionBatchRead[strTag]) <= get<1>(m_mapSessionBatchRead[strTag]))
		{
			nSize = get<2>(m_mapSessionBatchRead[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
			*session, into(vecCol);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return nSize;
}

bool COrcalObject::ExcuteBatchRead(const string & strDBTag, const string & strTag, string & strErrOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	bool bRet = false;
	string strValue;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionBatchRead.find(strTag) != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second; //m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		*session, now, bRet, strErrOut;
	}
#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionBatchRead);
#endif
	return bRet;
}

bool COrcalObject::EndBatchRead(const string& strDBTag, const string& strTag, string& strErrOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockExclusive(&m_srwSessionBatchRead);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	bool bRet = false;
	string strValue, strErrMsg;
	typedef map<string, shared_ptr<occiwrapper::Connection>>::iterator _mapiter1;
	typedef map<string, tuple<shared_ptr<occiwrapper::Session>, UInt32, UInt32, UInt32, UInt32>>::iterator _mapiter2;
	_mapiter1 iterConn = m_mapConnection.find(strDBTag);
	_mapiter2 iterSess = m_mapSessionBatchRead.find(strTag);

	if (iterConn == m_mapConnection.end()) iterConn = m_mapConnection.begin();

	if (/*iterConn != m_mapConnection.end() &&*/ iterSess != m_mapSessionBatchRead.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionBatchRead[strTag]);
		// 删除session
		m_mapSessionBatchRead.erase(iterSess);
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockExclusive(&m_srwSessionBatchRead);
#endif
	return bRet;
}

/*	执行存储过程
*	strTableName：表名
*	strColsName: 列名集合,列名之间用逗号隔开
*	(such as "string_value, date_value, int_value, float_value, number_value")
*	nColNum：存储过程
*
*	下面三个函数，为一组函数，不能单独使用，使用规则如下：
*
*	BeginProcedure();
*	for (int j = 0; j< nParamNum; j++)
*		AddParamIn(const );
*	for (int i = 0; i< nColNum; i++)
*		ReadProcedureResult();
*	EndProcedure();
*	执行完成之后，数据库环境即失效
*/
bool COrcalObject::BeginProcedure(const string& strDBTag, const string& strProcedureName, string& strTagOut,
		unsigned int nParamNum, unsigned int nColNum, bool bHasCursor)
{
	if (m_mapConnection.size() <= 0) return false;

	do 
	{
		std::random_device rd;
		std::mt19937 mt(rd());
		char szRandom[64] = {0};
		sprintf(szRandom, "%u", mt());
		strTagOut = szRandom;

	} while (m_mapSessionProcedure.find(strTagOut) != m_mapSessionProcedure.end());

	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	///if (m_mapConnection.find(strDBTag) != m_mapConnection.end())
	//{
		typedef shared_ptr<occiwrapper::Session> _ptrSession;
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = _ptrSession(new occiwrapper::Session(con,false,strTagOut, bHasCursor));

		string strSQL;
		strSQL = "begin " + strProcedureName + "( ";
		if (nParamNum > 0)
		{
			for (unsigned int i = 1; i <= nParamNum; i++)
			{
				char szValueIdx[32] = { 0 };
				if (i == nParamNum)
				{
					sprintf(szValueIdx, ":%d); end;", i);
				}
				else
				{
					sprintf(szValueIdx, ":%d, ", i);
				}
				strSQL += szValueIdx;
			}
		}
		else
		{
			strSQL += "); end;";
		}

		*session << strSQL;

		AcquireSRWLockExclusive(&m_srwSessionProcedure);
		m_mapSessionProcedure.insert(make_pair(strTagOut, make_tuple(session,nColNum,0,nParamNum,0)));
		ReleaseSRWLockExclusive(&m_srwSessionProcedure);
	//}

	return true;
}

int COrcalObject::AddParamIn(const Int32& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);			
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const Int64& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const Int16& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const Int8& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const UInt32& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const UInt64& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const UInt16& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const UInt8& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const string& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const char* Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const float& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const double& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const bool& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamIn(const struct tm& time, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(time, occiwrapper::PAR_IN);
			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}


int COrcalObject::AddParamOut(Int32& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_OUT);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamOut(Int64& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_OUT);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamOut(UInt32& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_OUT);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamOut(UInt64& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_OUT);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamOut(string& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_OUT);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamOut(float& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_OUT);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamOut(double& Param, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, use(Param, occiwrapper::PAR_OUT);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::AddParamOut(const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;
	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<4>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<4>(m_mapSessionProcedure[strTag]) <= get<3>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<4>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, useCursor(1,occiwrapper::PAR_OUT);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<Int32>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<Int64>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<Int16>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<Int8>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<UInt32>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<UInt64>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<UInt16>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<UInt8>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<string>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<char*>& vecCol, const string& strDBTag, const string& strTag)
{
	int nSize = 0;
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<float>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<double>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<bool>& vecCol, const string& strDBTag, const string& strTag)
{
	int nSize = 0;
	return nSize;
}

int COrcalObject::ReadProcedureResult(vector<struct tm>& vecCol, const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return -1;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	int nSize = 0;
	bool bRet = false;
	string strValue, strErrMsg;

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	if (/*m_mapConnection.find(strDBTag) != m_mapConnection.end() &&*/
		m_mapSessionProcedure.find(strTag) != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		(get<2>(m_mapSessionProcedure[strTag]))++; // current colums ++ 
		if (get<2>(m_mapSessionProcedure[strTag]) <= get<1>(m_mapSessionProcedure[strTag]))
		{
			nSize = get<2>(m_mapSessionProcedure[strTag]);
#ifdef WIN32
			ReleaseSRWLockShared(&m_srwConnect);
			ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
			*session, into(vecCol);

			return nSize;
		}
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return nSize;
}

bool COrcalObject::ExcuteProcedure(const string & strDBTag, const string & strTag, string & strErrOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	bool bRet = false;
	string strValue, strErrMsg;
	typedef map<string, shared_ptr<occiwrapper::Connection>>::iterator _mapiter1;
	typedef map<string, tuple<shared_ptr<occiwrapper::Session>, UInt32, UInt32, UInt32, UInt32>>::iterator _mapiter2;
	_mapiter1 iterConn = m_mapConnection.find(strDBTag);
	_mapiter2 iterSess = m_mapSessionProcedure.find(strTag);

	if (iterConn == m_mapConnection.end()) iterConn = m_mapConnection.begin();

	if (/*iterConn != m_mapConnection.end() &&*/ iterSess != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		*session, now, bRet, strErrMsg;
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return bRet;
}

bool COrcalObject::EndProcedure(const string& strDBTag, const string& strTag, string& strErrOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionProcedure);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	bool bRet = false;
	string strValue, strErrMsg;
	typedef map<string, shared_ptr<occiwrapper::Connection>>::iterator _mapiter1;
	typedef map<string, tuple<shared_ptr<occiwrapper::Session>, UInt32, UInt32, UInt32, UInt32>>::iterator _mapiter2;
	_mapiter1 iterConn = m_mapConnection.find(strDBTag);
	_mapiter2 iterSess = m_mapSessionProcedure.find(strTag);

	if (iterConn == m_mapConnection.end()) iterConn = m_mapConnection.begin();

	if (/*iterConn != m_mapConnection.end() &&*/ iterSess != m_mapSessionProcedure.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionProcedure[strTag]);
		*session,now,bRet,strErrMsg;

		// 删除session
		m_mapSessionProcedure.erase(iterSess);
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionProcedure);
#endif
	return bRet;
}

bool COrcalObject::BeginExcuteSQL(const string& strDBTag, string& strTagOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockExclusive(&m_srwSessionSQL);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	bool bRet = false;
	string strValue = "";
	do
	{
		std::random_device rd;
		std::mt19937 mt(rd());
		char szRandom[64] = { 0 };
		sprintf(szRandom, "%u", mt());
		strTagOut = szRandom;

	} while (m_mapSessionSQL.find(strTagOut) != m_mapSessionSQL.end());

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	//if (m_mapConnection.find(strDBTag) != m_mapConnection.end())
	//{
		typedef shared_ptr<occiwrapper::Session> _ptrSession;
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = _ptrSession(new occiwrapper::Session(con, true, strTagOut));

		//*session << strSQL, now, bRet, strErrOut;
		m_mapSessionSQL.insert(make_pair(strTagOut, make_tuple(session, 0, 0, 0, 0)));
	//}
#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockExclusive(&m_srwSessionSQL);
#endif
	return bRet;
}

bool COrcalObject::ExcuteSQL(const string& strDBTag, const string& strTag, const string& strSQL, string& strErrOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionSQL);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	bool bRet = false;
	string strValue, strErrMsg;
	typedef map<string, shared_ptr<occiwrapper::Connection>>::iterator _mapiter1;
	typedef map<string, tuple<shared_ptr<occiwrapper::Session>, UInt32, UInt32, UInt32, UInt32>>::iterator _mapiter2;
	_mapiter1 iterConn = m_mapConnection.find(strDBTag);
	_mapiter2 iterSess = m_mapSessionSQL.find(strTag); 
	if (iterConn == m_mapConnection.end()) iterConn = m_mapConnection.begin();

	if (/*iterConn != m_mapConnection.end() &&*/ iterSess != m_mapSessionSQL.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionSQL[strTag]);
		occiwrapper::Statement stmt = (*session << strSQL);
		
		stmt.Reset();
		stmt,now, bRet, strErrOut;
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionSQL);
#endif
	return bRet;
}

bool COrcalObject::EndExcuteSQL(const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockExclusive(&m_srwSessionSQL);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	bool bRet = false;
	string strValue, strErrMsg;
	typedef map<string, shared_ptr<occiwrapper::Connection>>::iterator _mapiter1;
	typedef map<string, tuple<shared_ptr<occiwrapper::Session>, UInt32, UInt32, UInt32, UInt32>>::iterator _mapiter2;
	_mapiter1 iterConn = m_mapConnection.find(strDBTag);
	_mapiter2 iterSess = m_mapSessionSQL.find(strTag);
	if (iterConn == m_mapConnection.end()) iterConn = m_mapConnection.begin();

	if (/*iterConn != m_mapConnection.end() &&*/ iterSess != m_mapSessionSQL.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = get<0>(m_mapSessionSQL[strTag]);

		// 删除session
		m_mapSessionSQL.erase(iterSess);
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockExclusive(&m_srwSessionSQL);
#endif
	return bRet;
}

bool COrcalObject::BeginGetTableProperty(const string& strDBTag, string& strTagOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockExclusive(&m_srwSessionTablePro);
	AcquireSRWLockShared(&m_srwConnect);
#endif 
	bool bRet = false;
	string strValue = "";
	do
	{
		std::random_device rd;
		std::mt19937 mt(rd());
		char szRandom[64] = { 0 };
		sprintf(szRandom, "%u", mt());
		strTagOut = szRandom;

	} while (m_mapSessionTablePro.find(strTagOut) != m_mapSessionTablePro.end());

	map<string, shared_ptr<occiwrapper::Connection>>::iterator iter = m_mapConnection.find(strDBTag);
	if (iter == m_mapConnection.end()) iter = m_mapConnection.begin();

	//if (m_mapConnection.find(strDBTag) != m_mapConnection.end())
	//{
		typedef shared_ptr<occiwrapper::Session> _ptrSession;
		shared_ptr<occiwrapper::Connection> con = iter->second;//m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = _ptrSession(new occiwrapper::Session(con, true, strTagOut));
		m_mapSessionTablePro.insert(make_pair(strTagOut, session));
		bRet = true;
	//}
#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockExclusive(&m_srwSessionTablePro);
#endif
	return bRet;
}

bool COrcalObject::ExcuteGetTablePropertry(const string& strDBTag, const string& strTag,
	const string& strTableName, map<UInt8, string>& mapTablePro, string& strErrOut)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockShared(&m_srwSessionTablePro);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	bool bRet = false;
	string strValue, strErrMsg;
	typedef map<string, shared_ptr<occiwrapper::Connection>>::iterator _mapiter1;
	typedef map<string, shared_ptr<occiwrapper::Session>>::iterator _mapiter2;
	_mapiter1 iterConn = m_mapConnection.find(strDBTag);
	_mapiter2 iterSess = m_mapSessionTablePro.find(strTag);
	if (iterConn == m_mapConnection.end()) iterConn = m_mapConnection.begin();

	if (/*iterConn != m_mapConnection.end() &&*/ iterSess != m_mapSessionTablePro.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = m_mapSessionTablePro[strTag];
		occiwrapper::Statement stmt = (*session, bRet);

		if (!stmt.GetTableProperty(strTableName, mapTablePro))
			strErrOut = stmt.GetErrMsg();

		bRet = true;
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockShared(&m_srwSessionTablePro);
#endif
	return bRet;
}

bool COrcalObject::EndGetTableProperty(const string& strDBTag, const string& strTag)
{
	if (m_mapConnection.size() <= 0) return false;

#ifdef WIN32
	AcquireSRWLockExclusive(&m_srwSessionTablePro);
	AcquireSRWLockShared(&m_srwConnect);
#endif 

	bool bRet = false;
	string strValue, strErrMsg;
	typedef map<string, shared_ptr<occiwrapper::Connection>>::iterator _mapiter1;
	typedef map<string, shared_ptr<occiwrapper::Session>>::iterator _mapiter2;
	_mapiter1 iterConn = m_mapConnection.find(strDBTag);
	_mapiter2 iterSess = m_mapSessionTablePro.find(strTag);
	if (iterConn == m_mapConnection.end()) iterConn = m_mapConnection.begin();

	if (/*iterConn != m_mapConnection.end() &&*/ iterSess != m_mapSessionTablePro.end())
	{
		shared_ptr<occiwrapper::Connection> con = m_mapConnection[strDBTag];
		shared_ptr<occiwrapper::Session> session = m_mapSessionTablePro[strTag];

		// 删除session
		m_mapSessionTablePro.erase(iterSess);
	}

#ifdef WIN32
	ReleaseSRWLockShared(&m_srwConnect);
	ReleaseSRWLockExclusive(&m_srwSessionTablePro);
#endif
	return bRet;
}