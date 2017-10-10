#ifndef _PUBLIC_H_
#define _PUBLIC_H_


#include "Gcm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "aes_gcm.h"
//---------------------------------------
#define MAX_DEDICATEDKEY_LEN 32
#define MAX_PASSWORD_LEN 64 //密码的最大长度

//新增 与加密相关
#define AP_TITLE_LEN 8
//-----------------------------------------
typedef enum _DL_CONNECT_RESULT
{
	CONNECT_OK,
	CONNECT_NOK_REMOTE,
    CONNECT_NOK_LOCAL,
	CONNECT_NO_RESPONSE
}DL_CONNECT_RESULT;

typedef enum _DL_DISC_RESULT
{
	DISC_OK,
	DISC_NOK,
	DISC_NO_RESPONSE
}DL_DISC_RESULT;

typedef enum _DL_DISC_REASON
{
	REMOTE,
	LOCAL_DL,
	LOCAL_PHY
}DL_DISC_REASON;  

typedef enum _FRAME_TYPE
{
	I_COMPLETE,
	I_FIRST_FRAGMENT, 
	I_FRAGMENT, 
	I_LAST_FRAGMENT, 
	UI
}FRAME_TYPE;

//typedef enum _PRIMITIVE_DL
//{
//	//00 为无服务
//    NO_PRIMITIVE = 0x00,
//    DL_CONNECT_REQ,
//    DL_DATA_REQ,
//    DL_DISCONNECT_REQ,
//	DL_CONNECT_CNF,
//	DL_DATA_IND,
//	DL_DISCONNECT_CNF,
//}PRIMITIVE_DL;  

//typedef struct _DL_PRIMITIVESERVICE
//{
//	PRIMITIVE_DL Primitive;
//	union
//	{
//        struct
//        {
//            const unsigned char *lpUserInfor;
//			unsigned int UserInforLen;   
//        }DlConnectReq;
//
//        struct
//        {
//            FRAME_TYPE FrameType;
//            const unsigned char *lpPdu;
//			unsigned int PduLen;   
//        }DlDataReq;
//
//        struct
//        {
//            const unsigned char *lpUserInfor;
//			unsigned int UserInforLen;   
//        }DlDisconnectReq;
//        
//		struct
//		{
//            DL_CONNECT_RESULT Result;
//			const unsigned char *lpUserInfor;
//			unsigned int UserInforLen;
//			//UserInformation
//		}DlConnectCnf;	
//		
//		struct
//		{			
//			FRAME_TYPE FrameType;
//			const unsigned char *lpPdu;
//			unsigned int PduLen;
//			//Data
//		}DlDataInd;
//		
//		struct
//		{	
//			DL_DISC_RESULT Result;
//			const unsigned char *lpUserInfor;
//			unsigned int UserInforLen;
//			//UserInformation
//		}DlDisconnectCnf; 		
//	}Params;
//}DL_PRIMITIVESERVICE;

//--------------HDLC---------------------
typedef enum _MAC_STATE//
{
	NDM = 0x00,
	WAIT_CONNECTION,
	NRM,
	WAIT_DISCONNECTION
}MAC_STATE;


typedef enum _FRAME_COMMAND
{
	I_FRAME = 0x00,
	UI_FRAME = 0x03,
	RR_FRAME = 0x11,
	RNR_FRAME = 0x15,
	DM_FRAME = 0x1F,
	DISC_FRAME = 0x53,
	UA_FRAME = 0x73,
	SNRM_FRAME = 0x93,
	FRMR_FRAME = 0x97
}FRAME_COMMAND;

typedef union _Frame_Format
{
	unsigned short Val;
	struct
	{   
        unsigned short FrameLength : 11;
        unsigned short Segment : 1;
        unsigned short FrameType : 4;
	}bits;
}Frame_Format;

typedef union _Ctrl_Format
{
	unsigned char Val;
	struct
	{  
		unsigned char Res : 1;
        unsigned char SSS : 3;
        unsigned char PorF : 1;
        unsigned char RRR : 3;
	}bits;
}Ctrl_Format;

typedef struct _HDLC_PARAMS
{
	unsigned char WindowSizeTransmit, WindowSizeReceive;
	unsigned short MaximumInformationFieldLengthTransmit, MaximumInformationFieldLengthReceive;
}HDLC_PARAMS;

//-------------------------COSEMAPP---------------------------------------
typedef enum _COSEMAPP_STATE//
{
	IDLE = 0x00,
    ASSOCIATION_PENDING,
    ASSOCIATED,
    HLS_ASSOCIATION_PENDING,
    HLS_ASSOCIATED,
    ASSOCIATION_RELEASE_PENDING
}COSEMAPP_STATE;

