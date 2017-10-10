#pragma once

#include <map>
#include <random>
#ifdef WIN32
#include <windows.h>
#endif

#include "../include/DBInterFace.h"
using namespace WSDBInterFace;
#include "OcciWrapper/OcciWrapper.h"

/**************************************************************************
*
*	@ oracle����OCCI�ӿ�ʵ�֣����Բ�������oracle�ͻ��˶������С�
*
*	@ ÿһ�����̴���һ������һ��DBOBject�����Ӧһ��Environment����
*
*	@ һ��DBOBJect������ݴ�����û����������ݿ����Ĳ�ͬ�����Զ�Ӧ������ӡ�
*
*	@ һ��������(connectpool)���԰������С����(connect)��
*
*	@ ÿһ�������Ӹ���DBTag�����࣬OPENDB���Ե��ö�Ρ�
*
*	@ ÿһ��С���Ӹ���tag�����࣬���tag�����������ֵΨһ��
*
*	@ �ر����ݿ�ʱ���ȹر�С���ӣ��ٹرմ����ӡ�
*
*	@ һ��Envirment һ��connect���ڲ��������ӳأ������session��
*
*				                                                                   
*				             Environment
*				 ------------------------------------
*						/         |         \
*				 ------------------------------------
*				     con         con       con                     con->connect                                            
*				   /  |  \     /  |  \   /  |  \       
*				 ------------------------------------
*				 ses ses ses  ses ses ses ses ses ses              ses->session                                      
*				                                                                       
***************************************************************************/

class COrcalObject : public IDBObject
{
public:
	COrcalObject();
	virtual ~COrcalObject();

	/*	�����ݿ�
	*	strDBName: ���ݿ���
	*	strUserName: ��¼�û���
	*	strPassWord: ��¼����
	*	strIP: ���ݿ�IP��ַ
	*	nPort: ���ݿ�˿�
	*	strErrOut����������ʧ�ܺ�Ĵ�����Ϣ
	*/
	virtual bool OpenDB(const string& strDBName, const string& strUserName, 
		const string& strPassWord, const string& strIP, 
		unsigned int nPort, string& strDBTagOut, string& strErrOut) override;

	/*	�ر����ݿ�
	*	strErrOut����������ʧ�ܺ�Ĵ�����Ϣ
	*/
	virtual bool CloseDB(const string& strDBTag, string& strErrOut) override;

	/*	�������ݲ���
	*	strTableName������
	*	strColsName: ��������,����֮���ö��Ÿ���
	*	(such as "string_value, date_value, int_value, float_value, number_value")
	*	nColNum��һ���ж�����
	*
	*	��������������Ϊһ�麯�������ܵ���ʹ�ã�ʹ�ù������£�
	*
	*	BeginBatchInsert();
	*	for (int i = 0; i< nColNum; i++)
	*		SetInsertColData();
	*	EndBatchInsert();
	*	ִ�����֮�����ݿ⻷����ʧЧ
	*/
	virtual bool BeginBatchInsert(const string& strDBName, const string& strTableName, 
		const string& strColsName, unsigned int nColNum, string& strTagOut) override;

