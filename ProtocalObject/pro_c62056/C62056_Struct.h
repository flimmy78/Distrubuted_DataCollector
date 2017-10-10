#ifndef _C62056_STRUCT_H_
#define _C62056_STRUCT_H_
#include "Public.h"
#define ID_LEN 16

//新增  与加密相关
#define MAN_ID_LEN 3
#define MAN_NUM_LEN 5
#define FC_LEN 4
#define KEY_LEN 16

//下层支持层
typedef enum _SUPPORT_LAYER
{
    HDLC = 0x01,
    TCPIP,
    UDPIP
}SUPPORT_LAYER;


typedef union _ADDR
{
    struct
    {
        unsigned int ServerLowerAddr : 14;  //服务器低地址
        unsigned int ServerUpperAddr : 14;  //服务器高地址
        unsigned int res : 4;
        unsigned char ClientAddr;
        unsigned char ServerAddrLen;
    }HDLC_ADDR1;

    struct
    {
        unsigned int ServerAddr : 28;
        unsigned int res : 4;
        unsigned char ClientAddr;
        unsigned char ServerAddrLen;
    }HDLC_ADDR2;

    struct
    {
        //unsigned char ID[ID_LEN];  //modified by slx 2012-9-14
        unsigned short Client_wPort; //modified by slx 2012-9-14
        unsigned short Server_wPort;  //modified by slx 2012-9-14
        //unsigned int Client_TCP_Port;
        //unsigned int Server_TCP_Port;
        //unsigned char Client_IP_Addr[4];
        //unsigned char Server_IP_Addr[4];
    }TCPIP_ADDR;

    struct
    {
        //unsigned char ID[ID_LEN];    //modified by slx 2012-9-14
        unsigned int Client_wPort;
        unsigned int Server_wPort;
        //unsigned int Client_UDP_Port;
        //unsigned int Server_UDP_Port;
        //unsigned char Client_IP_Addr[4];
        //unsigned char Server_IP_Addr[4];
    }UDPIP_ADDR;  
}ADDR;


//------------2013-9-7  新增与加密相关的参数

typedef enum _SECUTIRY_POLICY
{
    NO_SECURITY = 0,
    ALL_MESSAGES_AUTHENTICATED,
    ALL_MESSAGES_ENCRYPTED,
    ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED,
}SECUTIRY_POLICY;

typedef enum _SECURITY_SUITE
{
    AES_GCM_128 = 0,
}SECURITY_SUITE;

typedef struct _SECURITY_MATERIAL
{
    unsigned char MK[KEY_LEN];	
    unsigned char EK[KEY_LEN];
    unsigned char BK[KEY_LEN];	
    unsigned char AK[KEY_LEN];
    unsigned char DK[KEY_LEN];
    union
    {
        struct
        {
            unsigned char ManId[MAN_ID_LEN];
            unsigned char ManNum[MAN_NUM_LEN];
            unsigned char FC[FC_LEN];
        }params;
        unsigned char IV[IV_LEN];
    }IV;
}SECURITY_MATERIAL;

typedef struct _SECURITY_OPTION
{
    SECUTIRY_POLICY SecurityPolicy;
    SECURITY_SUITE  SecuritySuite;
    SECURITY_MATERIAL SecurityMaterial;
}SECURITY_OPTION;
#endif
 