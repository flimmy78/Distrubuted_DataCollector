#pragma once

#include <string>
#include <vector>
#include <list>
#include <time.h>
#include <map>
using namespace std;
#define DBINTERFACE class

#ifdef WIN32
#ifndef DB_DLL_IMPORT
#define __DB_INTERFACE_API __declspec(dllexport)
#else
#define __DB_INTERFACE_API __declspec(dllimport)
#ifdef _DEBUG
#pragma comment( lib, "DBObjectd.lib")
#else
#pragma comment( lib, "DBObject.lib")
#endif
#endif
#else
#define __DB_INTERFACE_API
#endif

/***********************************************************************************************************
remark

	@ 每一个进程创建一个对象，一个DBOBject对象对应一个Environment对象

	@ 一个DBOBJect对象根据传入的用户名密码数据库名的不同，可以对应多个连接

	@ 一个大连接(connectpool)可以包含多个小连接(connect).

	@ 每一个大连接根据DBTag来分类，OPENDB可以调用多次

	@ 每一个小连接根据tag来分类，这个tag随机产生，但值唯一

	@ 关闭数据库时，先关闭小连接，再关闭大连接

	@ 一个Envirment 一个connect（内部创建连接池），多个session(session就是con线程池中的子链接)


							 Environment
				 ------------------------------------
						/         |         \
				 ------------------------------------
					 con         con       con
				   /  |  \     /  |  \   /  |  \
				 ------------------------------------
				 ses ses ses  ses ses ses ses ses ses

**************************************************************************************************************/

namespace WSDBInterFace
{
#if defined(WIN32) || defined(WIN64)
		//
		// Windows/Visual C++
		//
		typedef signed char            Int8;
		typedef unsigned char          UInt8;
		typedef signed short           Int16;
		typedef unsigned short         UInt16;
		typedef int					   Int32;
		typedef unsigned int           UInt32;
		typedef signed __int64         Int64;
		typedef unsigned __int64       UInt64;
#else
		//
		// Unix/GCC
		//
		typedef signed char            Int8;
		typedef unsigned char          UInt8;
		typedef signed short           Int16;
		typedef unsigned short         UInt16;
		typedef int					   Int32;
		typedef unsigned int           UInt32;
		typedef signed long            IntPtr;
		typedef signed long long	   Int64;
		typedef unsigned long long	   UInt64;
#endif

