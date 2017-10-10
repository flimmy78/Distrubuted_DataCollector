#include "OcciWrapper/Connection.h"
#include <iostream>
using namespace std;

occiwrapper::Connection* occiwrapper::Connection::CreateConnection( shared_ptr< occiwrapper::Environment > pParmEnviroment, const ConnectionInfo& connInfo , UInt8 nType )
{
	return new Connection( pParmEnviroment, connInfo, nType );
}

oracle::occi::Environment* occiwrapper::Connection::GetEnvirnment()
{
	return this->m_pEnviroment->GetEnvirnment();
}

oracle::occi::Statement* occiwrapper::Connection::CreateStatement( const string strSql /*= "" */ , const string& strTag )
{
	try
	{
		oracle::occi::Statement* p = NULL;
		if( m_eValidity == VALID && m_pEnviroment->m_eValidity == VALID /*&& m_pOcciConnection*/ )
		{
			p = GetOcciConnection(strTag)->createStatement( strSql.c_str(), "" );
		}
		return p;
	}
	catch ( oracle::occi::SQLException exc )
	{
		stringstream ss;
		ss << "create statement failed," << exc.what();
		m_strErrMsg = ss.str();
	}
	catch( ... )
	{
		m_strErrMsg = "create statement failed, unknown exception.";
	}
	return NULL;
}

bool occiwrapper::Connection::TerminateStatement( oracle::occi::Statement* pOcciStat, const string& strTag)
{
	try
	{
		if( m_eValidity == VALID && m_pEnviroment->m_eValidity == VALID )
		{
			if( pOcciStat != NULL )
			{
				if (m_nConnectType == 0 && m_pOcciConnection)
				{
					this->m_pOcciConnection->terminateStatement( pOcciStat );
				}
				else
				{
					this->GetOcciConnection(strTag)->terminateStatement( pOcciStat);
				}
				return true;
			}
		}
	}
	catch ( oracle::occi::SQLException exc )
	{
		stringstream ss;
		ss << "terminate statement failed," << exc.what();
		m_strErrMsg = ss.str();
	}
	catch( ... )
	{
		m_strErrMsg = "terminate statement failed, unknown exception.";
		
	}
	return false;
	
}

bool occiwrapper::Connection::Commit(const string& strTag)
{
	try
	{
		if( m_eValidity == VALID && m_pEnviroment->m_eValidity == VALID /*&& m_pOcciConnection*/ )
		{
			if (m_nConnectType == 0)
			{
				this->m_pOcciConnection->commit();
			}
			else
			{
				oracle::occi::Connection* p = nullptr;
				p = this->GetOcciConnection(strTag);
				p->commit();
			}
			return true;
		}
	}
	catch ( oracle::occi::SQLException exc )
	{
		stringstream ss;
		ss << "commit failed, failed," << exc.what();
		m_strErrMsg = ss.str();
		return false;
	}
	catch( ... )
	{
		m_strErrMsg = "commit failed, unknown exception.";
	}
	return false;
}

bool occiwrapper::Connection::Rollback(const string& strTag)
{
	try
	{
		if( m_eValidity == VALID && m_pEnviroment->m_eValidity == VALID /*&& m_pOcciConnection*/ )
		{
			if (m_nConnectType == 0)
			{
				this->m_pOcciConnection->rollback();
			}
			else
			{
				this->GetOcciConnection(strTag)->rollback();
			}
		}
		return true;
	}
	catch ( oracle::occi::SQLException exc )
	{
		stringstream ss;
		ss << "rollback failed," << exc.what();
		m_strErrMsg = ss.str();
		return false;
	}
	catch( ... )
	{
		m_strErrMsg = "rollback failed, unknown exception.";
	}
	return false;
}

