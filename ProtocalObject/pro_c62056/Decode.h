//---------------------------------------------------------------------------

#ifndef DecodeH
#define DecodeH
//---------------------------------------------------------------------------
#include <string>
#include "char_func.h"
//#include "StructDef.h"
//#include "Func.h"

//int DeCodeConformance(unsigned char * Conformance, const unsigned char * lpSrc);
//
//int DeCodeObjectIdentifier(struct Object_Identifier *O_I, const unsigned char * lpSrc);
//// DeCode the cosme object descripter
//int DeCodeObject(unsigned short *ClassID, unsigned char *OBIS,
//                            unsigned char *AttributeID, const unsigned char *lpSrc);
//
//// DeCode the logical name of cosem object
//int DeCodeOBIS(unsigned char * OBIS, const unsigned char *lpSrc);


// DeCode Lenth of Data
int DeCodeLength(unsigned int *Length, const unsigned char *lpSrc);


// DeCode the Data Type of  Array 
int DeCodeArray(unsigned int *ArrayNum, const unsigned char *lpSrc);


// DeCode the Data Type of Struct
int DeCodeStruct(unsigned int *NumOfElement, const unsigned char *lpSrc);


int DeCodeSizedBit(std::string &BitStr, unsigned int BitNum, const unsigned char *lpSrc);
int DeCodeBit(std::string &BitStr, const unsigned char *lpSrc);


int DeCodeOctetStr(std::string &OctetStr, const unsigned char *lpSrc);
int DeCodeSizedOctetStr(std::string &OctetStr, unsigned int OctetNum, const unsigned char *lpSrc);

int DeCodeVisibleStr(std::string &VisibleStr, const unsigned char *lpSrc);

// DeCode teh Data Type of  sized integer
int DeCodeSizedInteger(__int64 *Value, unsigned int ByteNum, const unsigned char *lpSrc);

int DeCodeSizedInteger_S(__int64 *Value, unsigned int  ByteNum, const unsigned char *lpSrc);


int DeCodeSizedUnsigned(unsigned int *Value, unsigned int ByteNum, const unsigned char *lpSrc);

unsigned char DeCodeBCD(unsigned char *Value, const unsigned char *lpSrc);

int DeCodeFloat32(float *Value, const unsigned char *lpSrc);

int DeCodeFloat64(float *Value, const unsigned char *lpSrc);

int DeCodeDateTime(std::string &DtTm, const unsigned char *lpSrc);

int DeCodeDate(std::string &Dt, const unsigned char *lpSrc);

int DeCodeTime(std::string &Tm, const unsigned char *lpSrc);

#endif
