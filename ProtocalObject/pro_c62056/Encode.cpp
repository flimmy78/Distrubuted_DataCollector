/***********************************************************************************
EnConde.cpp
基本数据类型的62056帧的 数据编码程序集

***********************************************************************************/

#include "Encode.h"
#include "char_func.h"


//int EnCodeConformance(AnsiString Conformance, unsigned char * lpDest)
//{
//	unsigned char * lptmp = lpDest;
//	unsigned char Header[4] = {0x5F, 0x1F, 0x04, 0x00};
//	memcpy(lptmp, Header, 4 * sizeof(unsigned char));
//	lptmp += 4;
//	*lptmp++ = HexToInt(Conformance.SubString(1,2));
//
//	*lptmp++ = HexToInt(Conformance.SubString(3,2));
//
//	*lptmp = HexToInt(Conformance.SubString(5,2));
//
//	return 7;
//}
//
//int EncodeIdAndPri(int Invoke_Id,int Priority,unsigned char *lpDest)
//{
//	    unsigned char *lptmp = lpDest;
//  
//        unsigned char result = 0x20;//既然有响应，那么此次的Service_Class = 1
//        result = (Invoke_Id & 0x07) << 5;
//        result += Priority & 0x01;
//  
//        *lptmp = result;
//        return 1;
//};
//
//int EnCodeObjectIdentifier(struct Object_Identifier O_I, unsigned char * lpDest)
//{
//	/*
//	us jic;
//	us c;
//	us cn;
//	us io;
//	us DLMS_UA;
//	us index;
//	us id; 
//	*/
//	if(O_I.jic != 2 || O_I.c != 16 || O_I.cn != 756 || O_I.io != 5 || O_I.DLMS_UA != 8)
//	{
//	        return 0;	
//	}
//	unsigned char * lptmp = lpDest;
//	
//	*lptmp++ = 0x60;// O_I.jic * 2 + O_I.c;
//	*lptmp++ = 0x85;//( O_I.cn >> 7 ) & 0x7F + 0x80;
//	*lptmp++ = 0x74;//O_I.cn & 0x7F + 0x80;
//	*lptmp++ = 0x05;//O_I.io;
//	*lptmp++ = 0x08;//O_I.DLMS_UA;
//	*lptmp++ = O_I.index;
//	*lptmp = O_I.id;
//	return 7;
//}
//// encode the cosme object descripter
//int EnCodeObjectDescriptor(unsigned short ClassID, AnsiString OBIS,
//                            unsigned char AttributeID, unsigned char * lpDest)
//{
//	unsigned char * lptmp = lpDest;
//
//	lptmp += EnCodeSizedUnsigned(ClassID, 2, lptmp);
//
//	lptmp += EnCodeSizedOctetStr(OBIS, 6, lptmp );
//
//	EnCodeSizedInteger(AttributeID, 1, lptmp);
//
//	return 9;
//}
//
//
//// encode the logical name of cosem object
//int EnCodeOBIS(unsigned char * lpDest, AnsiString OBIS)
//{
//	unsigned char * lptmp = lpDest;
//	*lptmp++ = 0x09;
//	*lptmp++ = 0x06;
//	lptmp += EnCodeSizedOctetStr( OBIS, 6, lptmp);
//	return (lptmp - lpDest);
//}
//
int BNU(unsigned int Length)//Byte Number of Unsigned
{
	int i = 0;
	do
	{
		Length = Length >> 8;
		i++;
	}while(Length);
	//while((Length = (Length >> 8))) i++;
	return i;
}

