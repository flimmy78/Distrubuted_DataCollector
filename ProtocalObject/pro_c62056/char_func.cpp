//---------------------------------------------------------------------------

#pragma hdrstop

#include "char_func.h"
//---------------------------------------------------------------------------
bool IsHex(const std::string& HexStr)
{
    if(HexStr.length() % 2 != 0)
        return false;
        
    for(unsigned int i = 0; i < HexStr.length(); i ++)
    {
        if((HexStr[i + 1] >= '0' && HexStr[i+1] <= '9')
            || (HexStr[i + 1] >= 'A' && HexStr[i + 1] <= 'F')
            || (HexStr[i + 1] >= 'a' && HexStr[i + 1] <= 'f'))
        {
            continue;
        }
        else
        {
            return false;
        }
    }
    return true;
};


int HexToInt(const std::string&  HexStr)
{    
	char tt[3];
    strcpy_s(tt, HexStr.c_str());

	int result;// = 0;
	
	for (int i = 0; i <= 1; i++)  
	{    
		if (tt[i]>='0' && tt[i]<='9')//在0~9之间    
		{      
			tt[i] = tt[i]-48;    
		}    
		else
		{
			if ( tt[i]>='A' && tt[i]<='F' )
			{              
				tt[i] = tt[i]-55;        
			}
            else if ( tt[i]>='a' && tt[i]<='f' )
            {
                tt[i] = tt[i]-87; 
            }
            else
            {
                //throw TException("");
            }
		}
	}
	result = (tt[ 0 ]* 16 + tt[ 1 ]);  	
	return result;
};

int IntToHex(int Value, unsigned char *lpDest)//返回十六进制字节串的长度
{
	unsigned char *lptmp = lpDest;
		
	char tt, tz;

	tt = Value / 16;   // 字节的高四位
	if ( tt >= 10 )
		tz = tt - 10 + 0x41;
	else
		tz = tt + 0x30;

	*lptmp++ = tz;

	//printf("%c", tz);
	

	tt = Value & 0x0F; // 字节的低四位
	if ( tt >= 10 )
		tz = tt -10 + 0x41;
	else
		tz = tt + 0x30;

	*lptmp++ = tz;

	return 2;
};

std::string ToDisplayChar(const unsigned char *source, unsigned int lenth)
{
    if (source == NULL || lenth == 0)
        return "";
	std::string dest = "";
    char tt, tz;
    for(unsigned int i = 0; i < lenth; i++)
    {
        tt = source[i] / 16;   // 字节的高四位
        if(tt >= 10)
            tz = tt - 10 + 0x41;
        else
            tz = tt + 0x30;
        dest += tz;

        tt = source[i] & 0x0F; // 字节的低四位
        if( tt >= 10 )
            tz = tt -10 + 0x41;
        else
            tz = tt + 0x30;
        dest += tz;
        dest += " ";
    }
    return dest;
};

void ManMumIntToStr(unsigned char *ManNumStr, __int64 ManNum, unsigned char Len)
{
    unsigned char* Vv = (unsigned char*)&ManNum;
    if(Len > 8)
        return;
        
    for(unsigned char i = 0; i < Len; i++)
    {
        ManNumStr[i] = Vv[Len - i - 1]; 
    }
}
