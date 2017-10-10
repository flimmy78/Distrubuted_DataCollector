//---------------------------------------------------------------------------

#ifndef C62056H
#define C62056H
//---------------------------------------------------------------------------
/*
 * �ļ�:C62056.h
 * ˵��:62056��Լ�����ӿ�ͷ�ļ�
 * ����:
 * ����:2010-10-10
 * ����:����ϼ
 */

/*
 * �Ķ���¼��
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
    *   �������ƣ�SetParams
    *   ����    ������Э��ջ�Ĳ���
    *   ����    ��
    *
    *   ����ֵ  ��  
    *
    *   author:         ����ϼ
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM int _stdcall SetParams(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr,
                                    const unsigned char *Xml, unsigned int XmlLen);
    
    /*********************************************
    *   �������ƣ�ReleaseMemory
    *   ����    ���ͷ��ڴ�ռ䣬��ʹ�õ��ڴ�ռ���DLL���䣬���ҲӦ���øú����ͷ�
    *   ����    ��
    *               1. 
    *
    *   ����ֵ  ��  ��
    *   ��ע    ��������Ӧ�ó������ͷ�DLL������ڴ�ռ�
    *
    *   author:         ����ϼ
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM void _stdcall ReleaseMemory(unsigned char **OUTData);


    /*********************************************
    *   �������ƣ�TimeOut
    *   ����    �������������ʱ
    *   ����    ��
    *               1. OUTData,OUTDataLen �����XML�ĵ�(�ַ���)�ͳ��ȣ����ܰ���֡���߷���ԭ��,�������߶���,�������߾���
    *               2. SupportLayerType ʹ�õ��²�֧�ֲ�,Ŀǰ��֧�� HDLC
    *               3. Addr ��ַ����
    *
    *   ����ֵ  ��  ����
    *               = 0 XML�ĵ�
    *               > 0 ������Ϣ, ��ʱ����OUTData�е�����
    *   ��ע    �����������ʹ�õĿռ䣬��DLL������䣬Ӧ�ý��̸������ReleaseMemory�ͷ�
    *
    *   author:         ����ϼ
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM int _stdcall ResponseTimeOut(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);


	/*********************************************
    *   �������ƣ�ProcessServicePrimitive
    *   ����    ���������ԭ��
    *   ����    ��
    *               1. OUTData, OUTDataLen �����XML�ĵ�(�ַ���)�ͳ��ȣ����ܰ���֡���߷���ԭ��,�������߶���,�������߾���
    *               2. SupportLayerType ʹ�õ��²�֧�ֲ�,Ŀǰ��֧�� HDLC
    *               3. Addr ��ַ����
    *               4. Xml, XmlLen �����XML�ĵ�(�ַ���)
    *
    *   ����ֵ  ��  ����
    *               = 0 XML�ĵ�
    *               > 0 ������Ϣ, ��ʱ����OUTData�е�����
    *   ��ע    �����������ʹ�õĿռ䣬��DLL������䣬Ӧ�ý��̸������ReleaseMemory�ͷ�
    *
    *   author:         ����ϼ
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM int _stdcall ProcessServicePrimitive(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr,
                                    const unsigned char *Xml, unsigned int XmlLen);


	
    /*********************************************
    *   �������ƣ�ProcessFrame
    *   ����    ��������յ���֡
    *   ����    ��
    *               1. OUTData, OUTDataLen �����XML�ĵ�(�ַ���)�ͳ��ȣ����ܰ���֡���߷���ԭ��,�������߶���,�������߾���
    *               2. SupportLayerType ʹ�õ��²�֧�ֲ�,Ŀǰ��֧�� HDLC
    *               3. Frame, FrameLen �����֡
    *
    *   ����ֵ  ��  ����
    *               = 0 XML�ĵ�
    *               > 0 ������Ϣ, ��ʱ����OUTData�е�����
    *   ��ע    �����������ʹ�õĿռ䣬��DLL������䣬Ӧ�ý��̸������ReleaseMemory�ͷ�
    *
    *   author:         ����ϼ
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM int _stdcall ProcessFrame(unsigned char **OUTData, unsigned int &OUTDataLen,
                                SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN],
                                const unsigned char *Frame, unsigned int FrameLen);


    /*********************************************
    *   �������ƣ�PhyAbort
    *   ����    �������������ӶϿ��ж�
    *   ����    ��
    *               1. SupportLayerType ʹ�õ��²�֧�ֲ�,Ŀǰ��֧�� HDLC
    *               2. Addr ��ַ����
    *
    *   ����ֵ  ��  ��
    *
    *   author:         ����ϼ
    *   Corporation:    Wasion
    *   Date:           2010-10-10
    *********************************************/
    /*extern "C" */DLL_EXIM void _stdcall PhyAbort(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);

    /*********************************************
    *   �������ƣ�SetSecurity
    *   ����    �����ü�����ز���
    *   ����    ��
    *               1. SupportLayerType ʹ�õ��²�֧�ֲ�,Ŀǰ��֧�� HDLC
    *               2. Addr ��ַ����
    *               3. ������ز�����ע��ò����ڵ�ManuId��ManuNum��Fc�Ⱦ�Ϊ�ͻ��˲���
    *
    *   ����ֵ  ��  ��
    *   ��ע    ��
    *
    *   author:         ����ϼ
    *   Corporation:    Wasion
    *   Date:           2013-09-10
    *********************************************/
    /*extern "C" */DLL_EXIM unsigned char _stdcall SetSecurity(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr,
                                    SECURITY_OPTION SecurityOption);

//#ifdef  __cplusplus
};
//#endif

#endif
