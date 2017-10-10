#include "stdafx.h"
#include "../include/common.h"
#include <stdlib.h>


// 返回自系统开机以来的毫秒数（tick）  
unsigned long WSCOMMON::GetTickCountEX(void)
{
	unsigned long currentTime;
#ifdef WIN32
	currentTime = GetTickCount();
#endif
#ifdef LINUX
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
	return currentTime;
}

// 获取系统时间
void WSCOMMON::Getlocaltime(tm & curTime)
{

#ifdef WIN32
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	curTime.tm_year = sysTime.wYear;
	curTime.tm_mon = sysTime.wMonth;
	curTime.tm_mday = sysTime.wDay;

	curTime.tm_hour = sysTime.wHour;
	curTime.tm_min = sysTime.wMinute;
	curTime.tm_sec = sysTime.wSecond;
#endif
#ifdef LINUX
	struct tm *ptr;
	time_t lt;
	lt = time(NULL);
	ptr = localtime(&lt);

	curTime.tm_hour = ptr->tm_hour;
	curTime.tm_isdst = ptr->tm_isdst;
	curTime.tm_mday = ptr->tm_mday;
	curTime.tm_min = ptr->tm_min;
	curTime.tm_mon = ptr->tm_mon;
	curTime.tm_sec = ptr->tm_sec;
	curTime.tm_wday = ptr->tm_wday;
	curTime.tm_yday = ptr->tm_yday;
	curTime.tm_year = ptr->tm_year;
#endif
}

//产生长度为length的随机字符串    
void GenRandomString(int length, char* ouput)
{
	int flag, i;
	srand((unsigned)time(NULL));
	for (i = 0; i < length - 1; i++)
	{
		flag = rand() % 3;
		switch (flag)
		{
		case 0:
			ouput[i] = 'A' + rand() % 26;
			break;
		case 1:
			ouput[i] = 'a' + rand() % 26;
			break;
		case 2:
			ouput[i] = '0' + rand() % 10;
			break;
		default:
			ouput[i] = 'x';
			break;
		}
	}
}
