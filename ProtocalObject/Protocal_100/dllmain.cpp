// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

short GetByteFromHexChar(char hChar)
{
	switch (hChar)
	{
	case 48:
		return 0;
	case 49:
		return 1;
	case 50:
		return 2;
	case 51:
		return 3;
	case 52:
		return 4;
	case 53:
		return 5;
	case 54:
		return 6;
	case 55:
		return 7;
	case 56:
		return 8;
	case 57:
		return 9;
	case 65:
	case 97:
		return 10;
	case 66:
	case 98:
		return 11;
	case 67:
	case 99:
		return 12;
	case 68:
	case 100:
		return 13;
	case 69:
	case 101:
		return 14;
	case 70:
	case 102:
		return 15;
	default:
		return -1;
	}
}

void HexStrToByteArray(const char* pHexStr, unsigned char* szByte, unsigned long& nSize)
{
	UINT nFrameSize = strlen(pHexStr);
	short nRtn = 0; int k = -1;
	BYTE btGot, count = 0;

	for (UINT i = 0; i < nFrameSize; i++)
	{
		nRtn = GetByteFromHexChar(pHexStr[i]);
		if (-1 == nRtn)
		{
			count = 0;
			continue;
		}
		else
		{
			count++;
		}

		if (1 == count)
		{
			btGot = nRtn * 16;
			continue;
		}
		else
		{
			btGot += nRtn;
			k++;
			szByte[k] = btGot;
			count = 0;
		}
	}
	nSize = k + 1;
}

bool OBISToHexStr(char* pOBIS, char* pHexStr)
{
	if (0 == pOBIS[0])
	{
		return false;
	}
	try
	{
		sprintf_s(pHexStr, 50, "%02X", atoi(pOBIS));
		char szHexStr[10];
		BYTE index1(0), index2(0), index3(0), index4(0), index5(0);
		for (BYTE i = 0; 0 != pOBIS[i]; i++)
		{
			if ('.' == pOBIS[i])
			{
				sprintf_s(szHexStr, "%02X", atoi(&pOBIS[i + 1]));
				strcat_s(pHexStr, 50, szHexStr);
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

void StrToHEXStr(const char* pStr, char* pHexStr, unsigned long& nSize)
{
	unsigned long i = 0;
	for (; i < nSize; i++)
	{
		sprintf_s(&(pHexStr[i * 2]), 1050, "%2X", (unsigned char)(pStr[i]));
	}
	nSize = i * 2;
}

bool StrIsDigit(char* pStr, int nLen)
{
	for (int i = 0; i < nLen; i++)
	{
		if ((pStr[i] < 48) || (pStr[i] > 57))
		{
			return false;
		}
	}
	return true;
}

