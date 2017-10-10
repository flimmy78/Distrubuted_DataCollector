//---------------------------------------------------------------------------

#pragma hdrstop

#include "common_func.h"
//---------------------------------------------------------------------------
int GetTag(const char* TagStr)
{
    if (!_stricmp(TagStr, "null-data"))						return 0;
    if (!_stricmp(TagStr, "array"))							return 1;
    if (!_stricmp(TagStr, "structure"))						return 2;
    if (!_stricmp(TagStr, "boolean"))						return 3;
    if (!_stricmp(TagStr, "bit-string"))					return 4;
    if (!_stricmp(TagStr, "double-long"))					return 5;
	if (!_stricmp(TagStr, "double-long-unsigned"))			return 6;
	if (!_stricmp(TagStr, "octet-string"))					return 9;
	if (!_stricmp(TagStr, "visible-string"))				return 10;
	if (!_stricmp(TagStr, "bcd"))							return 13;
	if (!_stricmp(TagStr, "integer"))						return 15;
	if (!_stricmp(TagStr, "long"))							return 16;
	if (!_stricmp(TagStr, "unsigned"))						return 17;
	if (!_stricmp(TagStr, "long-unsigned"))					return 18;
	if (!_stricmp(TagStr, "compact-array"))					return 19;
	if (!_stricmp(TagStr, "long64"))						return 20;
	if (!_stricmp(TagStr, "long64-unsigned"))				return 21;
	if (!_stricmp(TagStr, "enum"))							return 22;
	if (!_stricmp(TagStr, "float32"))						return 23;
	if (!_stricmp(TagStr, "float64"))						return 24;
	if (!_stricmp(TagStr, "date_time"))						return 25;
	if (!_stricmp(TagStr, "date"))							return 26;
	if (!_stricmp(TagStr, "time"))							return 27;
	if (!_stricmp(TagStr, "don't-care"))					return 255;
    else
        return -1;                
};

int CalcRawDataLen(int MaxPduSize, int HeadLen)
{
    if(MaxPduSize <= 127 + HeadLen + 1)
    {
        return(MaxPduSize - HeadLen - 1);
    }
    else if(MaxPduSize <= 256 +  + HeadLen + 2)
    {
        return(MaxPduSize - HeadLen - 2);
    }
    else //if() 65546 ¼´ 65535
    {
        return(MaxPduSize - HeadLen - 3);
    }
}