	DBINTERFACE IDBObject
	{
		public:
		/*	打开数据库
		*	@strDBName: 数据库名
		*	@strUserName: 登录用户名
		*	@strPassWord: 登录密码
		*	@strIP: 数据库IP地址
		*	@nPort: 数据库端口
		*	@strErrOut：函数返回失败后的错误信息
		*/
		virtual bool OpenDB(const string& strDBName, const string& strUserName, 
			const string& strPassWord, const string& strIP, 
			UInt32 nPort, string& strDBTagOut, string& strErrOut) = 0 ;

		/*	关闭数据库
		*	当所有连接全关闭时，退出数据库环境
		*	@strErrOut：函数返回失败后的错误信息
		*/
		virtual bool CloseDB(const string& strDBTag, string& strErrOut) = 0;

		/*	批量数据插入
		*	@strTableName：表名
		*	@strColsName: 列名集合,列名之间用逗号隔开
		*	(such as "string_value, date_value, int_value, float_value, number_value")
		*	@nColNum：一共有多少列
		*	@strTagOut：标识一个数据库连接，传出参数
		*	@strTag: 标识一个数据库连接，等于strTagOut
		*	下面三个函数，为一组函数，不能单独使用，使用规则如下：
		*
		*	BeginBatchInsert();
		*	for (int i = 0; i< nColNum; i++)
		*		SetInsertColData();
		*	EndBatchInsert();
		*	执行完成之后，数据库环境即失效
		*/
		virtual bool BeginBatchInsert(const string& strDBTag, const string& strTableName, 
			const string& strColsName, UInt32 nColNum, string& strTagOut) = 0;

		virtual int SetInsertColData(const vector<Int32>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int SetInsertColData(const vector<Int64>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int SetInsertColData(const vector<Int16>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int SetInsertColData(const vector<Int8>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int SetInsertColData(const vector<UInt32>& vecCol, const string& strDBTag, const string& strTag)	= 0;
		virtual int SetInsertColData(const vector<UInt64>& vecCol, const string& strDBTag, const string& strTag)	= 0;
		virtual int SetInsertColData(const vector<UInt16>& vecCol, const string& strDBTag, const string& strTag)	= 0;
		virtual int SetInsertColData(const vector<UInt8>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int SetInsertColData(const vector<string>& vecCol, const string& strDBTag, const string& strTag)	= 0;
		virtual int SetInsertColData(const vector<char*>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int SetInsertColData(const vector<float>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int SetInsertColData(const vector<double>& vecCol, const string& strDBTag, const string& strTag)	= 0;
		virtual int SetInsertColData(const vector<bool>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int SetInsertColData(const vector<struct tm>& vecCol, const string& strDBTag, const string& strTag)	= 0;

		virtual bool EndBatchInsert(const string& strDBTag, const string& strTag, string& strErrOut) = 0;

		/*	数据读取
		*	@strTableName：表名
		*	@strColsName: 列名集合,列名之间用逗号隔开
		*	(such as "string_value, date_value, int_value, float_value, number_value")
		*	@nColNum：一共有多少列
		*	@strTagOut：标识一个数据库连接，传出参数
		*	@strTag: 标识一个数据库连接，等于strTagOut
		*
		*	下面三个函数，为一组函数，不能单独使用，使用规则如下：
		*
		*	BeginRead();
		*	for (int i = 0; i< nColNum; i++)
		*		SetReadColData();
		*	EndRead();
		*	执行完成之后，数据库环境即失效
		*/
		virtual bool BeginBatchRead(const string& strDBTag, const string& strSql, UInt32 nColNum, string& strTagOut)	= 0;

		virtual int SetReadColData(vector<Int32>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<Int64>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<Int16>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<Int8>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<UInt32>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<UInt64>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<UInt16>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<UInt8>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<string>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<char*>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<float>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<double>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<bool>& vecCol, const string& strDBTag, const string& strTag)				= 0;
		virtual int SetReadColData(vector<struct tm>& vecCol, const string& strDBTag, const string& strTag)			= 0;

		virtual bool ExcuteBatchRead(const string& strDBTag, const string& strTag, string& strErrOut)					= 0;
		virtual bool EndBatchRead(const string& strDBTag, const string& strTag, string& strErrOut)						= 0;

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
		*		AddParamIn();
		*	AddParamOut() 输入参数可以有多个，但是输出只能一个
		*	for (int i = 0; i< nColNum; i++)
		*		ReadProcedureResult();
		*
		*	ExcuteProcedure();
		*	EndProcedure();
		*	执行完成之后，数据库环境即失效
		*/
		virtual bool BeginProcedure(const string& strDBTag, const string& strProcedureName, string& strTagOut,
			unsigned int nParamNum = 0, unsigned int nColNum = 0, bool bHasCursor = false) = 0;

		virtual int AddParamIn(const Int32& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const Int64& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const Int16& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const Int8& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamIn(const UInt32& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const UInt64& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const UInt16& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const UInt8& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const string& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const char* Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamIn(const float& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const double& Param, const string& strDBTag, const string& strTag)					= 0;
		virtual int AddParamIn(const bool& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamIn(const struct tm& time, const string& strDBTag, const string& strTag)					= 0;

		virtual int AddParamOut(Int32& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamOut(Int64& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamOut(UInt32& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamOut(UInt64& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamOut(string& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamOut(float& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamOut(double& Param, const string& strDBTag, const string& strTag)						= 0;
		virtual int AddParamOut(const string& strDBTag, const string& strTag)										= 0;

		virtual int ReadProcedureResult(vector<Int32>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<Int64>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<Int16>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<Int8>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<UInt32>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<UInt64>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<UInt16>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<UInt8>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<string>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<char*>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<float>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<double>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<bool>& vecCol, const string& strDBTag, const string& strTag)		= 0;
		virtual int ReadProcedureResult(vector<struct tm>& vecCol, const string& strDBTag, const string& strTag)	= 0;

		virtual bool ExcuteProcedure(const string& strDBTag, const string& strTag, string& strErrOut)				= 0;
		virtual bool EndProcedure(const string& strDBTag, const string& strTag, string& strErrOut)					= 0;

		/* 执行一条SQL语句
		*
		*	BeginExcuteSQL(@数据库标识, @返回的Session标识) 创建一个Session
		*
		*	ExcuteSQL(@数据库标识, @返回的Session标识, @SQL语句, @错误返回值) 通过这个Session反复执行不同的SQL语句
		*
		*	EndExcuteSQL(@数据库标识, @返回的Session标识)  释放对应的Seesion
		*/
		virtual bool BeginExcuteSQL(const string& strDBTag, string& strTagOut)												= 0;		
		virtual bool ExcuteSQL(const string& strDBTag, const string& strTag, const string& strSQL, string& strErrOut)		= 0;
		virtual bool EndExcuteSQL(const string& strDBTag, const string& strTag)												= 0;


		/* 获取表的列序号以及对应的列名
		*
		*	BeginGetTableProperty(@数据库标识, @返回的Session标识) 创建一个Session
		*
		*	GetTablePropertry(@数据库标识, @返回的Session标识, @SQL语句, @错误返回值) 通过这个Session反复执行
		*
		*	EndGetTableProperty(@数据库标识, @返回的Session标识)  释放对应的Seesion
		*/
		virtual bool BeginGetTableProperty(const string& strDBTag, string& strTagOut) = 0;
		virtual bool ExcuteGetTablePropertry(const string& strDBTag, const string& strTag, const string& strTableName,
			map<UInt8,string>& mapTablePro, string& strErrOut) = 0;
		virtual bool EndGetTableProperty(const string& strDBTag, const string& strTag) = 0;
	};

	/* 创建数据库操作对象 */
	enum DBType
	{
		ORCAL_TYPE = 0,
		SQLSERVER_TYPE = 1,
	};

	__DB_INTERFACE_API bool CreateDBObject(DBType type, IDBObject** pDBObject);
};