	virtual int SetInsertColData(const vector<Int32>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<Int64>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<Int16>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<Int8>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<UInt32>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<UInt64>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<UInt16>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<UInt8>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<string>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<char*>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<float>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<double>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<bool>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetInsertColData(const vector<struct tm>& vecCol, const string& strDBTag, const string& strTag) override;

	virtual bool EndBatchInsert(const string& strDBTag, const string& strTag, string& strErrOut) override;

	/*	���ݶ�ȡ
	*	strTableName������
	*	strColsName: ��������,����֮���ö��Ÿ���
	*	(such as "string_value, date_value, int_value, float_value, number_value")
	*	nColNum��һ���ж�����
	*
	*	��������������Ϊһ�麯�������ܵ���ʹ�ã�ʹ�ù������£�
	*
	*	BeginRead();
	*	for (int i = 0; i< nColNum; i++)
	*		SetReadColData();
	*	EndRead();
	*	ִ�����֮�����ݿ⻷����ʧЧ
	*/
	virtual bool BeginBatchRead(const string& strDBTag, const string& strSql, 
		unsigned int nColNum, string& strTagOut) override;

	virtual int SetReadColData(vector<Int32>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<Int64>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<Int16>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<Int8>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<UInt32>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<UInt64>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<UInt16>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<UInt8>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<string>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<char*>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<float>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<double>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<bool>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int SetReadColData(vector<struct tm>& vecCol, const string& strDBTag, const string& strTag) override;

	virtual bool ExcuteBatchRead(const string& strDBTag, const string& strTag, string& strErrOut) override;
	virtual bool EndBatchRead(const string& strDBTag, const string& strTag, string& strErrOut) override;

	/*	ִ�д洢����
	*	strTableName������
	*	strColsName: ��������,����֮���ö��Ÿ���
	*	(such as "string_value, date_value, int_value, float_value, number_value")
	*	nColNum���洢����
	*
	*	��������������Ϊһ�麯�������ܵ���ʹ�ã�ʹ�ù������£�
	*
	*	BeginProcedure();
	*	for (int j = 0; j< nParamNum; j++)
	*		AddParamIn();
	*	AddParamOut() ������������ж�����������ֻ��һ��
	*	for (int i = 0; i< nColNum; i++)
	*		ReadProcedureResult();
	*	EndProcedure();
	*	ִ�����֮�����ݿ⻷����ʧЧ
	*/
	virtual bool BeginProcedure(const string& strDBName, const string& strProcedureName, string& strTagOut, 
		unsigned int nParamNum = 0, unsigned int nColNum = 0, bool bHasCursor = false) override;

	virtual int AddParamIn(const Int32& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const Int64& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const Int16& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const Int8& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const UInt32& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const UInt64& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const UInt16& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const UInt8& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const string& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const char* Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const float& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const double& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const bool& Param, const string& strDBTag, const string& strTag) override;
	virtual int AddParamIn(const struct tm& time, const string& strDBTag, const string& strTag) override;

	virtual int AddParamOut(Int32& Param, const string& strDBTag, const string& strTag)		override;
	virtual int AddParamOut(Int64& Param, const string& strDBTag, const string& strTag)		override;
	virtual int AddParamOut(UInt32& Param, const string& strDBTag, const string& strTag)	override;
	virtual int AddParamOut(UInt64& Param, const string& strDBTag, const string& strTag)	override;
	virtual int AddParamOut(string& Param, const string& strDBTag, const string& strTag)	override;
	virtual int AddParamOut(float& Param, const string& strDBTag, const string& strTag)		override;
	virtual int AddParamOut(double& Param, const string& strDBTag, const string& strTag)	override;
	virtual int AddParamOut(const string& strDBTag, const string& strTag)	override;

	virtual int ReadProcedureResult(vector<Int32>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<Int64>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<Int16>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<Int8>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<UInt32>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<UInt64>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<UInt16>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<UInt8>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<string>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<char*>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<float>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<double>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<bool>& vecCol, const string& strDBTag, const string& strTag) override;
	virtual int ReadProcedureResult(vector<struct tm>& vecCol, const string& strDBTag, const string& strTag) override;

	virtual bool ExcuteProcedure(const string& strDBTag, const string& strTag, string& strErrOut) override;
	virtual bool EndProcedure(const string& strDBTag, const string& strTag, string& strErrOut) override;

	/* ִ��һ��SQL���*/
	virtual bool BeginExcuteSQL(const string& strDBTag, string& strTagOut) override;
	virtual bool ExcuteSQL(const string& strDBTag, const string& strTag, const string& strSQL, string& strErrOut) override;
	virtual bool EndExcuteSQL(const string& strDBTag, const string& strTag) override;

	/* ��ȡ���������Լ���Ӧ������
	*
	*	BeginGetTableProperty(@���ݿ��ʶ, @���ص�Session��ʶ) ����һ��Session
	*
	*	GetTablePropertry(@���ݿ��ʶ, @���ص�Session��ʶ, @SQL���, @���󷵻�ֵ) ͨ�����Session����ִ��
	*
	*	EndGetTableProperty(@���ݿ��ʶ, @���ص�Session��ʶ)  �ͷŶ�Ӧ��Seesion
	*/
	virtual bool BeginGetTableProperty(const string& strDBTag, string& strTagOut) override;
	virtual bool ExcuteGetTablePropertry(const string& strDBTag, const string& strTag, const string& strTableName,
		map<UInt8, string>& mapTablePro, string& strErrOut) override;
	virtual bool EndGetTableProperty(const string& strDBTag, const string& strTag) override;

private:
	bool m_bOpened;
	// UInt32,UInt32,UInt32,UInt32 = colnums,curcolnum,paramnums,curparanum
	map<string, tuple<shared_ptr<occiwrapper::Session>,UInt32,UInt32,UInt32,UInt32>> m_mapSessionBatchInsert;	 // �������ݲ���

	map<string, tuple<shared_ptr<occiwrapper::Session>, UInt32, UInt32, UInt32, UInt32>> m_mapSessionProcedure; // ����洢����

	map<string, tuple<shared_ptr<occiwrapper::Session>, UInt32, UInt32, UInt32, UInt32>> m_mapSessionBatchRead; // �������ݶ�ȡ

	map<string, tuple<shared_ptr<occiwrapper::Session>, UInt32, UInt32, UInt32, UInt32>> m_mapSessionSQL; // ����DDLSQL���

	map<string, shared_ptr<occiwrapper::Session>> m_mapSessionTablePro; // ����DDLSQL���

	map<string, shared_ptr<occiwrapper::Connection>> m_mapConnection;

	shared_ptr<occiwrapper::SessionFactory> m_sessionFactory;

#ifdef WIN32
	SRWLOCK	 m_srwSessionSQL;
	SRWLOCK	 m_srwSessionProcedure;
	SRWLOCK	 m_srwSessionBatchRead;
	SRWLOCK	 m_srwSessionBatchInsert;
	SRWLOCK	 m_srwSessionTablePro;
	SRWLOCK	 m_srwConnect;
#endif
};