//---------------------------------------------------------------------------

#ifndef char_funcH
#define char_funcH
//---------------------------------------------------------------------------
#include <string>

bool IsHex(const std::string& HexStr);
int HexToInt(const std::string& HexStr);
int IntToHex(int Value, unsigned char *lpDest);//����ʮ�������ֽڴ��ĳ���;
std::string ToDisplayChar(const unsigned char *source, unsigned int lenth);
void ManMumIntToStr(unsigned char *ManNumStr, __int64 ManNum, unsigned char Len);
#endif
