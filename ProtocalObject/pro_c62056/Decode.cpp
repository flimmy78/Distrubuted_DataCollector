

#include "Decode.h"


//
//int DeCodeConformance(unsigned char * Conformance, const unsigned char * lpSrc)
//{
//	unsigned char Temp[3];
//	if(*lpSrc == 0x5F && *(lpSrc + 1) == 0x1F && *(lpSrc + 2) == 0x04 && *(lpSrc + 3) == 0x00)
//	{
//		Temp[0] = *(lpSrc + 4);
//		Temp[1] = *(lpSrc + 5);
//		Temp[2] = *(lpSrc + 6);
//        Conformance += IntToHex(Temp,3, Conformance);
//	    *Conformance = 0;
//        return 7;
//	}
//    return -1;
//
//}

//int DeCodeObjectIdentifier(struct Object_Identifier *O_I, const unsigned char * lpSrc)
//{
//	if(*lpSrc == 0x60 && *(lpSrc + 1) == 0x85 &&
//		 *(lpSrc + 2) == 0x74 && *(lpSrc + 3) == 0x05 &&
//		 *(lpSrc + 4) == 0x08)
//	{
//		/*
//			us jic;
//			us c;
//			us cn;
//			us io;
//			us DLMS_UA;
//			us index;
//			us id;
//		*/
//		(*O_I).jic = 2; (*O_I).c = 16;
//		(*O_I).cn = 756; (*O_I).io = 5;
//		(*O_I).DLMS_UA = 8;
//		(*O_I).index = *(lpSrc + 5);
//		(*O_I).id = *(lpSrc + 6);
//		return 7;
//	}
//	else
//		return -1;
//
//
////    unsigned char c1, c2, c3, c4, c5, c6, c7;
////    c1 = *lpSrc++;
////    c2 = *lpSrc++;
////    c3 = *lpSrc++;
////    c4 = *lpSrc++;
////    c5 = *lpSrc++;
////    c6 = *lpSrc++;
////    c7 = *lpSrc++;
////
////    (*O_I).jic = c1 / 40;
////    (*O_I).c = c1 - ((*O_I).jic * 40);
////    (*O_I).cn = ( (c2 & 0x7F) << 7) + c3;
////    (*O_I).io = c4;
////    (*O_I).DLMS_UA = c5;
////    (*O_I).index = c6;
////    (*O_I).id = c7;
////    return 7;
//};

//// DeCode the cosme object descripter
//int DeCodeObject(unsigned short *ClassID, unsigned char *OBIS,
//                            unsigned char *AttributeID, const unsigned char *lpSrc)
//{
//	int Count = 0;
//	unsigned int Value = 0;
//
//	Count += DeCodeSizedInteger(&Value, 2, lpSrc);
//	*ClassID = (unsigned short)Value;
//
//	Count += DeCodeOBIS(OBIS, lpSrc + Count);
//
//	OBIS[12] = 0;
//
//	//int Value1 = 0;
//
//	Count += DeCodeSizedInteger(&Value, 1, lpSrc + Count);
//	*AttributeID = (unsigned char)Value;
//
//	return Count;
//}


//// DeCode the logical name of cosem object
//int DeCodeOBIS(unsigned char * OBIS, const unsigned char *lpSrc)
//{
//	//if(*lpSrc == 0x09 && *(lpSrc + 1) == 0x06)
//	//{
//	//	lpSrc += 2;
//		IntToHex(lpSrc, 6, OBIS);
//		return 6;
//	//	return 8;
//	//}
//	//else
//	//	return 0;
//
//};

// DeCode Lenth of Data
int DeCodeLength(unsigned int *Length, const unsigned char *lpSrc)
{
	*Length = 0;
	if(((*lpSrc) & 0x80) > 0)//长度长编码
	{
		int OctetNum = (*lpSrc++) & 0x7F;		

		for(int i = 0; i < OctetNum; i++)
		{
			*Length = (*Length << 8) + *lpSrc++;
		}	
		return (OctetNum + 1);
	}
	else//长度短编码
	{
		*Length = *lpSrc;
		return 1;
	}
}


// DeCode the Data Type of  Array 
int DeCodeArray(unsigned int *ArrayNum, const unsigned char *lpSrc)
{
	return DeCodeLength(ArrayNum, lpSrc);
}


// DeCode the Data Type of Struct
int DeCodeStruct(unsigned int *NumOfElement, const unsigned char *lpSrc)
{
	return DeCodeLength(NumOfElement, lpSrc);  
}


int DeCodeSizedBit(std::string &BitStr, unsigned int BitNum,
	 const unsigned char *lpSrc)
{
	BitStr = "";
	unsigned ByteNum = BitNum / 8 + (BitNum % 8 > 0 ? 1 : 0);
	
	for(unsigned int i = 0; i < ByteNum; i++ )  
	{ 
		char szBuf[8] = { 0 };
		sprintf_s(szBuf, "%02X", *lpSrc++);
		BitStr += szBuf;
	}		                                                                      
	return ByteNum;
}


int DeCodeBit(std::string &BitStr, const unsigned char *lpSrc)
{
	unsigned int BitNum = 0;
	int tmp = 0;
    
	tmp = DeCodeLength(&BitNum, lpSrc);

	tmp += DeCodeSizedBit(BitStr, BitNum, lpSrc + tmp);
	
	return tmp;
}


int DeCodeOctetStr(std::string &OctetStr, const unsigned char *lpSrc)
{
	unsigned int OctetNum = 0;
	int tmp = 0;
	
	tmp = DeCodeLength(&OctetNum, lpSrc);

	tmp += DeCodeSizedOctetStr(OctetStr, OctetNum, lpSrc + tmp);

    return tmp;
}