occiwrapper::Connection::~Connection()
{
	//close occiConnection
	if( m_eValidity == VALID && m_pEnviroment->m_eValidity == VALID /*&& m_pOcciConnection*/ )
	{
		try
		{
			if (m_nConnectType == 0)
			{
				m_pEnviroment->m_pOcciEnviroment->terminateConnection( this->m_pOcciConnection );
				this->m_eValidity = INVALID;
				this->m_pOcciConnection = NULL;
			}
			else
			{
				m_pEnviroment->m_pOcciEnviroment->terminateStatelessConnectionPool( this->m_pOcciStatelessConnection);
				this->m_eValidity = INVALID;
				this->m_pOcciConnection = NULL;
			}

		}
		catch (oracle::occi::SQLException exc)
		{
			stringstream ss;
			ss << "terminate connection failed," << exc.what();
			m_strErrMsg = ss.str();
		}
		catch( ... ) 
		{
			m_strErrMsg = "terminate connection unknown exception";
		}
	}
}

occiwrapper::Validity occiwrapper::Connection::GetValidity()
{
	return this->m_eValidity;
}

string occiwrapper::Connection::GetErrMsg()
{
	return m_strErrMsg;
}

occiwrapper::Connection::Connection( shared_ptr< occiwrapper::Environment >& pParmEnviroment, const ConnectionInfo& connInfo , UInt8 nType)
{
	m_pOcciConnection = nullptr;
	m_nConnectType = nType;
	this->m_eValidity = INVALID;
	m_objConnInfo = connInfo;
	this->m_pEnviroment = shared_ptr< occiwrapper::Environment >( pParmEnviroment );
	if( m_pEnviroment->m_pOcciEnviroment != NULL )
	{
		stringstream ss;
		ss<< m_objConnInfo.ip << ":" << m_objConnInfo.port << "/" << m_objConnInfo.sid;
		try
		{
			if (nType == 0)
			{
				m_pOcciConnection = m_pEnviroment->m_pOcciEnviroment->createConnection( m_objConnInfo.username,
					m_objConnInfo.password, ss.str() );

				if( m_pOcciConnection )
				{
					this->m_eValidity = VALID;
				}
			}			
			else
			{
				m_pOcciStatelessConnection = m_pEnviroment->m_pOcciEnviroment->createStatelessConnectionPool( m_objConnInfo.username, 
					m_objConnInfo.password, ss.str(), 40,10,1,oracle::occi::StatelessConnectionPool::HOMOGENEOUS );

				if( m_pOcciStatelessConnection )
				{
					this->m_eValidity = VALID;
				}
			}
		}
		catch( oracle::occi::SQLException exc )
		{
			stringstream ss;
			ss << "connection database," << m_objConnInfo.GetHashString() << " failed," << exc.what();
			m_strErrMsg = ss.str();
		}
		catch( ... )
		{
			stringstream ss;
			ss << "connection database," << m_objConnInfo.GetHashString() << " failed, unknown exception";
			m_strErrMsg = ss.str();
		}
	}
}

oracle::occi::Connection* occiwrapper::Connection::GetOcciConnection(const string& strTag)
{
	if (m_nConnectType == 1)
	{
		oracle::occi::Connection* p = nullptr;
		try
		{
			map<string, oracle::occi::Connection*>::iterator iter = m_mapConnection.find(strTag);
			if (iter == m_mapConnection.end())
			{
				p = m_pOcciStatelessConnection->getConnection();
				m_mapConnection.insert(make_pair(strTag, p));			
			}
			else
			{
				p = m_mapConnection[strTag];
			}
		}
		catch( oracle::occi::SQLException exc )
		{
			stringstream ss;
			ss << "connection database," << m_objConnInfo.GetHashString() << " failed," << exc.what();
			m_strErrMsg = ss.str();

			throw exc;
		}
		catch( ... )
		{
			stringstream ss;
			ss << "connection database," << m_objConnInfo.GetHashString() << " failed, unknown exception";
			m_strErrMsg = ss.str();

			throw UnknownException( ss.str() );
		}
		return 	p;
	}
	else
	{
		return m_pOcciConnection;
	}
}

