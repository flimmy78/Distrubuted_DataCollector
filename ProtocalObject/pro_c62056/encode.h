//---------------------------------------------------------------------------

#ifndef EncodeH
#define EncodeH

#include <string>
//#include "StructDef.h"
//#include "Func.h"
/***********************************************************************************
EnConde.h
基本数据类型的62056帧的 数据编码程序集

***********************************************************************************/

//int EnCodeConformance(AnsiString Conformance, unsigned char * lpDest);
//
//int EncodeIdAndPri(int Invoke_Id,int Priority,unsigned char *lpDest);
//
//int EnCodeObjectIdentifier(struct Object_Identifier O_I, unsigned char * lpDest);
//
//// encode the cosme object descripter
//int  EnCodeObjectDescriptor(unsigned short ClassID, AnsiString OBIS, 
//                            unsigned char AttributeID, unsigned char * lpDest);
//
//// encode the logical name of cosem object
//int EnCodeOBIS(unsigned char * lpDest, const char *const OBIS);

int BNU(unsigned int Length);//Byte Number of Unsigned

// encode Lenth of Data
int EnCodeLength(unsigned int Length, unsigned char *lpDest);


////encode the Data Type of  Array
//int EnCodeArray(unsigned int ArrayNum, unsigned char *lpDest);
//
////encode the Data Type of Struct
//int EnCodeStruct(unsigned int NumOfElement, unsigned char *lpDest);
//
//encode the Data Type of  Sized Bits
int EnCodeSizedBit(std::string BitStr, unsigned int BitNum, unsigned char *lpDest);
//
//encode the Data Type of Sizeable Bits
int EnCodeBit(std::string BitStr, unsigned int BitNum, unsigned char *lpDest);

//encode teh Data Type of  Sizeable octet-string
int EnCodeOctetStr(std::string OctetStr, unsigned char *lpDest);

//encode teh Data Type of  Sized octet-string
int EnCodeSizedOctetStr(std::string OctetStr, unsigned int OctetNum, unsigned char *lpDest);


int EnCodeVisibleStr(std::string VisibleStr, unsigned char *lpDest);

// encode teh Data Type of  Sized visible-string, char
int EnCodeSizedVisibleStr(std::string VisibleStr, unsigned int OctetNum, unsigned char *lpDest);
//encode teh Data Type of  sized integer
int EnCodeSizedInteger(long long int Value, unsigned int ByteNum, unsigned char *lpDest);

int EnCodeSizedUnsigned(__int64 Value, unsigned int ByteNum, unsigned char *lpDest);

int EnCodeBCD(unsigned char Value, unsigned char *lpDest);

int EnCodeFloat32(float Value, unsigned char *lpDest);
//
int EnCodeFloat64(double Value, unsigned char *lpDest);
//
//
int EnCodeDateTime(std::string DtTm, unsigned char *lpDest);

int EnCodeDate(std::string Dt, unsigned char *lpDest);

int EnCodeTime(std::string Tm, unsigned char *lpDest);


#endif
