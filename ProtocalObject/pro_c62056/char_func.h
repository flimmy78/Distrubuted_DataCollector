//---------------------------------------------------------------------------

#ifndef char_funcH
#define char_funcH
//---------------------------------------------------------------------------
#include <string>

bool IsHex(const std::string& HexStr);
int HexToInt(const std::string& HexStr);
int IntToHex(int Value, unsigned char *lpDest);//返回十六进制字节串的长度;
std::string ToDisplayChar(const unsigned char *source, unsigned int lenth);
void ManMumIntToStr(unsigned char *ManNumStr, __int64 ManNum, unsigned char Len);
#endif