// encode Lenth of Data
int EnCodeLength(unsigned int Length, unsigned char *lpDest)
{
	int ByteNum = BNU(Length);

	unsigned char *lptmp = lpDest;

	if(Length <= 127)
	{
		*lptmp = Length;
		return 1;
	}
	else
	{
		*lptmp++ =(( ByteNum & 0x7F) | 0x80);

		for(int i = ByteNum - 1; i >= 0; i --)
		{
			*lptmp++ = Length >> (8 * i);
		}
		return (ByteNum + 1);
	}
};
//
//
//// encode the Data Type of  Array 
//int EnCodeArray(unsigned int ArrayNum, unsigned char *lpDest)
//{
//	
//	unsigned char *lptmp = lpDest;
//	*lptmp++ = 0x01;
//	lptmp += EnCodeLength(ArrayNum, lptmp);
//	return (lptmp - lpDest);
//}
//
//
//// encode the Data Type of Struct
//int EnCodeStruct(unsigned int NumOfElement, unsigned char *lpDest)
//{
//	unsigned char *lptmp = lpDest;
//	*lptmp++ = 0x02;
//	lptmp += EnCodeLength(NumOfElement, lptmp);
//	return (lptmp - lpDest);
//}
//
//
// encode the Data Type of  Sized Bits
int EnCodeSizedBit(std::string BitStr, unsigned int BitNum,
                              unsigned char *lpDest)//BitStr为16进制表示的bit串
{
	unsigned char * lptmp = lpDest;
	
	int ByteNum = BitNum / 8 + (BitNum % 8 > 0 ? 1 : 0);
	
	for(int i = 1; i < ByteNum * 2; i = i + 2 )  
	{                                                                          
		*lptmp++ = HexToInt(BitStr.substr(i, 2));
    }   
		                                                                      
	return ByteNum;
} 


//encode the Data Type of Sizeable Bits
int EnCodeBit(std::string BitStr, unsigned int BitNum, unsigned char *lpDest)
{                  
  unsigned char * lptmp = lpDest;           
                                            
  lptmp += EnCodeLength(BitNum, lptmp);   

  lptmp += EnCodeSizedBit(BitStr, BitNum, lptmp);
	
  return (lptmp - lpDest);
}
//
//
// encode teh Data Type of  Sizeable octet-string, HEX
int EnCodeOctetStr(std::string OctetStr, unsigned char *lpDest)
{
	unsigned char * lptmp = lpDest;

    unsigned int OctetNum = OctetStr.length() / 2;

	lptmp += EnCodeLength(OctetNum, lptmp);
	
	lptmp += EnCodeSizedOctetStr(OctetStr, OctetNum, lptmp);
	
	return (lptmp - lpDest);
}


// encode teh Data Type of  Sized octet-string, HEX
int EnCodeSizedOctetStr(std::string OctetStr, unsigned int OctetNum, unsigned char *lpDest)
{
	unsigned char * lptmp = lpDest;
	
	for(unsigned int i = 1; i < OctetNum * 2; i = i + 2)               
    {
        *lptmp++ = HexToInt(OctetStr.substr(i, 2));
    }
        
	return OctetNum;
}

// encode teh Data Type of  Sizeable visible-string, char
int EnCodeVisibleStr(std::string VisibleStr, unsigned char *lpDest)
{
	unsigned char * lptmp = lpDest;

    unsigned int OctetNum = VisibleStr.length();

	lptmp += EnCodeLength(OctetNum, lptmp);
	
	lptmp += EnCodeSizedVisibleStr(VisibleStr, OctetNum, lptmp);
	
	return (lptmp - lpDest);
}


// encode teh Data Type of  Sized visible-string, char
int EnCodeSizedVisibleStr(std::string VisibleStr, unsigned int OctetNum, unsigned char *lpDest)
{
	unsigned char * lptmp = lpDest;
    unsigned char* cp = (unsigned char*)VisibleStr.c_str();

    memcpy((char *)lptmp, (char *)cp, OctetNum);
	return OctetNum;
}

