// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once
// 添加这个宏，可以避免winsock2.h和winsock.h头文件冲突
// 还可以关闭一些不常用的系统函数，加快编译
#define WIN32_LEAN_AND_MEAN
#define DB_DLL_IMPORT
#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