int DeCodeSizedOctetStr(std::string &OctetStr, unsigned int OctetNum,
                              const unsigned char *lpSrc)
{
    OctetStr = "";
	for(unsigned int i = 0; i < OctetNum; i++ )  
	{
		char szBuf[8] = { 0 };
		sprintf_s(szBuf, "%02X", *lpSrc++);
		OctetStr += szBuf;
	}         
	return OctetNum;
}

int DeCodeVisibleStr(std::string &VisibleStr, const unsigned char *lpSrc)
{
    unsigned int OctetNum = 0;
    VisibleStr = "";
    const unsigned char *lptmp = lpSrc;

	lptmp += DeCodeLength(&OctetNum, lptmp);

    for(unsigned int i = 0; i < OctetNum; i++ )  
	{
		VisibleStr += char(*lptmp++);
	}

    return lptmp - lpSrc;
}

// DeCode teh Data Type of  sized integer
int DeCodeSizedInteger(__int64 *Value, unsigned int ByteNum, const unsigned char *lpSrc)
{
	*Value = 0;
	if( ByteNum <= 4 )
	{
            for(unsigned int i = 0; i < ByteNum; i ++)
            {			
                    *Value = (*Value << 8) + *lpSrc++;			
            }
            return ByteNum;
  	}
        else if( ByteNum == 8)
        {
			std::string temp;
			char szBuf[8] = { 0 };
			sprintf_s(szBuf, "%02X", lpSrc[0]);
			temp += szBuf;
			sprintf_s(szBuf, "%02X", lpSrc[1]);
			temp += szBuf;
			sprintf_s(szBuf, "%02X", lpSrc[2]);
			temp += szBuf;
			sprintf_s(szBuf, "%02X", lpSrc[3]);
			temp += szBuf;
			sprintf_s(szBuf, "%02X", lpSrc[4]);
			temp += szBuf;
			sprintf_s(szBuf, "%02X", lpSrc[5]);
			temp += szBuf;
			sprintf_s(szBuf, "%02X", lpSrc[6]);
			temp += szBuf;
			sprintf_s(szBuf, "%02X", lpSrc[7]);
			temp += szBuf;

			*Value = strtoll(temp.c_str(), NULL, 16);
            if (*Value < 0)
            {
                *Value = 0 - *Value;
            }
            return ByteNum;
        }
	else
	{
	    return 0;
	}
};

int DeCodeSizedInteger_S(__int64 *Value, unsigned int ByteNum, const unsigned char *lpSrc)
{
	*Value = 0;
	if( ByteNum <= 4 )
	{
		for(unsigned i = 0; i < ByteNum; i ++)
		{			
			*Value = (*Value << 8) + *lpSrc++;			
		}		
        }
	else
	{
		return 0;
	}
	/*printf("*Value = %d\n",*Value);
	switch(ByteNum)
	{
		case 1:
			if(*Value > 127) *Value = *Value - 256;	
			printf("*Value = %d\n",*Value);	
			break;	
		case 2:
			break;
		
		case 4:
			break;
	}*/
	return ByteNum;
};


int DeCodeSizedUnsigned(unsigned int *Value, unsigned int ByteNum,
	 const unsigned char *lpSrc)
{
	*Value = 0;
	
	if( ByteNum <= 4 )
	{
		for(unsigned i = 0; i < ByteNum; i ++)
		{
			*Value = (*Value << 8) + *lpSrc++;
		}
		return ByteNum;
  	}
	else
	{
		return 0;
	}
}

unsigned char DeCodeBCD(unsigned char *Value, const unsigned char *lpSrc)
{
	return 0;
}


int DeCodeFloat32(float *Value, const unsigned char *lpSrc)
{
    unsigned char ii[4];
    ii[0] = lpSrc[3];
    ii[1]  = lpSrc[2];
    ii[2]  = lpSrc[1];
    ii[3]    = lpSrc[0];
    memcpy((unsigned char *)Value, ii, 4);
	return 4;
}


int DeCodeFloat64(float *Value, const unsigned char *lpSrc)
{
    unsigned char ii[8];
    ii[0] = lpSrc[3];
    ii[1] = lpSrc[2];
    ii[2] = lpSrc[1];
    ii[3] = lpSrc[0];
    ii[0] = lpSrc[3];
    ii[1] = lpSrc[2];
    ii[2] = lpSrc[1];
    ii[3] = lpSrc[0];
    memcpy((unsigned char *)Value, ii, 4);
	return 8;
}

int DeCodeDateTime(std::string &DtTm, const unsigned char *lpSrc)
{
//	if(lpSrc[0] == 0x09 && lpSrc[1] == 0x0C)
//	{
//		memcpy(DtTm, lpSrc + 2, 12 * sizeof(unsigned char));	
//		return 14;
//	}
//	else
//		return 0;
    return DeCodeSizedOctetStr(DtTm, 12, lpSrc);
}


int DeCodeDate(std::string &Dt, const unsigned char *lpSrc)
{
//	if(lpSrc[0] == 0x09 && lpSrc[1] == 0x05)
//	{
//		memcpy(Dt, lpSrc + 2, 5 * sizeof(unsigned char));
//		return 7;
//	}
//	else
//		return 0;
    return DeCodeSizedOctetStr(Dt, 5, lpSrc);
}


int DeCodeTime(std::string &Tm, const unsigned char *lpSrc)
{
//	if(lpSrc[0] == 0x09 && lpSrc[1] == 0x04)
//	{
//		memcpy(Tm, lpSrc + 2, 4 * sizeof(unsigned char));	
//		return 6;
//	}
//	else
//		return 0;
    return DeCodeSizedOctetStr(Tm, 4, lpSrc);
}