typedef enum _APPLICATION_CONTEXT_NAME
{
    LN_NOCIPER = 0x01,
    SN_NOCIPER,
    LN_CIPER,
    SN_CIPER
}APPLICATION_CONTEXT_NAME;

typedef enum _SECURITY_MECHANISM_NAME
{
    LOWEST_LEVEL = 0x00,
    LOW_LEVEL,
	HIGH_LEVEL,
    HIGH_LEVEL_MD5,
    HIGH_LEVEL_SHA1,
    HIGH_LEVEL_CMAC,
}SECURITY_MECHANISM_NAME;

typedef enum _RLRQ_REASON
{
    RLRQ_REASON_NORMAL = 0,
    RLRQ_REASON_URGENT = 1,
    RLRQ_REASON_USER_DEFINED = 30   
}RLRQ_REASON;

typedef union _CONFORMANCE_BLOCK
{
	unsigned char Val[3];
	struct
	{    
		unsigned char res2 : 2;
		unsigned char UnconfirmedWrite : 1;
        unsigned char Write : 1;
        unsigned char Read : 1;
        unsigned char res1: 3;

		unsigned char InformationReport : 1;
        unsigned char MultipleReferences : 1;
		unsigned char BlockTransferWithAction : 1;
        unsigned char BlockTransferWithSet : 1;
        unsigned char BlockTransferWithGet : 1;
        unsigned char Attribute0SupportedWithGet : 1;
        unsigned char PriorityMgmtSupported : 1;
        unsigned char Attribute0SupportedWithSet : 1;

		unsigned char Action : 1;
        unsigned char EventNotification : 1;
        unsigned char SelectiveAccess : 1;
        unsigned char Set : 1;
        unsigned char Get : 1;
        unsigned char ParameterizedAccess : 1;
        unsigned char res3 : 2;
	}bits;
}CONFORMANCE_BLOCK;

typedef enum _COSEM_APDU
{
    INITIATE_REQUEST = 1,
    READ_REQUEST = 5,
    WRITE_REQUEST = 6,
    INITIATE_RESPONSE = 8,
    
    READ_RESPONSE = 12,
    WRITE_RESPONSE = 13,
    CONFIRMED_SERVICE_ERROR = 14,
    UNCONFIRMED_WRITE_REQUEST = 22,
    INFORMATION_REPORT_REQUEST = 24,

    //--with global ciphering
    GLO_INITIATE_REQUEST = 33,
    GLO_READ_REQUEST = 37,
    GLO_WRITE_REQUEST = 38,
    GLO_INITIATE_RESPONSE = 40,
    GLO_READ_RESPONSE = 44,
    GLO_WRITE_RESPONSE = 45,

    GLO_CONFIRMED_SERVICE_ERROR = 46,
    GLO_UNCONFIRMED_WRITE_REQUEST = 54,
    GLO_INFORMATION_REPORT_REQUEST = 56,


    //

    GET_REQUEST = 192,
    SET_REQUEST = 193,
    EVENT_NOTIFICATION_REQUEST = 194,
    PUSH_FRAME = 15,
    ACTION_REQUEST = 195,
    GET_RESPONSE = 196,
    SET_RESPONSE = 197,
    ACTION_RESPONSE = 199,


    GLO_GET_REQUEST = 200,
    GLO_SET_REQUEST = 201,
    GLO_EVENT_NOTIFICATION_REQUEST = 202,
    GLO_ACTION_REQUEST = 203,

    GLO_GET_RESPONSE = 204,
    GLO_SET_RESPONSE = 205,
    GLO_ACTION_RESPONSE = 207,
    //===
    ded_get_request =208,
    ded_set_request =209,
    ded_event_notification_request=210,
    ded_action_Request =211,
    ded_get_response =212,
    ded_set_response=213,
    ded_action_response =215,
    EXCEPTION_RESPONSE = 216,

    ACTIONAUT_REQUEST = 250
}COSEM_APDU;

typedef struct _XDLMS_INITIATE_REQ
{
    unsigned char DedicatedKey[MAX_DEDICATEDKEY_LEN];
    unsigned char DedicatedKeyLen;
    bool ResponseAllowed;
    unsigned short ClientMaxReceivedPduSize;
    unsigned char DlmsVersionNumber;
    CONFORMANCE_BLOCK Conformance;   
}XDLMS_INITIATE_REQ;

