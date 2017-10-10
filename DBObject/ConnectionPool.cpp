#include "OcciWrapper/ConnectionPool.h"
#include "OcciWrapper/Common.h"
#include <iostream>

occiwrapper::ConnectionPool::ConnectionPool( string strCharset, string strNCharset , UInt8 nType )
{
	m_pEnviroment = shared_ptr< occiwrapper::Environment >( occiwrapper::Environment::CreateEnvironment( strCharset, strNCharset ) );

	m_eValidity = m_pEnviroment.get() == nullptr ? INVALID : VALID;

	m_nPoolType = nType;
}

occiwrapper::ConnectionPool::~ConnectionPool()
{
	// auto free connection point
	// because shared_ptr< Connection >
	this->m_mapConnection.clear();
}

shared_ptr< occiwrapper::Connection > occiwrapper::ConnectionPool::GetConnection( const occiwrapper::ConnectionInfo& connInfo )
{
	if (m_eValidity == INVALID)
	{
		return shared_ptr< Connection >();
	}

	//find in connectionMap
	string key = connInfo.GetHashString();
	map< string, shared_ptr< Connection > >::iterator it = m_mapConnection.find( key );
	if( it == m_mapConnection.end() )
	{
		shared_ptr< Connection > tempConn( Connection::CreateConnection( m_pEnviroment, connInfo, m_nPoolType ) );
		if( tempConn )
		{
			m_mapConnection.insert( make_pair( connInfo.GetHashString(), tempConn ) );
			return tempConn;
		}
		else
		{
			return shared_ptr< Connection >( );
		}
	}
	return it->second;
}

size_t occiwrapper::ConnectionPool::GetConnMapSize()
{
	return this->m_mapConnection.size();
}