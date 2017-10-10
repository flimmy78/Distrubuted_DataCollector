//---------------------------------------------------------------------------

#ifndef C62056H
#define C62056H
//---------------------------------------------------------------------------
/*
 * 文件:C62056.h
 * 说明:62056规约解析接口头文件
 * 内容:
 * 日期:2010-10-10
 * 作者:孙丽霞
 */

/*
 * 改动记录：
 *
 *
 */


#include "C62056_Struct.h"

#ifndef _DLL_FILE_

    #pragma comment(lib,"pro_C62056.lib")

#endif


//#ifdef _WINDOWS
    #ifdef _DLL_FILE_
	    #define DLL_EXIM __declspec(dllexport)
    #else
	    #define DLL_EXIM __declspec(dllimport)
    #endif
//#else
//    #define DLL_EXIM
//#endif


//#ifdef  __cplusplus
extern "C" {
//#endif
    /*********************************************
    *   函数名称：SetParams
    *   功能    ：设置协议栈的参数
    *   参数    ：
    *
    *   返回值  ：  
    *
    *   author:         孙丽霞
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM int _stdcall SetParams(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr,
                                    const unsigned char *Xml, unsigned int XmlLen);
    
    /*********************************************
    *   函数名称：ReleaseMemory
    *   功能    ：释放内存空间，若使用的内存空间由DLL分配，最后也应调用该函数释放
    *   参数    ：
    *               1. 
    *
    *   返回值  ：  无
    *   备注    ：不可在应用程序中释放DLL分配的内存空间
    *
    *   author:         孙丽霞
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM void _stdcall ReleaseMemory(unsigned char **OUTData);


    /*********************************************
    *   函数名称：TimeOut
    *   功能    ：处理服务器超时
    *   参数    ：
    *               1. OUTData,OUTDataLen 输出的XML文档(字符串)和长度，可能包括帧或者服务原语,或者两者都有,或者两者均无
    *               2. SupportLayerType 使用的下层支持层,目前仅支持 HDLC
    *               3. Addr 地址参数
    *
    *   返回值  ：  整型
    *               = 0 XML文档
    *               > 0 错误消息, 此时忽略OUTData中的内容
    *   备注    ：输出内容所使用的空间，由DLL负责分配，应用进程负责调用ReleaseMemory释放
    *
    *   author:         孙丽霞
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM int _stdcall ResponseTimeOut(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);


	/*********************************************
    *   函数名称：ProcessServicePrimitive
    *   功能    ：处理服务原语
    *   参数    ：
    *               1. OUTData, OUTDataLen 输出的XML文档(字符串)和长度，可能包括帧或者服务原语,或者两者都有,或者两者均无
    *               2. SupportLayerType 使用的下层支持层,目前仅支持 HDLC
    *               3. Addr 地址参数
    *               4. Xml, XmlLen 输入的XML文档(字符串)
    *
    *   返回值  ：  整型
    *               = 0 XML文档
    *               > 0 错误消息, 此时忽略OUTData中的内容
    *   备注    ：输出内容所使用的空间，由DLL负责分配，应用进程负责调用ReleaseMemory释放
    *
    *   author:         孙丽霞
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM int _stdcall ProcessServicePrimitive(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr,
                                    const unsigned char *Xml, unsigned int XmlLen);


	
    /*********************************************
    *   函数名称：ProcessFrame
    *   功能    ：处理接收到的帧
    *   参数    ：
    *               1. OUTData, OUTDataLen 输出的XML文档(字符串)和长度，可能包括帧或者服务原语,或者两者都有,或者两者均无
    *               2. SupportLayerType 使用的下层支持层,目前仅支持 HDLC
    *               3. Frame, FrameLen 输入的帧
    *
    *   返回值  ：  整型
    *               = 0 XML文档
    *               > 0 错误消息, 此时忽略OUTData中的内容
    *   备注    ：输出内容所使用的空间，由DLL负责分配，应用进程负责调用ReleaseMemory释放
    *
    *   author:         孙丽霞
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM int _stdcall ProcessFrame(unsigned char **OUTData, unsigned int &OUTDataLen,
                                SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN],
                                const unsigned char *Frame, unsigned int FrameLen);


    /*********************************************
    *   函数名称：PhyAbort
    *   功能    ：处理物理链接断开中断
    *   参数    ：
    *               1. SupportLayerType 使用的下层支持层,目前仅支持 HDLC
    *               2. Addr 地址参数
    *
    *   返回值  ：  无
    *
    *   author:         孙丽霞
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM void _stdcall PhyAbort(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);

    /*********************************************
    *   函数名称：SetSecurity
    *   功能    ：设置加密相关参数
    *   参数    ：
    *               1. SupportLayerType 使用的下层支持层,目前仅支持 HDLC
    *               2. Addr 地址参数
    *               3. 加密相关参数，注意该参数内的ManuId，ManuNum，Fc等均为客户端参数
    *
    *   返回值  ：  无
    *   备注    ：
    *
    *   author:         孙丽霞
    *   Corporation:    Wasion
    *   Date:           2013-09-10
    *********************************************/
    /*extern "C" */DLL_EXIM unsigned char _stdcall SetSecurity(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr,
                                    SECURITY_OPTION SecurityOption);

//#ifdef  __cplusplus
};
//#endif

#endif