typedef struct _AARQ_PARAMS
{
    APPLICATION_CONTEXT_NAME ApplicationContextName;
    //新增 加密相关
    char CallingAPTitle[AP_TITLE_LEN];
    bool SenderAcseRequirementsPresent;
    bool SenderAcseRequirements;
    SECURITY_MECHANISM_NAME MechanismName;
    unsigned char CallingAuthenticationValue[MAX_PASSWORD_LEN];
    unsigned int CallingAuthenticationValueLen;
    XDLMS_INITIATE_REQ XdlmsInitiateReq;

    unsigned char F_StoC[SH_LEN + T_LEN];
}AARQ_PARAMS;

typedef struct _XDLMS_INITIATE_RES
{
    unsigned short ServiceMaxReceivedPduSize;
    unsigned char DlmsVersionNumber;
    CONFORMANCE_BLOCK Conformance;   
}XDLMS_INITIATE_RES;

typedef enum _ASSOCIATION_RESULT
{
    ACCEPTED = 0,
    REJECTED_PERMANENT,
    REJECTED_TRANSIENT   
}ASSOCIATION_RESULT;

typedef struct _AARE_PARAMS
{
    APPLICATION_CONTEXT_NAME ApplicationContextName;
    bool ResponderAcseRequirementsPresent;
    bool ResponderAcseRequirements;
    ASSOCIATION_RESULT AssociationResult;
    SECURITY_MECHANISM_NAME MechanismName;
    unsigned char RespondingAPTitle[AP_TITLE_LEN];
    unsigned char RespondingAuthenticationValue[MAX_PASSWORD_LEN];
    unsigned int RespondingAuthenticationValueLen;
    XDLMS_INITIATE_RES XdlmsInitiateRes;
}AARE_PARAMS;



typedef struct _RLRQ_PARAMS
{
    bool ReleaseRequestReasonPresent;
    RLRQ_REASON ReleaseRequestReason;
}RLRQ_PARAMS;

typedef union _INVOKE_ID_AND_PRIORITY
{
	unsigned char Val;
	struct
	{
        unsigned char InvokeId : 4;
        unsigned char res : 2;
        unsigned char ServiceClass : 1;
        unsigned char Priority : 1;
	}bits;
}INVOKE_ID_AND_PRIORITY;

typedef enum _GET_REQUEST_TYPE
{
    GET_REQ_NORMAL = 0x01,
    GET_REQ_NEXT,
    GET_REQ_WITH_LIST
}GET_REQUEST_TYPE;

typedef enum _SET_REQUEST_TYPE
{
    SET_REQ_NORMAL = 0x01,
    SET_REQ_WITH_FIRST_DATABLOCK,
    SET_REQ_WITH_DATABLOCK,
    SET_REQ_WITH_LIST,
    SET_REQ_WITH_LIST_AND_FIRST_DATABLOCK
}SET_REQUEST_TYPE;


typedef enum _ACTION_REQUEST_TYPE
{
    ACTION_REQ_NORMAL = 0x01,
    ACTION_REQ_NEXT_PBLOCK,
    ACTION_REQ_WITH_LIST,
    ACTION_REQ_WITH_FIRST_PBLOCK,
    ACTION_REQ_WITH_LIST_AND_FIRST_PBLOCK,
    ACTION_REQ_WITH_PBLOCK
}ACTION_REQUEST_TYPE;


typedef struct _COSEM_ATTRIBUTE_DESCRIPTOR
{
    unsigned short ClassId;  
    unsigned char InstanceId[6];
    char AttributeId;
}COSEM_ATTRIBUTE_DESCRIPTOR;

/*
typedef enum _ACCESS_SELECTOR_VALUE
{
    RANGE_DESCRIPTOR = 0x01,
    ENTRY_DESCRIPTOR
}ACCESS_SELECTOR_VALUE;    */

typedef enum _GET_RESPONSE_TYPE
{
    GET_RES_NORMAL = 0x01,
    GET_RES_WITH_DATABLOCK,
    GET_RES_WITH_LIST
}GET_RESPONSE_TYPE;

typedef enum _SET_RESPONSE_TYPE
{
    SET_RES_NORMAL = 0x01,
    SET_RES_DATABLOCK,
    SET_RES_LAST_DATABLOCK,
    SET_RES_LAST_DATABLOCK_WITH_LIST,
    SET_RES_WITH_LIST
}SET_RESPONSE_TYPE;

typedef enum _ACTION_RESPONSE_TYPE
{
    ACTION_RES_NORMAL = 0x01,
    ACTION_RES_WITH_PBLOCK,
    ACTION_RES_WITH_LIST,
    ACTION_RES_NEXT_PBLOCK
}ACTION_RESPONSE_TYPE;


#endif
