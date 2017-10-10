#pragma once

#include "OcciWrapper/Common.h"
#include "OcciWrapper/Connection.h"
#include "OcciWrapper/ConnectionInfo.h"
#include "OcciWrapper/ConnectionPool.h"
#include "OcciWrapper/StatementImpl.h"
#include "OcciWrapper/Statement.h"

#define into( x ) occiwrapper::into( x )

namespace occiwrapper
{
	class __OCCI_WRAPPER_API Session
	{
	public:
		/***
		*	@brief: Creates a new session, using the given connector 
		*	@add by CUCmehp.
		*/
		Session( shared_ptr< occiwrapper::Connection > pConnection, bool bAutoCommit, const string& strTag = "", bool bHasCursor = false);

		/***
		*	@brief: destroy instance
		*	@add by CUCmehp, since 2012-07-15
		*/
		virtual ~Session();
		
		/***
		*	@brief: Creates a Statement with the given data as SQLContent
		*	@add by CUCmehp
		*/
		template <typename T>
		Statement operator << (const T& t)
		{
			if (m_pStatement != nullptr)
				*m_pStatement << t;
			return *m_pStatement;
		}

		Statement& operator , (occiwrapper::Statement::Manipulator manip)
		{
			return *m_pStatement,manip;
		}

		Statement& operator , ( bool& bSuccessed )
		{
			if (m_pStatement != nullptr) 
				*m_pStatement,bSuccessed;
			return *m_pStatement;
		}

		Statement& operator , ( string& strErrorMsg )
		{
			if (m_pStatement != nullptr) 
				*m_pStatement, strErrorMsg;
			return *m_pStatement;
		}

		// Registers the Binding at the Statement
		Statement& operator , (AbstractBinding* info)
		{
			if (m_pStatement != nullptr)
				*m_pStatement,info;
			return *m_pStatement;
		}
		
		// Registers objects used for extracting data at the Statement.
		Statement& operator , ( AbstractExtraction* extract )
		{
			if (m_pStatement != nullptr) 
				*m_pStatement,extract;
			return *m_pStatement;
		}

		// Sets a limit on the maximum number of rows a select is allowed to return.
		// Set per default to Limit::LIMIT_UNLIMITED which disables the limit.
		Statement& operator , (const Limit& extrLimit)
		{
			if (m_pStatement != nullptr)
				*m_pStatement, extrLimit;
			return *m_pStatement;
		}

		/***
		*	@brief: Creates a StatementImpl
		*	@add by CUCmehp
		*/
		shared_ptr< StatementImpl > CreateStatementImpl();

		/***
		*	@brief: Commits and ends a transaction.
		*	@add by CUCmehp
		*/
		void Commit();

		/***
		*	@brief: Rolls back and ends a transaction.
		*/
		void Rollback();

		/***
		*	@brief: set auto commit
		*	@add by CUCmehp
		*/
		void SetAutoCommit( bool isAutoCommit );

		/***
		*	@brief: Session is valid or not
		*	@return:
		*		if session is valid, return true, else return false
		*/
		inline bool IsValid()
		{
			return m_pConnection != NULL;
		}

	private:
		Session();

		//hold the oracle connection
		occiwrapper::Statement*	m_pStatement;		
		shared_ptr< occiwrapper::Connection >	m_pConnection;
		bool									m_bAutoCommit;
		string									m_strTag;
		bool									m_bHasCursor;
	};
} 