// encode teh Data Type of  sized integer
int EnCodeSizedInteger(long long int Value, unsigned int ByteNum, unsigned char *lpDest)
{
	unsigned char *lptmp = lpDest;

	//if( Value <= 4294967295 && ByteNum <= 4 )
	//{
		unsigned char* Vv = (unsigned char*)&Value;
                //if (ByteNum == 8)
                //{
		char szBuf[64] = {0};
		sprintf_s(szBuf, "%016x", (unsigned int)Value);
        std::string vs = (char*)szBuf;
                //}

		switch(ByteNum)
		{
			case 1: 
                            *lptmp = *Vv;
                            return 1;
			case 2:
                            lptmp[0] = Vv[1];
                            lptmp[1] = Vv[0];
                            return 2;
                        case 4:
                            lptmp[0] = Vv[3];
                            lptmp[1] = Vv[2];
                            lptmp[2] = Vv[1];
                            lptmp[3] = Vv[0];
                            return 4;
                        case 8:
                            //lptmp[0] = Vv[7];
                            //lptmp[1] = Vv[6];
                            //lptmp[2] = Vv[5];
                            //lptmp[3] = Vv[4];
                            //lptmp[4] = Vv[3];
                            //lptmp[5] = Vv[2];
                            //lptmp[6] = Vv[1];
                            //lptmp[7] = Vv[0];
                            lptmp[0] = (unsigned char)strtol(vs.substr(1, 2).c_str(), NULL, 16);
                            lptmp[1] = (unsigned char)strtol(vs.substr(3, 2).c_str(), NULL, 16);
                            lptmp[2] = (unsigned char)strtol(vs.substr(5, 2).c_str(), NULL, 16);
                            lptmp[3] = (unsigned char)strtol(vs.substr(7, 2).c_str(), NULL, 16);
                            lptmp[4] = (unsigned char)strtol(vs.substr(9, 2).c_str(), NULL, 16);
                            lptmp[5] = (unsigned char)strtol(vs.substr(11, 2).c_str(), NULL, 16);
                            lptmp[6] = (unsigned char)strtol(vs.substr(13, 2).c_str(), NULL, 16);
                            lptmp[7] = (unsigned char)strtol(vs.substr(15, 2).c_str(), NULL, 16);
                            return 8;
                        default:
                            return 0;
                }
              //}
                    //else
                    //{
                    //	return 0;
                    //}
};

int EnCodeSizedUnsigned(__int64 Value, unsigned int ByteNum, unsigned char *lpDest)
{
  unsigned char* Vv = (unsigned char*)&Value;

  switch(ByteNum)
  {
  	case 1: 
  		*lpDest = *Vv;
			return 1;
    case 2: 
    	lpDest[0] = Vv[1];
      lpDest[1] = Vv[0];
      return 2;
    case 4: lpDest[0] = Vv[3];
    	lpDest[1] = Vv[2];
      lpDest[2] = Vv[1];
      lpDest[3] = Vv[0];
      return 4;
    default:
    	return 0;
  }
};

int EnCodeBCD(unsigned char Value, unsigned char *lpDest)
{
  return 0;
}

int EnCodeFloat32(float Value, unsigned char *lpDest)
{
	unsigned char* Vv = (unsigned char*)&Value;
        lpDest[0] = Vv[3];
        lpDest[1] = Vv[2];
        lpDest[2] = Vv[1];
        lpDest[3] = Vv[0];
        return 4;
}



int EnCodeFloat64(double Value, unsigned char *lpDest)
{
    unsigned char* Vv = (unsigned char*)&Value;
        lpDest[0] = Vv[7];
        lpDest[1] = Vv[6];
        lpDest[2] = Vv[5];
        lpDest[3] = Vv[4];
        lpDest[4] = Vv[3];
        lpDest[5] = Vv[2];
        lpDest[6] = Vv[1];
        lpDest[7] = Vv[0];
        return 8;
}
//
//
int EnCodeDateTime(std::string DtTm, unsigned char *lpDest)
{
	return  EnCodeSizedOctetStr(DtTm,12,lpDest);
}

int EnCodeDate(std::string Dt, unsigned char *lpDest)
{
 	return  EnCodeSizedOctetStr(Dt,5,lpDest);
}


int EnCodeTime(std::string Tm, unsigned char *lpDest)
{
	return  EnCodeSizedOctetStr(Tm,4,lpDest);
}


