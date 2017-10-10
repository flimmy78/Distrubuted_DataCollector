#include "OcciWrapper/SessionFactory.h"

occiwrapper::SessionFactory::SessionFactory( string strCharset, string strNCharset,  UInt8 nType )
	: m_strCharset( strCharset )
	, m_strNCharset( strNCharset )
	, m_objConnectionPool( strCharset, strNCharset, nType )
{
	
}

occiwrapper::SessionFactory::~SessionFactory()
{

}

shared_ptr< occiwrapper::Connection > occiwrapper::SessionFactory::CreateConnect( const ConnectionInfo& connInfo, bool bAutoCommit)
{
	if (m_objConnectionPool.GetConnectionStatus() != INVALID)
	{
		shared_ptr< occiwrapper::Connection > p = m_objConnectionPool.GetConnection(connInfo);
		return p;
	}
	else
	{
		return shared_ptr< occiwrapper::Connection >( );
	}
}
