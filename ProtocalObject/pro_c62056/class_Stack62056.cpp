//---------------------------------------------------------------------------

#include "class_Stack62056.h"
//---------------------------------------------------------------------------
#include <assert.h>

#include "char_func.h"
#include "calc_fcs.h"
#include "Encode.h"
#include "Decode.h"
#include "common_func.h"
#include <windows.h>


#define APDU_OFFSET 20
//------------------------------
//非数据帧
#define INFOR_OFFSET1 7  //一个帧中，信息域相对于0x7E的偏移量，不包括服务器地址
//数据帧
#define INFOR_OFFSET2 14

#define MAX_ENCAPSULATION_LEN 14  //最大帧封装的长度:包括帧头和帧尾
#define BUFF_REDUNDANCE 256//缓冲区的余量

//int FileNo = 0;
//------------------------COSEMApp-----------------------------

//-------------------------HDLC---------------------------------
typedef struct _FRAME_SEND_HEAD
{
	Frame_Format FrameFormat;
    unsigned int ServerAddr;
	unsigned char ClientAddr;
	Ctrl_Format Ctrl;
	unsigned short HCS;
}FRAME_SEND_HEAD;


//-----------------------------------------------------------
//extern unsigned int gXmlLen;
//---------------------------------------------------------
CStack62056::CStack62056(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr)
{
    ase_gcm_128 = new CAESGCM128();
    //
    mSupportLayerType = SupportLayerType;
    memcpy(&mAddr, &Addr, sizeof(ADDR));
    memcpy(mID, ID, ID_LEN * sizeof(unsigned char));

    //------------Security-----------------//
    InitSecurity();
    
    //------------COSEMApp-----------------//
    InitCosemAppParams();
    
    //--------------HDLC-----------------//
    InitHdlcParams();

    
}

void CStack62056::Reset(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr)
{
    //assert(SupportLayerType != HDLC);
    
    mSupportLayerType = SupportLayerType;
    memcpy(&mAddr, &Addr, sizeof(ADDR));
    memcpy(mID, ID, ID_LEN * sizeof(unsigned char));       
    
    //------------COSEMApp----------------//
    ResetCosemAppAssociation();
    
    //--------------HDLC-----------------//
    ResetHdlcConnection();
}


CStack62056::~CStack62056()
{
    //------------COSEMApp----------------//
    if(mApduBuff != NULL)
    {
        delete [] mApduBuff;
        mApduBuff = NULL;
    }

    if(mApduToHdlcBuff != NULL)
    {
        delete [] mApduToHdlcBuff;
        mApduToHdlcBuff = NULL;
    }
    
    if(mBlockRecvBuff != NULL)
    {
        delete [] mBlockRecvBuff;
        mBlockRecvBuff = NULL; 
    }
    
    //--------------HDLC-----------------//
    if(LongTransferReceive.Buff != NULL)
    {
        delete [] LongTransferReceive.Buff;
    }
    LongTransferReceive.Buff = NULL;

    if(LastIFrame.Infor != NULL)
    {
        delete [] LastIFrame.Infor;
    }
    LastIFrame.Infor = NULL;
    

    if(mOutFrame != NULL)
    {
        delete [] mOutFrame;
    }
    mOutFrame = NULL;

    //
    delete ase_gcm_128;
    //CoUninitialize();
}

SUPPORT_LAYER CStack62056::GetSupportLayerType()
{
    return mSupportLayerType;
}


ADDR CStack62056::GetAddr()
{
    return mAddr;
}

unsigned char * CStack62056::GetID()
{
    return mID;
}


//获得当前对象的使用状态
bool CStack62056::GetStackUsageFlag()
{
    if(mCosemAppState == IDLE && mMacState == NDM)
        return false;
    else
        return true;
};


int CStack62056::SetParams(const unsigned char *Xml, unsigned int XmlLen)
{
    /* by yanshiqi 
	_di_IXMLDocument InXml = NewXMLDocument("1.0");
    try
    {
        InXml->LoadFromXML(AnsiString((const char*)Xml));
    }
    catch( ... )
    {
        return 150;
    }
    
    CMarkup& InRoot = InXml->GetDocumentElement();
	*/

    return 1;
}

void CStack62056::PhyAbort()
{
    if(mMacState != NDM)
    {
        ResetHdlcConnection();
    }

    if(mCosemAppState != IDLE)
    {
        ResetCosemAppAssociation();
    }
}


//隶属于数据链路层特有的
int CStack62056::ResponseTimeOut(unsigned char **OUTData, unsigned int &OUTDataLen)
{
    int Result;
    //---
	CMarkup OutRoot;
	OutRoot.SetDoc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	OutRoot.AddElem("Root");

    //---
    
    if(mPermission == true)
    {
        return 10;
    }
    //根据当前状态
    if(mMacState == IDLE)
    {
        return 11;    
    }
    else if(mMacState == WAIT_CONNECTION)
    {
        if(RetryTimes <= MAX_RETRY_TIMES)
        {
            RetryTimes++;
            //重新发送SNRM帧
            MakeSNRM(OutRoot, true);
            Result = 0;
        }
        else
        {
            ResetHdlcConnection();
            Result = DlConnectCnf(OutRoot, CONNECT_NO_RESPONSE, NULL, 0);
        }
    }
    else if(mMacState == WAIT_DISCONNECTION)
    {
        if(RetryTimes <= MAX_RETRY_TIMES)
        {
            RetryTimes++;
            //重新发送SNRM帧
            MakeDISC(OutRoot, true);
            Result = 0;
        }
        else
        {
            ResetHdlcConnection();
            Result = DlDisconnectCnf(OutRoot, DISC_NO_RESPONSE, NULL, 0);
        }
    }
    else//NRM
    {
        if(RetringMode == false) //第一次超时
        {
            RetringMode = true;//进入I帧重发状态
            RetryMode = SEND_TEST_RR;
            RetryTimes++;
            MakeRR(OutRoot);  //发送RR帧
            Result = 0;

        }
        else//后续帧依然超时
        {
            if(RetryMode == SEND_TEST_RR)
            {
                if(RetryTimes <= MAX_RETRY_TIMES)
                {
                    RetryTimes++;//说明是表已收到主台的帧，但未响应的情形，继续等待，直到MAX_RETRY_TIMES倍超时时间
                    Result = 0;
                }
                else
                {
                    RetringMode = false;
                    RetryTimes = 0;
                    return 12;
                }
            }
            else//上一帧发的是重发帧，即发RR有响应，但I没响应，
            {
                RetringMode = false;
                RetryTimes = 0;
                return 13;    
            }  
        }
    }

    if(Result > 0)
    {
        *OUTData = NULL;
        OUTDataLen = 0;
        return Result;
    }
    else //if(Result == 0 || Result == -1)
    {
        XmlToStr(OutRoot, OUTData, OUTDataLen);
        return 0;
    }    
}

int CStack62056::ProcessServicePrimitive(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    const unsigned char *Xml, unsigned int XmlLen)
{
	CMarkup InRoot;
    try
    {
		InRoot.SetDoc((const char*)Xml);
    }
    catch(...)
    {
        return 150;
    }
    
	InRoot.FindElem();
    mXmlLen = XmlLen;
    //
    
    int Result; 
    //---
	CMarkup OutRoot;
	OutRoot.SetDoc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	OutRoot.AddElem("Root");
    //---
    std::string RootName = InRoot.GetTagName();
    if(RootName == "COSEM_OPEN_REQ")
    {
        Result = ProcessCosemOpenReq(OutRoot, InRoot);
    }
    else if(RootName == "COSEM_RELEASE_REQ")
    {
        Result = ProcessCosemReleaseReq(OutRoot, InRoot);
    }
    else if(RootName == "GET_REQ")
    {
        Result = ProcessGetReq(OutRoot, InRoot);
    }
    else if(RootName == "SET_REQ")
    {
        Result = ProcessSetReq(OutRoot, InRoot);
    }
    else if(RootName == "ACTION_REQ")
    {
        Result = ProcessActionReq(OutRoot, InRoot);
    }
    else if(RootName == "TRIGGER_EVENTNOTIFICATION_SENDING_REQ")
    {
        Result = ProcessTriggerEventNotificationSendingReq(OutRoot, InRoot);
    }
    else if(RootName == "READ_REQ")
    {
        Result = ProcessReadReq(OutRoot, InRoot);
    }
    else if(RootName == "WRITE_REQ")
    {
        Result = ProcessWriteReq(OutRoot, InRoot, true);
    }
    else if(RootName == "UNCONFIRMED_WRITE_REQ")
    {
        Result = ProcessWriteReq(OutRoot, InRoot, false);
    }
    else
    {
        return 301;   
    }


    if(Result > 0)
    {
        *OUTData = NULL;
        OUTDataLen = 0;
        return Result;
    }
    else //if(Result == 0 || Result == -1)
    {
        XmlToStr(OutRoot, OUTData, OUTDataLen);
        return 0;
    }
}

int CStack62056::ProcessHDLCFrame(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    bool Segment, unsigned char RecvNo, unsigned char SendNo,
                                    bool PorF, FRAME_COMMAND FrameCommand,
                                    const unsigned char *Infor, unsigned int InforLen)
{
    if(FrameCommand != UI_FRAME)
    {
        if(mPermission == true)
        {
            return 199;
        }
        if(InforLen > (unsigned int)HdlcParamsNegotiated.MaximumInformationFieldLengthReceive)
        {
            return 200;
        }
    }
    else
    {
        if(InforLen > (unsigned int)HdlcParamsDefault.MaximumInformationFieldLengthReceive)
        {
            return 200;
        }    
    }

    //---
	CMarkup OutRoot;
	OutRoot.SetDoc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	OutRoot.AddElem("Root");
    //---

    if(PorF)    mPermission = true;
    //mOutDataLen = 0;
    int Result;
    /*
    I_FRAME = 0x00,
	UI_FRAME = 0x03,
	RR_FRAME = 0x11,
	RNR_FRAME = 0x15,
	DM_FRAME = 0x1F,
	DISC_FRAME = 0x53,
	UA_FRAME = 0x73,
	SNRM_FRAME = 0x93,
	FRMR_FRAME = 0x97
    */
    switch(FrameCommand)
    {
        case I_FRAME:Result = ProcessI(OutRoot, Segment, RecvNo, SendNo, PorF, Infor, InforLen);
        break;
        case UI_FRAME:Result = ProcessUI(OutRoot, PorF, Infor, InforLen);
        break;
        case RR_FRAME:Result = ProcessRR(OutRoot, RecvNo);break;
        case RNR_FRAME:break;
        case DM_FRAME:Result = ProcessDM(OutRoot, Infor, InforLen);break;
        case UA_FRAME:Result = ProcessUA(OutRoot, Infor, InforLen);break; 
        case FRMR_FRAME:break;

        case DISC_FRAME:Result = 209; break;
        case SNRM_FRAME:Result = 210;break;//客户端不能收到该类型的帧
        default:break;//由前面的代码限制了该FrameCommand肯定是可识别的,否则已经返回110,此处不做任何处理
    }

    if(Result > 0)
    {
        *OUTData = NULL;
        OUTDataLen = 0;
        return Result;
    }
    else //if(Result == 0 || Result == -1)
    {
        XmlToStr(OutRoot, OUTData, OUTDataLen);
        return 0;
    }

}

int CStack62056::ProcessUdpWpdu(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    const unsigned char *lpPdu, unsigned int PduLen)
{
    //---
	CMarkup OutXml;
	OutXml.SetDoc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	OutXml.AddElem("Root");
    //---

    int Result = UdpDataInd(OutXml, lpPdu, PduLen);
    if(Result > 0)
    {
        *OUTData = NULL;
        OUTDataLen = 0;
        return Result;
    }
    else //if(Result == 0 || Result == -1)
    {
        XmlToStr(OutXml, OUTData, OUTDataLen);
        return 0;
    }

}

//新增  与加密相关
unsigned char CStack62056::SetSecurity(SECURITY_OPTION SecurityOption)
{
    if(SecurityOption.SecurityPolicy > ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED)
    {
        return 14;
    }

    if(SecurityOption.SecuritySuite != AES_GCM_128)
    {
        return 15;
    }
    memcpy((unsigned char *)&mSecurityOption, (unsigned char *)&SecurityOption, sizeof(SECURITY_OPTION));
    return 0;
}
//****************************************************************//
//*******************以下为private函数****************************//
//****************************************************************//
void CStack62056::InitSecurity()
{
    unsigned char FC[FC_LEN] = {0x00, 0x00, 0x00, 0x01};
    mSecurityOption.SecurityPolicy = NO_SECURITY;
    mSecurityOption.SecuritySuite = AES_GCM_128;
    memcpy(mSecurityOption.SecurityMaterial.MK, DefaultMK, KEY_LEN);
    memcpy(mSecurityOption.SecurityMaterial.EK, DefaultEK, KEY_LEN);
    memcpy(mSecurityOption.SecurityMaterial.AK, DefaultAK, KEY_LEN);
    memset(mSecurityOption.SecurityMaterial.IV.IV, 0, IV_LEN);
    memcpy(mSecurityOption.SecurityMaterial.IV.params.FC, FC, FC_LEN);
}

void CStack62056::InitCosemAppParams()
{
    mCosemAppState = IDLE;

    InitAarqParams();

    //
    mBlockTransfer.Flag = false;
    mBlockTransfer.BlockNumber = 0;
    mBlockTransfer.Offset = 0; 

    //------------------------------
    mApduBuff = NULL;
    mApduLen = 0;
    //

    mApduToHdlcBuff = NULL;
    mApduToHdlcLen = 0;
    //
                
    mBlockRecvBuff = NULL;
    mBlockRecvLen = 0;
    mBlockRecvBuffSize = 0;

    mClientMaxSendPduSize = DEFAULT_CLIENT_MAX_RECEIVED_PDU_SIZE + 3;
}

void CStack62056::InitAarqParams()
{
    mAarqParams.ApplicationContextName = LN_NOCIPER;
    mAarqParams.SenderAcseRequirementsPresent = false;
    mAarqParams.SenderAcseRequirements = false;
    mAarqParams.MechanismName = LOWEST_LEVEL;
    //mAarqParams.CallingAuthenticationValue = "";
    mAarqParams.CallingAuthenticationValueLen = 0;
    //mAarqParams.XdlmsInitiateReq.DedicatedKey = "";
    mAarqParams.XdlmsInitiateReq.DedicatedKeyLen = 0;
    mAarqParams.XdlmsInitiateReq.ResponseAllowed = false;
    mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize = DEFAULT_CLIENT_MAX_RECEIVED_PDU_SIZE;
    mAarqParams.XdlmsInitiateReq.DlmsVersionNumber = 6;
    mAarqParams.XdlmsInitiateReq.Conformance.Val[0] = 0x1C;
    mAarqParams.XdlmsInitiateReq.Conformance.Val[1] = 0xFE;
    mAarqParams.XdlmsInitiateReq.Conformance.Val[2] = 0x7F;
}

void CStack62056::InitRlrqParams()
{
    mRlrqParams.ReleaseRequestReasonPresent = true;
    mRlrqParams.ReleaseRequestReason = RLRQ_REASON_NORMAL;
}

void CStack62056::InitHdlcParams()
{
    mMacState = NDM;
    //此为协议默认值，HdlcParamsDefault可以由客户端应用进程通过接口进行修改
    //此修改须在连接建立之前，建立连接后，修改要到下一次建立连接时才会生效
    HdlcParamsDefault.WindowSizeTransmit = DEFAULT_WINDOWSIZE_TRANSMIT;
    HdlcParamsDefault.WindowSizeReceive = DEFAULT_WINDOWSIZE_RECEIVE;
    HdlcParamsDefault.MaximumInformationFieldLengthTransmit = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT;
    HdlcParamsDefault.MaximumInformationFieldLengthReceive = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_RECEIVE;

    HdlcParamsNegotiated.WindowSizeTransmit = DEFAULT_WINDOWSIZE_TRANSMIT;
    HdlcParamsNegotiated.WindowSizeReceive = DEFAULT_WINDOWSIZE_RECEIVE;
    HdlcParamsNegotiated.MaximumInformationFieldLengthTransmit = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT;
    HdlcParamsNegotiated.MaximumInformationFieldLengthReceive = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_RECEIVE;
  
    mRecvNo = mSendNo = 0;
    mPermission = true;

    RetringMode = false;
    RetryTimes = 0;

    //参数 与 缓冲区尺寸 之间的逻辑关系
    //用于缓存服务器到客户端的分帧
    //缓冲区的大小为mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize
    //当接收到第一个分帧时才申请内存
    LongTransferReceive.Flag = false;
    LongTransferReceive.Buff = NULL;
    LongTransferReceive.Len = 0;

    #ifdef CLIENT_LONG_TRANSFER_SUPPORT
    LongTransferSend.Flag = false;
    LongTransferSend.Buff = NULL;
    LongTransferSend.Len = 0;
    LongTransferSend.Offset = 0;
    #endif
    
    LastIFrame.Segment = false;
    LastIFrame.PorF = true;
    LastIFrame.SendNo = 0;
    LastIFrame.RecvNo = 0;
    LastIFrame.Infor = NULL;
    LastIFrame.InforLen = 0;
    //mOutFrame, mOutFrameLen用于存放真正输出给应用进程发送的帧
    //HDLC层发送缓冲区尺寸 <= HdlcParamsDefault.MaximumInformationFieldLengthTransmit + MAX_ENCAPSULATION_LEN
    //因为协商后的参数只会比该值小,不会比该值大
    //当用户修改了HdlcParamsDefault.MaximumInformationFieldLengthTransmit后,该缓冲区应删除再重新申请
    //mOutFrame = NULL;//new unsigned char[HdlcParamsDefault.MaximumInformationFieldLengthTransmit + MAX_ENCAPSULATION_LEN];
    //mOutFrameLen = 0;
    //此时分配mOutFrame空间

    mOutFrame = new unsigned char[HdlcParamsDefault.MaximumInformationFieldLengthTransmit + MAX_ENCAPSULATION_LEN];
    if(mOutFrame == NULL)
    {

    } 
    mOutFrameLen = 0;
}


//说明 init函数只在类对象创建的时候调用,连接断开后类对象是没有被删除的,可以被再次使用
//故而连接断开时,将资源释放,并把参数重置,这样再次使用时可以直接用
void CStack62056::ResetCosemAppAssociation()
{
    mCosemAppState = IDLE;

    InitAarqParams();
    InitRlrqParams();

    //
    mBlockTransfer.Flag = false;
    mBlockTransfer.BlockNumber = 0;
    mBlockTransfer.Offset = 0;

    if(mApduBuff != NULL)
    {
        delete [] mApduBuff;
        mApduBuff = NULL;
    }
    mApduLen = 0;

    if(mApduToHdlcBuff != NULL)
    {
        delete [] mApduToHdlcBuff;
        mApduToHdlcBuff = NULL;
    }
    mApduToHdlcLen = 0;


    mBlockReceive.Flag = false;
    mBlockReceive.BlockNumber = 0;

    if(mBlockRecvBuff != NULL)
    {
        delete [] mBlockRecvBuff;
        mBlockRecvBuff = NULL; 
    }
    mBlockRecvLen = 0;
    mBlockRecvBuffSize = 0;

    mClientMaxSendPduSize = DEFAULT_CLIENT_MAX_RECEIVED_PDU_SIZE + 3;
}

void CStack62056::ResetHdlcConnection()
{
    mMacState = NDM;
    //此为协议默认值，HdlcParamsDefault可以由客户端应用进程通过接口进行修改
    //此修改须在连接建立之前，建立连接后，修改要到下一次建立连接时才会生效
    HdlcParamsDefault.WindowSizeTransmit = DEFAULT_WINDOWSIZE_TRANSMIT;
    HdlcParamsDefault.WindowSizeReceive = DEFAULT_WINDOWSIZE_RECEIVE;
    HdlcParamsDefault.MaximumInformationFieldLengthTransmit = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT;
    HdlcParamsDefault.MaximumInformationFieldLengthReceive = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_RECEIVE;

    HdlcParamsNegotiated.WindowSizeTransmit = DEFAULT_WINDOWSIZE_TRANSMIT;
    HdlcParamsNegotiated.WindowSizeReceive = DEFAULT_WINDOWSIZE_RECEIVE;
    HdlcParamsNegotiated.MaximumInformationFieldLengthTransmit = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT;
    HdlcParamsNegotiated.MaximumInformationFieldLengthReceive = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_RECEIVE;
  
    mRecvNo = mSendNo = 0;
    mPermission = true;

    LongTransferReceive.Flag = false;
    if(LongTransferReceive.Buff != NULL)
    {
        delete [] LongTransferReceive.Buff;
    }
    LongTransferReceive.Buff = NULL;
    LongTransferReceive.Len = 0;

    #ifdef CLIENT_LONG_TRANSFER_SUPPORT
    LongTransferSend.Flag = false;
    LongTransferSend.Buff = NULL;
    LongTransferSend.Len = 0;
    LongTransferSend.Offset = 0;
    #endif
    
    LastIFrame.Segment = false;
    LastIFrame.PorF = true;
    LastIFrame.SendNo = 0;
    LastIFrame.RecvNo = 0;
    if(LastIFrame.Infor != NULL)
    {
        delete [] LastIFrame.Infor;
    }
    LastIFrame.Infor = NULL;  
    LastIFrame.InforLen = 0;


    if(mOutFrame != NULL)
    {
        delete [] mOutFrame;
    }
    mOutFrame = new unsigned char[HdlcParamsDefault.MaximumInformationFieldLengthTransmit + MAX_ENCAPSULATION_LEN];
    if(mOutFrame == NULL)
    {

    } 
    mOutFrameLen = 0;
}
//------------------------COSEMApp-------------------------------//
//COSEMApp层提供给HDLC层调用的服务函数
int CStack62056::DlConnectCnf(CMarkup& OutRoot, DL_CONNECT_RESULT ConnResult, const unsigned char *lpInfor, unsigned int InforLen)
{
    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == false)
    {
        return 701;
    }

    if(ConnResult == CONNECT_OK)
    {
		Sleep(50);
        int Result = MakeAarqApdu();
        if(Result != 0)
        {
            return Result;
        }

        Result = DlDataReq(OutRoot, I_COMPLETE, mApduBuff, mApduLen);
        if(Result != 0)
        {
            return Result;
        }

        return Result;
    }
    else
    {
        WriteCosemOpenCnf(OutRoot, ConnResult);

        return -1;
    }
}

int CStack62056::DlDataInd(CMarkup& OutRoot, FRAME_TYPE FramType, const unsigned char *lpPdu, unsigned int PduLen)
{
    if(lpPdu == NULL || PduLen == 0)
    {
        return 702;
    }
    //get,set,action cnf
    switch(*lpPdu++)
    {
        case 0x61:return ProcessAARE(OutRoot, lpPdu, PduLen - 1);
        
        case EVENT_NOTIFICATION_REQUEST:return ProcessEventNotificationInd(OutRoot, lpPdu, PduLen - 1);

        case PUSH_FRAME:return ProcessPushData(OutRoot, lpPdu, PduLen - 1);

        case GET_RESPONSE:return ProcessGetRes(OutRoot, lpPdu, PduLen - 1);

        case GLO_GET_RESPONSE: return ProcessGloRes(OutRoot, GET_RESPONSE, lpPdu, PduLen - 1);
		
        case ded_get_response: return ProcessGloRes(OutRoot, ded_get_response, lpPdu, PduLen - 1);

        case SET_RESPONSE:return ProcessSetRes(OutRoot, lpPdu, PduLen - 1);

        case GLO_SET_RESPONSE:return ProcessGloRes(OutRoot, SET_RESPONSE, lpPdu, PduLen - 1);
		
        case ded_set_response:return ProcessGloRes(OutRoot, ded_set_response, lpPdu, PduLen - 1);
		
        case ACTION_RESPONSE:return ProcessActionRes(OutRoot, lpPdu, PduLen - 1);
        
        case GLO_ACTION_RESPONSE:return ProcessGloRes(OutRoot, ACTION_RESPONSE, lpPdu, PduLen - 1);

        case ded_action_response:return ProcessGloRes(OutRoot, ded_action_response, lpPdu, PduLen - 1);

        //
        case READ_RESPONSE:return ProcessReadRes(OutRoot, lpPdu, PduLen - 1);

        case WRITE_RESPONSE:return ProcessWriteRes(OutRoot, lpPdu, PduLen - 1);

        case INFORMATION_REPORT_REQUEST:return ProcessInformationReportInd(OutRoot, lpPdu, PduLen - 1);

        case CONFIRMED_SERVICE_ERROR:return ProcessConfirmedServiceError(OutRoot, lpPdu, PduLen - 1);
        
        default:return 501;
    }
}

int CStack62056::DlDisconnectInd(CMarkup& OutRoot, DL_DISC_REASON Reason, bool Uss, const unsigned char *lpInfor, unsigned int InforLen)
{
    //cosem_abort_ind
    return -1;
}

int CStack62056::DlDisconnectCnf(CMarkup& OutRoot, DL_DISC_RESULT Result, const unsigned char *lpInfor, unsigned int InforLen)
{
    WriteCosemReleaseCnf(OutRoot, Result);

    //应用程序默认已经断开
    ResetCosemAppAssociation();

    return -1;
}

//--------------------------------------------------------------------
//COSEMApp层提供给UDP Wrapper层调用的服务函数
int CStack62056::UdpDataInd(CMarkup& OutRoot, const unsigned char *lpPdu, unsigned int PduLen)
{
    if(lpPdu == NULL || PduLen == 0)
    {
        return 242;
    }

    //get,set,action cnf
    switch(*lpPdu++)
    {
        case 0x61:return ProcessAARE(OutRoot, lpPdu, PduLen - 1);

        case 0x63:return ProcessRLRE(OutRoot, lpPdu, PduLen - 1);
        
        case EVENT_NOTIFICATION_REQUEST:return ProcessEventNotificationInd(OutRoot, lpPdu, PduLen - 1);

        case GET_RESPONSE:return ProcessGetRes(OutRoot, lpPdu, PduLen - 1);

        case GLO_GET_RESPONSE: return ProcessGloRes(OutRoot, GET_RESPONSE, lpPdu, PduLen - 1);

        case SET_RESPONSE:return ProcessSetRes(OutRoot, lpPdu, PduLen - 1);

        case GLO_SET_RESPONSE:return ProcessGloRes(OutRoot, SET_RESPONSE, lpPdu, PduLen - 1);

        case ACTION_RESPONSE:return ProcessActionRes(OutRoot, lpPdu, PduLen - 1);

        case GLO_ACTION_RESPONSE:return ProcessGloRes(OutRoot, ACTION_RESPONSE, lpPdu, PduLen - 1);

        //
        case READ_RESPONSE:return ProcessReadRes(OutRoot, lpPdu, PduLen - 1);

        case WRITE_RESPONSE:return ProcessWriteRes(OutRoot, lpPdu, PduLen - 1);

        case INFORMATION_REPORT_REQUEST:return ProcessInformationReportInd(OutRoot, lpPdu, PduLen - 1);

        case CONFIRMED_SERVICE_ERROR:return ProcessConfirmedServiceError(OutRoot, lpPdu, PduLen - 1);
        
        default:return 501;
    }
}

//返回值 = 0,正确, >0 错误码, -1需要生成XML文档
int CStack62056::ProcessCosemOpenReq(CMarkup& OutRoot, CMarkup& InRoot)
{   
    //判断前提条件和环境
    if(mCosemAppState != IDLE)
    {
        //重复AA请求
        //拒绝!
        return 303;
    }
        

    int Result = ParseAarq(InRoot);
    if(Result > 0)
    {
        return Result;
    }



    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == true
    && (mAddr.HDLC_ADDR1.ServerUpperAddr == ALL_STATION_ADDRESS || mAddr.HDLC_ADDR1.ServerLowerAddr == ALL_STATION_ADDRESS))
    {
        return 300;
    }
    
    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == true)
    {
        mCosemAppState = ASSOCIATION_PENDING;

        if(mSupportLayerType == HDLC)
        {
            //解析正确，调用hdlc层服务

            return DlConnectReq(OutRoot, NULL, 0);
        }
        else if(mSupportLayerType == UDPIP)
        {
            Result = MakeAarqApdu();
            if(Result != 0)
            {
                return Result;
            }

            return UdpDataReq(OutRoot, mApduBuff, mApduLen);
        }
        else
        {
            return 1;
        }
    }
    else
    {
        if(mSupportLayerType == HDLC)
        {
            //解析正确，调用hdlc层服务    
            Result = DlConnectReq(OutRoot, NULL, 0);
            if(Result != 0)
            {
                return Result;
            }

             //
            Result = MakeAarqApdu();
            if(Result != 0)
            {
                return Result;
            }
            /*
	     memcpy(ase_gcm_128->S,mAarqParams.CallingAuthenticationValue,MAX_PASSWORD_LEN);
            ase_gcm_128->Encrypt_StringData(SC_B_A, mSecurityOption.SecurityMaterial.BK, mAarqParams.CallingAPTitle, mSecurityOption.SecurityMaterial.IV.params.FC, 
	        mSecurityOption.SecurityMaterial.AK, mAareParams.RespondingAuthenticationValueLen, ase_gcm_128->T);

       // ase_gcm_128->AES_GCM_Encryption(TYPE_ENCRYPTION, SC_U_A, mAarqParams.CallingAPTitle,
        //                                    mSecurityOption.SecurityMaterial.IV.params.FC, mSecurityOption.SecurityMaterial.EK,
       //                                     mSecurityOption.SecurityMaterial.AK, mAareParams.RespondingAuthenticationValue, mAareParams.RespondingAuthenticationValueLen);

          mAarqParams.F_StoC[0] = SC_B_A;
          memcpy(mAarqParams.F_StoC + SC_LEN, mSecurityOption.SecurityMaterial.IV.params.FC, FC_LEN);
          memcpy(mAarqParams.F_StoC + SH_LEN, ase_gcm_128->T, T_LEN);
	     Result = MakeHLSApdu();
            if(Result != 0)
            {
                return Result;
             }
			*/
            Result = DlDataReq(OutRoot, UI, mApduBuff, mApduLen);
            if(Result != 0)
            {
                return Result;
            }

            //构造一个cosem-open-cnf 文档
            WriteCosemOpenCnf(OutRoot, CONNECT_OK);

            mCosemAppState = ASSOCIATED;
            
            return 0;//Result;
        }
        else if(mSupportLayerType == UDPIP)
        {
            Result = MakeAarqApdu();
            if(Result != 0)
            {
                return Result;
            }

            Result = UdpDataReq(OutRoot, mApduBuff, mApduLen);
            if(Result != 0)
            {
                return Result;
            }

            //构造一个cosem-open-cnf 文档
            WriteCosemOpenCnf(OutRoot, CONNECT_OK);

            mCosemAppState = ASSOCIATED;
            
            return 0;//Result;  
        }
        else
        {
            return 1;
        }      
    }
};


int CStack62056::ParseAarq(CMarkup& Root)
{
    //解析AARQ
    std::string strtmp;
	Root.IntoElem();
	bool bRet = Root.FindElem("ACSE_Protocol_Version");
    if(!(!bRet || (bRet && Root.GetAttrib("Value") == "80")))//
    {
        return 304;
    }

	bRet = Root.FindElem("Application_Context_Name");
    if(bRet == false)//
    {
        //return 305;
    }
    else
    {
		strtmp = Root.GetAttrib("Value");
        if(strtmp != "1" && strtmp != "2" && strtmp != "3" && strtmp != "4")
        {
            return 305;
        }

        mAarqParams.ApplicationContextName = (APPLICATION_CONTEXT_NAME)atoi(strtmp.c_str());
    }

    

//    Node = Root->ChildNodes->FindNode("Sender_Acse_Requirement");
//    if(Node == NULL)
//    {
//        mAarqParams.SenderAcseRequirementsPresent = false;
//        mAarqParams.MechanismName = LOWEST_LEVEL;
//    }
//    else
//    {
//        mAarqParams.SenderAcseRequirementsPresent = true;
//        strtmp = Node->GetAttribute("Value");
//        if(strtmp == "00")
//        {
//            mAarqParams.SenderAcseRequirements = false;
//            mAarqParams.MechanismName = LOWEST_LEVEL;
//        }
//        else if(strtmp == "80")
//        {
//            mAarqParams.SenderAcseRequirements = true;
//            Node = Root->ChildNodes->FindNode("Security_Mechanism_Name");
//            if(Node == NULL)
//            {
//                //return 307;
//            }
//            else
//            {
//                strtmp = Node->GetAttribute("Value");
//                if(strtmp != "1" && strtmp != "2" && strtmp != "3" && strtmp != "4" && strtmp != "5")
//                {
//                    return 308;
//                }
//                else
//                {
//                    mAarqParams.MechanismName = (SECURITY_MECHANISM_NAME)StrToInt(strtmp);
//                    Node = Root->ChildNodes->FindNode("Calling_Authentication_Value");
//                    if(Node == NULL)
//                    {
//                        return 309;
//                    }
//                    else
//                    {
//                        strtmp = (AnsiString)Node->GetAttribute("Value");
//                        if(strtmp.Length() > MAX_PASSWORD_LEN)
//                        {
//                            return 310;
//                        }
//                        else
//                        {
//                            memset(mAarqParams.CallingAuthenticationValue, 0, MAX_PASSWORD_LEN * sizeof(unsigned char));
//                            mAarqParams.CallingAuthenticationValueLen = strtmp.Length();
//                            if(mAarqParams.CallingAuthenticationValueLen > 0)
//                            {
//                                memcpy(mAarqParams.CallingAuthenticationValue, strtmp.c_str(), strtmp.Length());
//                            }
//                        }
//                    }
//                }
//            }
//        }
//        else
//        {
//            return 306;
//        }
//    }

    bRet = Root.FindElem("Sender_Acse_Requirement");
    if(bRet == false)
    {
        mAarqParams.SenderAcseRequirementsPresent = false;
        mAarqParams.MechanismName = LOWEST_LEVEL;
    }
    else
    {
        mAarqParams.SenderAcseRequirementsPresent = true;
        strtmp = Root.GetAttrib("Value");
        if(strtmp == "00")
        {
            mAarqParams.SenderAcseRequirements = false;
            mAarqParams.MechanismName = LOWEST_LEVEL;
        }
        else if(strtmp == "80")
        {
            mAarqParams.SenderAcseRequirements = true;
        }
    }

    if(mAarqParams.SenderAcseRequirements == true)
    {
        bRet = Root.FindElem("Security_Mechanism_Name");
        if(bRet == false)
        {
            //return 307;
            mAarqParams.SenderAcseRequirementsPresent = false;
            mAarqParams.SenderAcseRequirements = false;
            mAarqParams.MechanismName = LOWEST_LEVEL;
        }
        else
        {
            strtmp = Root.GetAttrib("Value");
            if(strtmp != "0" && strtmp != "1" && strtmp != "5") //only support LOWEST_LEVEL & LOW_LEVEL & HIGH_LEVEL_CMAC
            {
                return 308;
            }
            else
            {
                mAarqParams.MechanismName = (SECURITY_MECHANISM_NAME)atoi(strtmp.c_str());
                if(mAarqParams.MechanismName == LOWEST_LEVEL)
                {
                    mAarqParams.SenderAcseRequirementsPresent = false;
                    mAarqParams.SenderAcseRequirements = false;
                }
                else //if(mAarqParams.MechanismName == LOW_LEVEL || (mAarqParams.MechanismName == HIGH_LEVEL_CMAC)
                {
                    mAarqParams.SenderAcseRequirementsPresent = true;
                    mAarqParams.SenderAcseRequirements = true;
                    bRet = Root.FindElem("Calling_Authentication_Value");
                    if(bRet == false)
                    {
                        return 309;
                    }
                    else
                    {
                        strtmp = Root.GetAttrib("Value");
                        if(strtmp.length() > MAX_PASSWORD_LEN)
                        {
                            return 310;
                        }
                        else
                        {
                            memset(mAarqParams.CallingAuthenticationValue, 0, MAX_PASSWORD_LEN * sizeof(unsigned char));
                            mAarqParams.CallingAuthenticationValueLen = strtmp.length();
                            if(mAarqParams.CallingAuthenticationValueLen > 0)
                            {
                                memcpy(mAarqParams.CallingAuthenticationValue, strtmp.c_str(), strtmp.length());
                            }
                        }
                    }
                }
            }
        }
    }


    if((mAarqParams.ApplicationContextName == LN_CIPER || mAarqParams.ApplicationContextName == SN_CIPER)
    || (mAarqParams.MechanismName > LOW_LEVEL))
    {
        bRet = Root.FindElem("Calling_AP_Title");
        if(bRet == false)//
        {
            return 381;
        }
        else
        {
             strtmp = Root.GetAttrib("Value");//可视字符串

            //前三个字节为厂商标识码，威胜为WSE   
                 for(unsigned char i = 0; i < 8; i++)
                {
                    mAarqParams.CallingAPTitle[i] = HexToInt(strtmp.substr(i * 2+1, 2));
                }                 
            //mAarqParams.CallingAPTitle="W";
            //mAarqParams.CallingAPTitle+1="S";
            //mAarqParams.CallingAPTitle+2="E";
            //后面的数据ManNum为Manufacturer Number
           /* __int64 ManNum;
            try
            {
                ManNum = StrToInt64(strtmp.SubString(4, strtmp.Length() - 3));
            }
            catch(EConvertError &e)
            {
                return 382; 
            }
            ManMumIntToStr(mAarqParams.CallingAPTitle + MAN_ID_LEN, ManNum, MAN_NUM_LEN); */
        }
    }


    
    bRet = Root.FindElem("Dedicated_Key");
    if(bRet == false)
    {
       mAarqParams.XdlmsInitiateReq.DedicatedKeyLen = 0;
    }
    else
    {
        strtmp = Root.GetAttrib("Value");
        if(strtmp.length() == 0)
        {
            mAarqParams.XdlmsInitiateReq.DedicatedKeyLen = 0;
        }
        else
        {  
            if(strtmp.length() <= (MAX_DEDICATEDKEY_LEN * 2) && strtmp.length() % 2 == 0 && IsHex(strtmp))
            {
                mAarqParams.XdlmsInitiateReq.DedicatedKeyLen = (unsigned char)strtmp.length() / 2;
                for(unsigned char i = 0; i < mAarqParams.XdlmsInitiateReq.DedicatedKeyLen; i++)
                {
                    mAarqParams.XdlmsInitiateReq.DedicatedKey[i] = HexToInt(strtmp.substr(i * 2+1, 2));
                }                 
            }
            else
            {
                return 312;
            }
        }
    }


    bRet = Root.FindElem("DLMS_Version_Number");  
    if(bRet != false)
    {
        int DlmsVersionNumber;
        strtmp = Root.GetAttrib("Value");
        try
        {
            DlmsVersionNumber = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 313;
        }
        if(DlmsVersionNumber > 255 || DlmsVersionNumber < 6)
        {
            return 313;
        }
        
        mAarqParams.XdlmsInitiateReq.DlmsVersionNumber = DlmsVersionNumber;
    }
    else//(Node == NULL)
    {
        //mAarqParams.XdlmsInitiateReq.DlmsVersionNumber = 6;
    }

    bRet = Root.FindElem("DLMS_Conformance");
    if(bRet != false)
    {
        strtmp = Root.GetAttrib("Value");
        if(strtmp.length() == 6 && IsHex(strtmp))
        {
            EnCodeSizedOctetStr(strtmp, 3, mAarqParams.XdlmsInitiateReq.Conformance.Val);
        }
        else
        {
            return 315;
        }
    }
    else//(Node == NULL)
    {
        //或者使用缺省值
        //return 314;
    }


    bRet = Root.FindElem("Client_Max_Receive_PDU_Size");
    if(bRet != false)
    {
        int ClientMaxReceivedPduSize;
        strtmp = Root.GetAttrib("Value");
        try
        {
            ClientMaxReceivedPduSize = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 316;
        }
        if(ClientMaxReceivedPduSize < 0 || ClientMaxReceivedPduSize > 0xFFFF)
        {
            return 316;
        }

        //如果应用进程给出的参数小于该协议栈最大接收缓冲区长度,则使用协议栈最大接收缓冲区长度
        if(ClientMaxReceivedPduSize == 0 || ClientMaxReceivedPduSize > DEFAULT_CLIENT_MAX_RECEIVED_PDU_SIZE)
        {
            mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize = ClientMaxReceivedPduSize;
        }
        else
        {
            mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize = DEFAULT_CLIENT_MAX_RECEIVED_PDU_SIZE;
        }
    }
    else//(Node == NULL)
    {
        //或者使用缺省值
        //return 316;
    }


    bRet = Root.FindElem("Service_Class");
    if(bRet == false)
    {
        //或者使用缺省值
        //return 314;
        mAarqParams.XdlmsInitiateReq.ResponseAllowed = true;
    }
    else
    {
        strtmp = Root.GetAttrib("Value");
        if(strtmp == "1")
        {
            mAarqParams.XdlmsInitiateReq.ResponseAllowed = true;
        }
        else if(strtmp == "0")
        {
            mAarqParams.XdlmsInitiateReq.ResponseAllowed = false;

            //----------------------------------------------------------
            mApduToHdlcBuff = new unsigned char[mClientMaxSendPduSize];
            mApduToHdlcLen = 0;
            //--------------------------------------------------------------
        }
        else
        {
            return 311;
        }
    }
    return 0;
}

int CStack62056::ParseRlrq(CMarkup& Root)
{
    //解析RLRQ
    std::string strtmp;
    bool bRet = Root.FindChildElem("RELEASE_REQUEST_REASON");
    if(bRet != false)//
    {
		Root.IntoElem();
		std::string strValue = Root.GetAttrib("Value");
        unsigned char reason = (unsigned char)atoi(strValue.c_str());
		Root.OutOfElem();
        if(reason != RLRQ_REASON_NORMAL && reason != RLRQ_REASON_URGENT && reason != RLRQ_REASON_USER_DEFINED)
        {
            return 400;
        }
        mRlrqParams.ReleaseRequestReasonPresent = true;
        mRlrqParams.ReleaseRequestReason = (RLRQ_REASON)reason;
    }
    else
    {
        mRlrqParams.ReleaseRequestReasonPresent = false;
    }
    return 0;
}

int CStack62056::ProcessCosemReleaseReq(CMarkup& OutRoot, CMarkup& InRoot)
{
    int Result = ParseRlrq(InRoot);
    if(Result > 0)
    {
        return Result;
    }
    
    //直接调用下层服务
    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == true)
    {
        mCosemAppState = ASSOCIATION_RELEASE_PENDING;
        if(mSupportLayerType == HDLC)
        {
            return DlDisconnectReq(OutRoot, NULL, 0);
        }
        else if(mSupportLayerType == UDPIP)
        {
            Result = MakeRlrqApdu();
            if(Result > 0)
            {
                return Result;
            }

            Result = UdpDataReq(OutRoot, mApduBuff, mApduLen);
            if(Result != 0)
            {
                return Result;
            }

            return Result;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        if(mSupportLayerType == HDLC)
        {
            int Result = DlDisconnectReq(OutRoot, NULL, 0);
            if(Result != 0)
            {
                return Result;
            }

            WriteCosemReleaseCnf(OutRoot, DISC_OK);

            //应用程序默认已经断开
            ResetCosemAppAssociation();

            return -1;
        }
        else if(mSupportLayerType == UDPIP)
        {
            Result = MakeRlrqApdu();
            if(Result > 0)
            {
                return Result;
            }

            Result = UdpDataReq(OutRoot, mApduBuff, mApduLen);
            if(Result != 0)
            {
                return Result;
            }

            WriteCosemReleaseCnf(OutRoot, DISC_OK);

            //应用程序默认已经断开
            ResetCosemAppAssociation();

            return -1;
        }
        else
        {
            return 1;
        }
    }
}

int CStack62056::ProcessGetReq(CMarkup& OutRoot, CMarkup& InRoot)
{
    //1.检查状态
    if(mAareParams.MechanismName > LOW_LEVEL)
    {
        if(mCosemAppState != HLS_ASSOCIATED)
        {
            return 319;
        }
    }
    else
    {
        if(mCosemAppState != ASSOCIATED)
        {
            return 319;
        }
    }

    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == SN_NOCIPER || mAarqParams.ApplicationContextName == SN_CIPER)
    {
        return 318;
    }

    //3.检查一致性模块
    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Get == 0)
    {
        return 318;
    }

    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == false)
    {
        return 330;
    }
    //---------------------------------------
    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;
    int InvokeId;
    bool Priority;
    SERVICE_CLASS ServiceClass;
    int RequestType;
    std::string strtmp;

    //--------------
    int Result = InitApduBuff();
    if(Result != 0)
    {
        return Result;
    }
    //
    unsigned char *lptmp = mApduToHdlcBuff;
    *lptmp++ = GET_REQUEST;
    
    Result = ParseDataReqHead(InvokeId, Priority, ServiceClass, RequestType, InRoot);
    if(Result != 0)
    {
        return Result;
    }      

    if(ServiceClass == UNCONFIRMED)
    {
        return 322;
    }

    if(RequestType != GET_REQ_NORMAL && RequestType != GET_REQ_WITH_LIST)
    {
        return 329;
    }
    
    if(RequestType == GET_REQ_WITH_LIST && mAarqParams.XdlmsInitiateReq.Conformance.bits.MultipleReferences == 0)
    {
        return 344;
    }

    InvokeIdAndPriority.Val = 0;
    InvokeIdAndPriority.bits.InvokeId = InvokeId;
    InvokeIdAndPriority.bits.Priority = Priority;
    InvokeIdAndPriority.bits.ServiceClass = ServiceClass;
    
    //
    mLastDataRequest.ServiceType = SET_REQUEST;
    mLastDataRequest.InvokeIdAndPriority.Val = InvokeIdAndPriority.Val;
    mLastDataRequest.RequestType = RequestType;     
    //
    
    *lptmp++ = RequestType;  
    *lptmp++ = InvokeIdAndPriority.Val;
    mApduToHdlcLen = lptmp - mApduToHdlcBuff;  


    //
	bool bRet = false;
    switch(RequestType)
    {
        case GET_REQ_NORMAL:
        {  
            bRet = InRoot.FindChildElem("COSEM_Attribute_Descriptor");
            if(bRet == false)//
            {
                return 331;
            }
            else
            {
                int AttributeId;
                Result = ParseCosemAttributeDescriptor(AttributeId, mApduBuff, mApduLen, InRoot);
                if(Result != 0)
                {
                    return Result;
                }

                if(AttributeId == 0 && mAarqParams.XdlmsInitiateReq.Conformance.bits.Attribute0SupportedWithGet == 0)
                {
                    return 342;
                }

                mLastDataRequest.DescriptorsCount = 1;
            }
        }break;
        case GET_REQ_WITH_LIST:
        {
            unsigned int counttmp;
            Result = ParseCosemAttributeDescriptors(mApduBuff, mApduLen, counttmp,
                                        mAarqParams.XdlmsInitiateReq.Conformance.bits.Attribute0SupportedWithGet, InRoot);
            if(Result != 0)
            {
                return Result;
            }

            mLastDataRequest.DescriptorsCount = counttmp;
        }break;
        default:break;
    }

    //计算加密增量
    unsigned char EncryptionIncrement = 0;
    switch(mSecurityOption.SecurityPolicy)
    {
        case ALL_MESSAGES_AUTHENTICATED:EncryptionIncrement = 21; break;
        case ALL_MESSAGES_ENCRYPTED:EncryptionIncrement = 9;break;
        case ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED:EncryptionIncrement = 21;break;
        default:EncryptionIncrement = 0;break;
    }

    if(mApduLen + mApduToHdlcLen + EncryptionIncrement > mClientMaxSendPduSize)
    {
        return 349;
    }


    memcpy(mApduToHdlcBuff + mApduToHdlcLen, mApduBuff, mApduLen);
    mApduToHdlcLen += mApduLen;
    DeleteApduBuff();

    //新增，加密处理
    DoEncryption(GET_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);

    if(mSupportLayerType == HDLC)
    {
        return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
    }
    else if(mSupportLayerType == UDPIP)
    {
        return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
    }
    else
    {
        return 1;
    }
}

int CStack62056::ProcessSetReq(CMarkup& OutRoot, CMarkup& InRoot)
{
    //1.检查状态
        if(mAareParams.MechanismName > LOW_LEVEL)
    {
        if(mCosemAppState != HLS_ASSOCIATED && mAarqParams.XdlmsInitiateReq.ResponseAllowed == true)
        {
            return 319;
        }
    }
    else
    {
        if(mCosemAppState != ASSOCIATED)
        {
            return 319;
        }
    }
      
    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == SN_NOCIPER || mAarqParams.ApplicationContextName == SN_CIPER)
    {
        return 318;
    }
    
    //3.检查一致性模块
    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Set == 0)
    {
        return 318;
    }

    //-------------------------------
    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;  
    int InvokeId;
    bool Priority;
    SERVICE_CLASS ServiceClass;
    int RequestType;
    unsigned int lentmp;

    int Result = InitApduBuff();
    if(Result != 0)
    {
        return Result;
    }
    
    unsigned char *lptmp = mApduToHdlcBuff;
    *lptmp++ = SET_REQUEST;
    unsigned char *lpRequestType = lptmp;
    
    Result = ParseDataReqHead(InvokeId, Priority, ServiceClass, RequestType, InRoot);
    if(Result > 0)
    {
        return Result;
    }

    if(RequestType != SET_REQ_NORMAL && RequestType != SET_REQ_WITH_LIST)
    {
        return 329;
    }

    if(RequestType == SET_REQ_WITH_LIST && mAarqParams.XdlmsInitiateReq.Conformance.bits.MultipleReferences == 0)
    {
        return 344;
    }
    InvokeIdAndPriority.Val = 0;
    InvokeIdAndPriority.bits.InvokeId = InvokeId;
    InvokeIdAndPriority.bits.ServiceClass = ServiceClass;
    InvokeIdAndPriority.bits.Priority = Priority;

    //
    mLastDataRequest.ServiceType = SET_REQUEST;
    mLastDataRequest.InvokeIdAndPriority.Val = InvokeIdAndPriority.Val;
    mLastDataRequest.RequestType = RequestType;
    
    //
    *lptmp++ = RequestType;
    *lptmp++ = InvokeIdAndPriority.Val;
    mApduToHdlcLen = lptmp - mApduToHdlcBuff;

    
    bool bRet = false;
    switch(RequestType)
    {
        case SET_REQ_NORMAL:
        {  
            bRet = InRoot.FindChildElem("COSEM_Attribute_Descriptor");            
            if(bRet == false)//
            {
                return 331;
            }
            else
            {
                int AttributeId;
                Result = ParseCosemAttributeDescriptor(AttributeId, mApduBuff, mApduLen, InRoot);
                if(Result != 0)
                {
                    return Result;
                }
                if(AttributeId == 0 && mAarqParams.XdlmsInitiateReq.Conformance.bits.Attribute0SupportedWithSet == 0)
                {
                    return 343;
                }

                if((unsigned int)(mApduLen + mApduToHdlcLen) >= mClientMaxSendPduSize)
                {
                    return 349;
                }
                mLastDataRequest.DescriptorsCount = 1;

                memcpy(lptmp, mApduBuff, mApduLen);
                mApduToHdlcLen += mApduLen;
                lptmp += mApduLen;
            }

            //编码数据
            bRet = InRoot.FindChildElem("Value");
            if(bRet == false)//
            {
                return 350;
            }
            else
            {
                if(bRet == false)
                {
                    return 353;
                }

                if(ParseData(mApduLen, mApduBuff, InRoot) == false)
                {
                    return 351;
                }


                //计算加密增量
                unsigned char EncryptionIncrement = 0;
                switch(mSecurityOption.SecurityPolicy)
                {  
                    case ALL_MESSAGES_AUTHENTICATED:EncryptionIncrement = 21; break;
                    case ALL_MESSAGES_ENCRYPTED:EncryptionIncrement = 9;break;
                    case ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED:EncryptionIncrement = 21;break;
                    default:EncryptionIncrement = 0;break;
                }

                if(mApduLen + mApduToHdlcLen + EncryptionIncrement <= mClientMaxSendPduSize)
                {
                    memcpy(mApduToHdlcBuff + mApduToHdlcLen, mApduBuff, mApduLen);
                    mApduToHdlcLen += mApduLen;

                    DeleteApduBuff();

                    //新增
                    DoEncryption(SET_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);

                    //通知HDLC层发送
                    if(mSupportLayerType == HDLC)                  
                    {
                        if(ServiceClass == CONFIRMED)
                        {
                            return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                        }
                        else
                        {
                            return DlDataReq(OutRoot, UI, mApduToHdlcBuff, mApduToHdlcLen);
                        }
                    }
                    else if(mSupportLayerType == UDPIP)
                    {
                        return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else
                    {
                        return 1;
                    }
                }
                else//需要分块发送
                {
                    //判断此连接是否支持set块传输
                    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.BlockTransferWithSet == 0)
                    {
                        return 352;
                    }

                    if(ServiceClass == UNCONFIRMED)
                    {
                        return 382;
                    }

                    //启动一个set块传输流程
                    if(mBlockTransfer.Flag == true)
                    {
                    }

                    mBlockTransfer.Flag = true;
                    mBlockTransfer.BlockNumber = 1;
                    mBlockTransfer.Offset = 0;

                    *lpRequestType = SET_REQ_WITH_FIRST_DATABLOCK;
                    //-----------------------------------------
                    //将mApduBuff中的内容，作为第一块数据进行编码，放入mOutDataBuff中
                    *lptmp++ = 0x00;//last-block
                    lptmp += EnCodeSizedUnsigned(mBlockTransfer.BlockNumber, 4, lptmp);

                    //计算长度域 的编码长度
                    mBlockTransfer.RawDataLen = CalcRawDataLen(mClientMaxSendPduSize - EncryptionIncrement, lptmp - mApduToHdlcBuff);
                    mBlockTransfer.RawDataLenEncodeLen = EnCodeLength(mBlockTransfer.RawDataLen, lptmp);
                    lptmp += mBlockTransfer.RawDataLenEncodeLen;
                    memcpy(lptmp, mApduBuff, mBlockTransfer.RawDataLen);
                    lptmp += mBlockTransfer.RawDataLen;
                    mBlockTransfer.Offset = mBlockTransfer.RawDataLen;
                    mApduToHdlcLen = mClientMaxSendPduSize - EncryptionIncrement;

                    //新增，如果application context name为加密，则结合security option对Initiate.req进行加密
                    DoEncryption(SET_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);
                    
                    //通知HDLC层发送
                    if(mSupportLayerType == HDLC)
                    {
                        return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else if(mSupportLayerType == UDPIP)
                    {
                        return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
        }
        case SET_REQ_WITH_LIST:
        {
            unsigned int counttmp1;
            Result = ParseCosemAttributeDescriptors(mApduBuff, mApduLen, counttmp1,
                        mAarqParams.XdlmsInitiateReq.Conformance.bits.Attribute0SupportedWithSet, InRoot);
            if(Result != 0)
            {
                return Result;
            }
            if(mApduLen + mApduToHdlcLen >= mClientMaxSendPduSize)
            {
                return 349;
            }
            mLastDataRequest.DescriptorsCount = counttmp1;

            memcpy(lptmp, mApduBuff, mApduLen);                   
            mApduToHdlcLen += mApduLen;
            lptmp += mApduLen;

            //编码数据
            bRet = InRoot.FindChildElem("Values");
            if(bRet == NULL)//
            {
                return 354;
            }
            else
            {
				unsigned int counttmp2 =0;
				InRoot.IntoElem();
				while (InRoot.FindElem())
				{
					counttmp2++;
				}
				InRoot.ResetMainPos();

                if(counttmp1 != counttmp2)
                {
                    return 358;
                }

                unsigned char *lpApdu = mApduBuff;
                lpApdu += EnCodeLength(counttmp2, lpApdu);
                for(unsigned int i = 0; i < counttmp2; i ++)
                {
                    InRoot.FindElem();
                    if(ParseData(lentmp, lpApdu, InRoot) == false)
                    {
                        return 351;
                    }
                    lpApdu += lentmp;
                }
                mApduLen = lpApdu - mApduBuff;

                //计算加密增量
                unsigned char EncryptionIncrement = 0;
                switch(mSecurityOption.SecurityPolicy)
                {  
                    case ALL_MESSAGES_AUTHENTICATED:EncryptionIncrement = 21; break;
                    case ALL_MESSAGES_ENCRYPTED:EncryptionIncrement = 9;break;
                    case ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED:EncryptionIncrement = 21;break;
                    default:EncryptionIncrement = 0;break;
                }
                
                if(mApduLen + mApduToHdlcLen + EncryptionIncrement <= mClientMaxSendPduSize)
                {
                    memcpy(lptmp, mApduBuff, mApduLen);
                    mApduToHdlcLen += mApduLen;
                    DeleteApduBuff();

                    //新增
                    DoEncryption(SET_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);

                    //通知HDLC层发送
                    if(mSupportLayerType == HDLC)
                    {
                        if(ServiceClass == CONFIRMED)
                        {
                            return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                        }
                        else
                        {
                            return DlDataReq(OutRoot, UI, mApduToHdlcBuff, mApduToHdlcLen);
                        }
                    }
                    else if(mSupportLayerType == UDPIP)
                    {
                        return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else
                    {
                        return 1;
                    }
                }
                else//需要分块发送
                {
                    //判断此连接是否支持set块传输
                    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.BlockTransferWithSet == 0)
                    {
                        return 352;
                    }

                    if(ServiceClass == UNCONFIRMED)
                    {
                        return 382;
                    }

                    //启动一个set块传输流程
                    if(mBlockTransfer.Flag == true)
                    {
                    }

                    mBlockTransfer.Flag = true;
                    mBlockTransfer.BlockNumber = 1;
                    mBlockTransfer.Offset = 0;

                    *lpRequestType = SET_REQ_WITH_LIST_AND_FIRST_DATABLOCK;
                    //-----------------------------------------
                    //将mApduBuff中的内容，作为第一块数据进行编码，放入mOutDataBuff中
                    *lptmp++ = 0x00;//last-block
                    lptmp += EnCodeSizedUnsigned(mBlockTransfer.BlockNumber, 4, lptmp);
                    
                    //计算长度域 的编码长度
                    mBlockTransfer.RawDataLen = CalcRawDataLen(mClientMaxSendPduSize - EncryptionIncrement, lptmp - mApduToHdlcBuff);
                    mBlockTransfer.RawDataLenEncodeLen = EnCodeLength(mBlockTransfer.RawDataLen, lptmp);
                    lptmp += mBlockTransfer.RawDataLenEncodeLen;
                    memcpy(lptmp, mApduBuff, mBlockTransfer.RawDataLen);
                    lptmp += mBlockTransfer.RawDataLen;
                    mBlockTransfer.Offset = mBlockTransfer.RawDataLen;
                    mApduToHdlcLen = mClientMaxSendPduSize - EncryptionIncrement;

                    //新增，
                    DoEncryption(SET_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);
                    //通知HDLC层发送
                    if(mSupportLayerType == HDLC)
                    {
                        return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else if(mSupportLayerType == UDPIP)
                    {
                        return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
        }
        case SET_REQ_WITH_FIRST_DATABLOCK:
        case SET_REQ_WITH_DATABLOCK:
        case SET_REQ_WITH_LIST_AND_FIRST_DATABLOCK:  //以上三种类型不实现
        break;
        default:break;
    }
    return 0;
}

int CStack62056::ProcessActionReq(CMarkup& OutRoot, CMarkup& InRoot)
{
    //1.检查状态
     if(mAareParams.MechanismName > LOW_LEVEL)
    {
        if(mCosemAppState != HLS_ASSOCIATED)
        {
            return 319;
        }
    }
    else
    {
        if(mCosemAppState != ASSOCIATED)
        {
            return 319;
        }
    }


    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == SN_NOCIPER || mAarqParams.ApplicationContextName == SN_CIPER)
    {
        return 330;
    }

    
    //3.检查一致性模块
    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Action == 0)
    {
        return 318;
    }

    //------------------------------------
    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;
    int InvokeId;
    bool Priority;
    SERVICE_CLASS ServiceClass;
    int RequestType;
    unsigned int lentmp;
    //

    int Result = InitApduBuff();
    if(Result != 0)
    {
        return Result;
    }

    memset(mApduBuff,0,200);
    memset(mApduToHdlcBuff,0,mApduToHdlcLen);
    unsigned char *lptmp1 = mApduToHdlcBuff;
    *lptmp1++ = ACTION_REQUEST;
    unsigned char *lpRequestType = lptmp1;
    
    Result = ParseDataReqHead(InvokeId, Priority, ServiceClass, RequestType, InRoot);
    if(Result > 0)
    {
        return Result;
    }
    if(RequestType != ACTION_REQ_NORMAL && RequestType != ACTION_REQ_WITH_LIST)
    {
        return 329;
    }

    if(RequestType == ACTION_REQ_WITH_LIST && mAarqParams.XdlmsInitiateReq.Conformance.bits.MultipleReferences == 0)
    {
        return 344;
    }
    InvokeIdAndPriority.Val = 0;
    InvokeIdAndPriority.bits.InvokeId = InvokeId;
    InvokeIdAndPriority.bits.Priority = Priority;
    InvokeIdAndPriority.bits.ServiceClass = ServiceClass;
    //
    mLastDataRequest.ServiceType = ACTION_REQUEST;
    mLastDataRequest.InvokeIdAndPriority.Val = InvokeIdAndPriority.Val;
    mLastDataRequest.RequestType = RequestType;
    
    //
    *lptmp1++ = RequestType;
    *lptmp1++ = InvokeIdAndPriority.Val;
    mApduToHdlcLen = lptmp1 - mApduToHdlcBuff;
    //
   
	bool bRet = false;
    switch(RequestType)
    {
        case ACTION_REQ_NORMAL:
        {  
            bRet = InRoot.FindChildElem("COSEM_Method_Descriptor");
            if(bRet == false)//
            {
                return 331;
            }
            else
            {
                Result = ParseCosemMethodDescriptor(mApduBuff, mApduLen, InRoot);
                if(Result != 0)
                {
                    return Result;
                }
                if(mApduLen + mApduToHdlcLen >= mClientMaxSendPduSize)
                {
                    return 349;
                }
                mLastDataRequest.DescriptorsCount = 1;
            }
            memcpy(lptmp1, mApduBuff, mApduLen);
            mApduToHdlcLen += mApduLen;
            lptmp1 += mApduLen;

            
            //编码数据 (重新使用mApduBuff)
            bRet = InRoot.FindChildElem("Value");
            if(bRet == NULL)//
            {
                *lptmp1++ = 0x00;
                mApduToHdlcLen = lptmp1 - mApduToHdlcBuff;
                if(mApduToHdlcLen > mClientMaxSendPduSize)
                {
                    return 349;
                }

                DeleteApduBuff();

                //新增，
                DoEncryption(ACTION_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);
                
                //直接发送 action_normal
                if(mSupportLayerType == HDLC)
                {
                    if(ServiceClass == CONFIRMED)
                        return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                    else
                        return DlDataReq(OutRoot, UI, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else if(mSupportLayerType == UDPIP)
                {
                    return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else
                {
                    return 1;
                } 
            }
            else
            {
				bRet = InRoot.IntoElem();
                if(bRet == NULL)
                {
                    return 353;
                }

                unsigned char *lpApdu = mApduBuff;
                *lpApdu++ = 0x01;
                if(ParseData(mApduLen, lpApdu, InRoot) == false)
                {
                    return 351;
                }
                mApduLen += 1;

                //计算加密增量
                unsigned char EncryptionIncrement = 0;
                switch(mSecurityOption.SecurityPolicy)
                {
                    case ALL_MESSAGES_AUTHENTICATED:EncryptionIncrement = 21; break;
                    case ALL_MESSAGES_ENCRYPTED:EncryptionIncrement = 9;break;
                    case ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED:EncryptionIncrement = 21;break;
                    default:EncryptionIncrement = 0;break;
                }
                
                if(mApduLen + mApduToHdlcLen + EncryptionIncrement <= mClientMaxSendPduSize)
                {
                    memcpy(lptmp1, mApduBuff, mApduLen);
                    mApduToHdlcLen += mApduLen;
                    
                    DeleteApduBuff();

                    //新增，
                    DoEncryption(ACTION_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);
                    
                    //通知HDLC层发送
                    if(mSupportLayerType == HDLC)
                    {
                        if(ServiceClass == CONFIRMED)
                        {
                            return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                        }
                        else
                        {
                            return DlDataReq(OutRoot, UI, mApduToHdlcBuff, mApduToHdlcLen);
                        }
                    }
                    else if(mSupportLayerType == UDPIP)
                    {
                        return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else
                    {
                        return 1;
                    }
                }
                else//需要分块发送
                {
                    //判断此连接是否支持ACTION块传输
                    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.BlockTransferWithAction == 0)
                    {
                        return 352;
                    }

                    if(ServiceClass == UNCONFIRMED)
                    {
                        return 382;
                    }

                    //启动一个action块传输流程
                    if(mBlockTransfer.Flag == true)
                    {
                    }

                    mBlockTransfer.Flag = true;
                    mBlockTransfer.BlockNumber = 1;
                    mBlockTransfer.Offset = 0;

                    *lpRequestType = ACTION_REQ_WITH_FIRST_PBLOCK;
                    //-----------------------------------------
                    //将mApduBuff中的内容，作为第一块数据进行编码，放入mOutDataBuff中
                    *lptmp1++ = 0x00;//last-block
                    lptmp1 += EnCodeSizedUnsigned(mBlockTransfer.BlockNumber, 4, lptmp1);
                    //计算长度域 的编码长度 
                    mBlockTransfer.RawDataLen = CalcRawDataLen(mClientMaxSendPduSize - EncryptionIncrement, lptmp1 - mApduToHdlcBuff);
                    mBlockTransfer.RawDataLenEncodeLen = EnCodeLength(mBlockTransfer.RawDataLen, lptmp1);
                    lptmp1 += mBlockTransfer.RawDataLenEncodeLen;
                    memcpy(lptmp1, mApduBuff, mBlockTransfer.RawDataLen);
                    lptmp1+= mBlockTransfer.RawDataLen;
                    mBlockTransfer.Offset = mBlockTransfer.RawDataLen;
                    mApduToHdlcLen = mClientMaxSendPduSize - EncryptionIncrement;


                    //新增，
                    DoEncryption(ACTION_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);
                    
                    //通知HDLC层发送
                    if(mSupportLayerType == HDLC)
                    {
                        return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else if(mSupportLayerType == UDPIP)
                    {
                        return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
        }
        case ACTION_REQ_WITH_LIST:
        {
            unsigned int counttmp1;
            Result = ParseCosemMethodDescriptors(mApduBuff, mApduLen, counttmp1, InRoot);
            if(Result != 0)
            {
                return Result;
            }
            if(mApduLen + mApduToHdlcLen > mClientMaxSendPduSize)
            {
                return 349;
            }
            mLastDataRequest.DescriptorsCount = counttmp1;
            
            memcpy(lptmp1, mApduBuff, mApduLen);
            lptmp1 += mApduLen;
            mApduToHdlcLen += mApduLen;

            unsigned char *lpApdu = mApduBuff;
            //编码数据 (重新使用mApduBuff)
            bRet = InRoot.FindChildElem("Values");
            if(bRet == false)//
            {
                //直接加上counttmp1个null-data
                lpApdu += EnCodeLength(counttmp1, lpApdu);
                for(unsigned int i = 0; i < counttmp1; i ++)
                {
                    *lpApdu++ = 0x00;
                }
                mApduLen = lpApdu - mApduBuff;
            }
            else
            {
				unsigned int counttmp2 = 0;
				InRoot.IntoElem();
				while (InRoot.FindElem())
				{
					counttmp2++;
				}
				InRoot.ResetMainPos();

                if(counttmp1 != counttmp2)
                {
                    return 358;
                }

                lpApdu += EnCodeLength(counttmp2, lpApdu);
                for(unsigned int i = 0; i < counttmp2; i ++)
                {
					InRoot.FindElem();
                    if(ParseData(lentmp, lpApdu, InRoot) == false)
                    {
                        return 351;
                    }
                    lpApdu += lentmp;
                }
                mApduLen = lpApdu - mApduBuff;
            }

            //计算加密增量
            unsigned char EncryptionIncrement = 0;
            switch(mSecurityOption.SecurityPolicy)
            {  
                case ALL_MESSAGES_AUTHENTICATED:EncryptionIncrement = 21; break;
                case ALL_MESSAGES_ENCRYPTED:EncryptionIncrement = 9;break;
                case ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED:EncryptionIncrement = 21;break;
                default:EncryptionIncrement = 0;break;
            }
                
            if(mApduLen + mApduToHdlcLen + EncryptionIncrement <= mClientMaxSendPduSize)
            {
                memcpy(lptmp1, mApduBuff, mApduLen);
                mApduToHdlcLen += mApduLen;
                DeleteApduBuff();

                //新增，
                DoEncryption(ACTION_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);


                //通知HDLC层发送
                if(mSupportLayerType == HDLC)
                {
                    if(ServiceClass == CONFIRMED)
                    {
                        return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                    else
                    {
                        return DlDataReq(OutRoot, UI, mApduToHdlcBuff, mApduToHdlcLen);
                    }
                }
                else if(mSupportLayerType == UDPIP)
                {
                    return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else
                {
                    return 1;
                }
            }
            else//需要分块发送
            {
                //判断此连接是否支持action块传输
                if(mAarqParams.XdlmsInitiateReq.Conformance.bits.BlockTransferWithAction == 0)
                {
                    return 352;
                }

                if(ServiceClass == UNCONFIRMED)
                {
                    return 382;
                }

                //启动一个set块传输流程
                if(mBlockTransfer.Flag == true)
                {
                }

                mBlockTransfer.Flag = true;
                mBlockTransfer.BlockNumber = 1;
                mBlockTransfer.Offset = 0;

                *lpRequestType = ACTION_REQ_WITH_LIST_AND_FIRST_PBLOCK;
                //-----------------------------------------
                //将mApduBuff中的内容，作为第一块数据进行编码，放入mOutDataBuff中
                *lptmp1++ = 0x00;//last-block
                lptmp1 += EnCodeSizedUnsigned(mBlockTransfer.BlockNumber, 4, lptmp1);
                mApduToHdlcLen = lptmp1 - mApduToHdlcBuff;
                //计算长度域 的编码长度
                mBlockTransfer.RawDataLen = CalcRawDataLen(mClientMaxSendPduSize - EncryptionIncrement, mApduToHdlcLen);
                mBlockTransfer.RawDataLenEncodeLen = EnCodeLength(mBlockTransfer.RawDataLen, lptmp1);
                lptmp1 += mBlockTransfer.RawDataLenEncodeLen;
                memcpy(lptmp1, mApduBuff, mBlockTransfer.RawDataLen);
                lptmp1 += mBlockTransfer.RawDataLen;
                mBlockTransfer.Offset = mBlockTransfer.RawDataLen;
                mApduToHdlcLen = mClientMaxSendPduSize - EncryptionIncrement;

                //新增，
                DoEncryption(ACTION_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);


                //通知HDLC层发送
                if(mSupportLayerType == HDLC)
                {
                    return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else if(mSupportLayerType == UDPIP)
                {
                    return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else
                {
                    return 1;
                }
            }
        }
        case ACTION_REQ_NEXT_PBLOCK:
        case ACTION_REQ_WITH_FIRST_PBLOCK:
        case ACTION_REQ_WITH_LIST_AND_FIRST_PBLOCK:
        case ACTION_REQ_WITH_PBLOCK:  //以上四种类型不实现
        break;
        default:break;
    }

    return 0;
}

int CStack62056::ProcessTriggerEventNotificationSendingReq(CMarkup& OutRoot, CMarkup& InRoot)
{
    //直接构造一个 PorF = 1 的UI帧发送
    return DlDataReq(OutRoot, UI, NULL, 0);
}


int CStack62056::ProcessReadReq(CMarkup& OutRoot, CMarkup& InRoot)
{
    //1.检查状态
       if(mAareParams.MechanismName > LOW_LEVEL)
    {
        if(mCosemAppState != HLS_ASSOCIATED)
        {
            return 319;
        }
    }
    else
    {
        if(mCosemAppState != ASSOCIATED)
        {
            return 319;
        }
    }


    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == LN_NOCIPER || mAarqParams.ApplicationContextName == LN_CIPER)
    {
        return 330;
    }

    
    //3.检查一致性模块
    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Read == 0)
    {
        return 318;
    }

    //------------------------------------
    unsigned int lentmp;
    int Result;
    //
    
    unsigned char *lptmp = mApduToHdlcBuff;
    *lptmp++ = READ_REQUEST;
    bool bRet = InRoot.FindChildElem("Variable_Access_Specifications");
    if(bRet == false)
    {
        return 400;
    }

	unsigned int Count = 0;
	InRoot.IntoElem();
	while (InRoot.FindElem())
	{
		Count++;
	}
	InRoot.ResetMainPos();
 
    if(Count < 1)
    {
        return 401;
    }   
    lptmp += EnCodeLength(Count, lptmp);

	InRoot.IntoElem();
    for(unsigned int i = 0; i < Count; i ++)
    {
		InRoot.FindElem();
        if(InRoot.GetTagName() != "Variable_Access_Specification")
        {
            return 401;
        }
        Result = ParseVariableAccessSpecification(lptmp, lentmp, InRoot);
        if(Result != 0)
        {
            return Result;
        }

        lptmp += lentmp;   
    }
    mApduToHdlcLen = lptmp - mApduToHdlcBuff;
    return 0;
}



int CStack62056::ProcessWriteReq(CMarkup& OutRoot, CMarkup& InRoot, bool Confirmed)
{
    //1.检查状态
       if(mAareParams.MechanismName > LOW_LEVEL)
    {
        if(mCosemAppState != HLS_ASSOCIATED)
        {
            return 319;
        }
    }
    else
    {
        if(mCosemAppState != ASSOCIATED)
        {
            return 319;
        }
    }


    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == LN_NOCIPER || mAarqParams.ApplicationContextName == LN_CIPER)
    {
        return 330;
    }

    if(Confirmed == true)
    {
        //3.检查一致性模块
        if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Write == 0)
        {
            return 318;
        }
    }
    else
    {
        if(mAarqParams.XdlmsInitiateReq.Conformance.bits.UnconfirmedWrite == 0)
        {
            return 318;
        }
    }

    //------------------------------------
    unsigned int lentmp;
    int Result;
    //

    unsigned char *lptmp = mApduToHdlcBuff;
    if(Confirmed == true)
    {
        *lptmp++ = WRITE_REQUEST;
    }
    else
    {
        *lptmp++ = UNCONFIRMED_WRITE_REQUEST;
    }
    bool bRet = InRoot.FindChildElem("Variable_Access_Specifications");
    if(bRet == false)
    {
        return 400;
    }

	unsigned int Count = 0;
	InRoot.IntoElem();
	while (InRoot.FindElem())
	{
		Count++;
	}
	InRoot.ResetMainPos();

    if(Count < 1)
    {
        return 401;
    }
    lptmp += EnCodeLength(Count, lptmp);

	InRoot.IntoElem();
    for(unsigned int i = 0; i < Count; i ++)
    {
        InRoot.FindElem();
        if(InRoot.GetTagName() != "Variable_Access_Specification")
        {
            return 401;
        }
        Result = ParseVariableAccessSpecification(lptmp, lentmp, InRoot);
        if(Result != 0)
        {
            return Result;
        }

        lptmp += lentmp;
    }
	InRoot.OutOfElem();
	InRoot.OutOfElem();

    bRet = InRoot.FindChildElem("Values");
    if(bRet == false)
    {
        return 411;
    }

	unsigned int Count2 = 0;
	InRoot.IntoElem();
	while (InRoot.FindElem())
	{
		Count2++;
	}
	InRoot.ResetMainPos();

    if(Count2 < 1)
    {
        return 412;
    }

    if(Count != Count2)
    {
        return 413;
    }
    lptmp += EnCodeLength(Count2, lptmp);

    for(unsigned int i = 0; i < Count2; i ++)
    {
		InRoot.FindElem();
        if(InRoot.GetTagName() != "Value")
        {
            return 414;
        }
        if(ParseData(lentmp, lptmp, InRoot) == false)
        {
            return 415;
        }

        lptmp += lentmp;
    }        
    mApduToHdlcLen = lptmp - mApduToHdlcBuff;
    return 0;
}


int CStack62056::ParseVariableAccessSpecification(unsigned char *Dest, unsigned int &Len, CMarkup& Parent)
{
    unsigned char *lptmp = Dest;
    unsigned int lentmp;
    int valuetmp;
    std::string strtmp;

	Parent.IntoElem();
	bool bRet = Parent.FindElem();
    if(bRet == false)
    {
        return 402;
    }
    
	std::string strTagName = Parent.GetTagName();
    if(strTagName == "variable-name")
    {
        *lptmp++ = 0x02;
        strtmp = Parent.GetAttrib("Value");
        try
        {
            valuetmp = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 404;
        }
        if(valuetmp > 0xFFFF || valuetmp < 0)
        {
            return 404;
        }
        lptmp += EnCodeSizedUnsigned((unsigned long)valuetmp, 2, lptmp);
    }
    else if(strTagName == "detailed-access")
    {
        *lptmp++ = 0x03;
        bRet = Parent.FindChildElem("variable-name");
        if(bRet == false)
        {
            return 405;
        }
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        try
        {
            valuetmp = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 404;
        }
        if(valuetmp > 0xFFFF || valuetmp < 0)
        {
            return 404;
        }
        lptmp += EnCodeSizedUnsigned((unsigned long)valuetmp, 2, lptmp);

        bRet = Parent.FindChildElem("detailed-access");
        if(bRet == false)
        {
            return 405;
        }
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        if(IsHex(strtmp) == false)
        {
            return 406;
        }
        lptmp += EnCodeOctetStr(strtmp, lptmp);  
    }
    else if(strTagName == "parameterized-access")
    {
        *lptmp++ = 0x04;
        bRet = Parent.FindChildElem("variable-name");
        if(bRet == false)
        {
            return 407;
        }
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        try
        {
            valuetmp = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 404;
        }
        if(valuetmp > 0xFFFF || valuetmp < 0)
        {
            return 404;
        }
        lptmp += EnCodeSizedUnsigned((unsigned long)valuetmp, 2, lptmp);

		bRet = Parent.FindChildElem("selector");
        if(bRet == false)
        {
            return 407;
        }
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        try
        {
            valuetmp = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 408;
        }
        if(valuetmp > 0xFF || valuetmp < 0)
        {
            return 408;
        }
        lptmp += EnCodeSizedUnsigned(valuetmp, 1, lptmp);


		bRet = Parent.FindChildElem("parameter");
        if(bRet == false)
        {
            return 407;
        }
		bRet = Parent.IntoElem();
		bRet = Parent.IntoElem();
        if(bRet == false)//
        {
            return 409;
        }

        if(ParseData(lentmp, lptmp, Parent) == false)
        {
            return 410;
        }
        lptmp += lentmp;
    }
    else
    {
        return 403;
    }

    Len = lptmp - Dest;
    return 0;
}

int CStack62056::ParseDataReqHead(int &InvokeId, bool &Priority, SERVICE_CLASS &ServiceClass, int &RequestType, CMarkup& Root)
{
    std::string strtmp;
    
    bool bRet = Root.FindChildElem("Invoke_Id");
    if(bRet == false)//
    {
        return 320;
    }
    else
    {
		Root.IntoElem();
        strtmp = Root.GetAttrib("Value");
		Root.OutOfElem();
        try
        {
            InvokeId = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 321;
        }   

        if(InvokeId > 15 || InvokeId < 0)
        {
            return 321;
        }
    }

    bRet = Root.FindChildElem("Priority");
    if(bRet == false)//
    {
        return 323;
    }
    else
    {
		Root.IntoElem();
        strtmp = Root.GetAttrib("Value");
		Root.OutOfElem();
        if(strtmp == "0")
        {
            Priority = false;
        }
        else //if(strtmp == "01" || strtmp == "FF")
        {
            Priority = true;
        }
        //else
        //{
        //    return 324;
        //}
    }

	bRet = Root.FindChildElem("Service_Class");
    if(bRet == false)//
    {
        return 325;
    }
    else
    {
		Root.IntoElem();
		strtmp = Root.GetAttrib("Value");
		Root.OutOfElem();
        if(strtmp == "0")
        {
            ServiceClass = UNCONFIRMED;
        }
        else //if(strtmp == "1")
        {
            ServiceClass = CONFIRMED;
        }
        //else
        //{
        //    return 326;
        //}
    }

    //对比当前连接的Service_Class
    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == false && ServiceClass == CONFIRMED)
    {
        return 348;
    }

	bRet = Root.FindChildElem("Request_Type");
    if(bRet == false)//
    {
        return 327;
    }
    else
    {
		Root.IntoElem();
        strtmp = Root.GetAttrib("Value");
		Root.OutOfElem();
        try
        {
            RequestType = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 328;
        }
    }
    return 0;
}


int CStack62056::ParseCosemAttributeDescriptor(int &AttributeId, unsigned char *Dest, unsigned int &Len, CMarkup& Parent)
{
    std::string strtmp;
    unsigned char *lptmp = Dest;
    bool bRet = Parent.FindChildElem("COSEM_Class_Id");
    if(bRet == false)//
    {
        return 332;
    }
    else
    {
        int ClassId;
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        try
        {
            ClassId = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 333;
        }

        if(ClassId > 65535 || ClassId < 0)
        {
            return 334;
        }
           

        lptmp += EnCodeSizedUnsigned(ClassId, 2, lptmp);
    }
    bRet = Parent.FindChildElem("COSEM_Instance_Id");
    if(bRet == false)//
    {
        return 335;
    }
    else
    {
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        if(IsHex(strtmp) && strtmp.length() == 12)
        {
            lptmp += EnCodeSizedOctetStr(strtmp, 6, lptmp);  
        }
        else
        {
            return 336;
        }

    }
    bRet = Parent.FindChildElem("COSEM_Attribute_Id");
    if(bRet == false)//
    {
        return 337;
    }
    else
    {
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        try
        {
            AttributeId = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 338;
        }

        if(AttributeId > 128 || AttributeId < -127)
        {
            return 339;
        } 

        lptmp += EnCodeSizedInteger(AttributeId, 1, lptmp);
    }
    bRet = Parent.FindChildElem("Selective_Access_Parameters");
    if(bRet == false)//
    {
        *lptmp++ = 0x00;
        Len = 10;
    }
    else
    {
        if(mAarqParams.XdlmsInitiateReq.Conformance.bits.SelectiveAccess == 0)
        {
            return 340;//
        }

        *lptmp++ = 0x01;
        unsigned int lentmp = 0;
        if(ParseSelectiveAccessParameters(lptmp, lentmp, Parent) == false)
        {
            return 341;
        }

        Len = 10 + lentmp;
    } 
    return 0;
}

bool CStack62056::ParseSelectiveAccessParameters(unsigned char *Dest, unsigned int &Len, CMarkup& Parent)
{
    std::string strtmp;
    unsigned char *lptmp = Dest;
    bool bRet = Parent.FindChildElem("access_selector");
    if(bRet == false)//
    {
        return false;
    }
    else
    {
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        if(strtmp != "1" && strtmp != "2")
        {
            return false;
        }
        else
        {
            *lptmp++ = atoi(strtmp.c_str());
        }
    }

    bRet = Parent.FindChildElem("access_parameters");
    if(bRet == false)//
    {
        return false;
    }
    else
    {
		bRet = Parent.IntoElem();
        if(bRet == false)//
        {
            return false;
        }
        
        unsigned int lentmp;

        if(ParseData(lentmp, lptmp, Parent) == false)
        {
            return false;
        }
        else
        {
            Len = 1 + lentmp;
            return true;
        }
    }
}

int CStack62056::ParseCosemMethodDescriptor(unsigned char *Dest, unsigned int &Len, CMarkup& Parent)
{
    std::string strtmp;
    unsigned char *lptmp = Dest;
    
    bool bRet = Parent.FindChildElem("COSEM_Class_Id");
    if(bRet == false)//
    {
        return 332;
    }
    else
    {
        int ClassId;
		Parent.IntoElem();
        strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        try
        {
            ClassId = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 333;
        }

        if(ClassId > 65535 || ClassId < 0)
        {
            return 334;
        }
	 if(ClassId==64)
	 	flag=1;
           
        lptmp += EnCodeSizedUnsigned(ClassId, 2, lptmp);
    }
    bRet = Parent.FindChildElem("COSEM_Instance_Id");
    if(bRet == false)//
    {
        return 335;
    }
    else
    {
		Parent.IntoElem();
		strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        if(IsHex(strtmp) && strtmp.length() == 12)
        {
            lptmp += EnCodeSizedOctetStr(strtmp, 6, lptmp);
        }
        else
        {
            return 336;
        }

    }
    bRet = Parent.FindChildElem("COSEM_Method_Id");
    if(bRet == false)//
    {
        return 337;
    }
    else
    {
        int MethodId;
		Parent.IntoElem();
		strtmp = Parent.GetAttrib("Value");
		Parent.OutOfElem();
        try
        {
            MethodId = atoi(strtmp.c_str());
        }
        catch(...)
        {
            return 338;
        }

        if(MethodId > 128 || MethodId < -127)
        {
            return 339;
        } 
        if(MethodId==2 && flag==1)
        {
            if(mAareParams.MechanismName <= LOW_LEVEL)
            {
                flag = 0;
            }
            else
            {
                flag=2;
            }
        }
        lptmp += EnCodeSizedInteger(MethodId, 1, lptmp);
    }

    Len = lptmp - Dest;
    return 0;
}


int CStack62056::ParseCosemAttributeDescriptors(unsigned char *Dest, unsigned int &Len,
            unsigned int &DescriptorsCount, bool Attribute0Supported, CMarkup& Root)
{
    unsigned char *lptmp = Dest;
    bool bRet = Root.FindChildElem("COSEM_Attribute_Descriptors");
    if(bRet == false)//
    {
        return 333;
    }
    else
    {
		Root.IntoElem();
		while (Root.FindElem())
		{
			DescriptorsCount++;
		}
		Root.ResetMainPos();

        if(DescriptorsCount == 0)
        {
            return 334;
        }
                
        lptmp += EnCodeLength(DescriptorsCount, lptmp);

        for(unsigned int i = 0; i < DescriptorsCount; i++)
        {
            bRet = Root.FindElem();
            std::string TestNodeName = Root.GetTagName();
            if(TestNodeName != "COSEM_Attribute_Descriptor")
            {
                return 335;
            }

            int AttributeId;
            unsigned int lentmp;
            int Result = ParseCosemAttributeDescriptor(AttributeId, lptmp, lentmp, Root);
            if(Result != 0)
            {
                return Result;
            }

            if(AttributeId == 0 && Attribute0Supported == false)
            {
                return 342;
            }   
            lptmp += lentmp;
        }

        Len = lptmp - Dest;
        
        return 0;
    }
}

int CStack62056::ParseCosemMethodDescriptors(unsigned char *Dest, unsigned int &Len,
            unsigned int &DescriptorsCount, CMarkup& Root)
{
    unsigned char *lptmp = Dest;
    bool bRet = Root.FindChildElem("COSEM_Method_Descriptors");
    if(bRet == false)//
    {
        return 333;
    }
    else
    {
		Root.IntoElem();
		while (Root.FindElem())
		{
			DescriptorsCount++;
		}
		Root.ResetMainPos();

        if(DescriptorsCount == 0)
        {
            return 334;
        }
        lptmp += EnCodeLength(DescriptorsCount, lptmp);
 
        for(unsigned int i = 0; i < DescriptorsCount; i++)
        {
			bRet = Root.FindElem();
			std::string TestNodeName = Root.GetTagName();
            if(TestNodeName != "COSEM_Method_Descriptor")
            {
                return 335;
            }

            unsigned int lentmp;
            int Result = ParseCosemMethodDescriptor(lptmp, lentmp, Root);
            if(Result != 0)
            {
                return Result;
            }
            lptmp += lentmp;
        }

        Len = lptmp - Dest;
        
        return 0;
    }
}


bool CStack62056::ParseData(unsigned int &Len, unsigned char *lpDest, CMarkup& DataNode)
{
    unsigned char *lptmp = lpDest;
    try
    {
        int Tag = 0;		                                                             
        int Over = 0;//0--继续  1--完毕  -1--出错                                    
        std::string str = "";                                                      
        do                                                                           
        {
			std::string strTypeName = DataNode.GetAttrib("Type");
            Tag = GetTag(strTypeName.c_str());
            if(Tag == -1)                                                              
            {                                                                          
                Over = -1;                                                               
                break;                                                                   
            }                                                                          
            *lptmp++ = Tag;
            if(Tag == 1 || Tag == 2)                                                   
            {                                                                          
                //寻找子节点
				int Count = 0;
				DataNode.IntoElem();
				while (DataNode.FindElem())
				{
					Count++;
				}
				DataNode.ResetMainPos();

                lptmp += EnCodeLength(Count, lptmp);                                     
                if(Count)                                                                
                {                                                                        
                    //mNode[mdepth++] = mCurrentNode;
					DataNode.IntoElem();
					DataNode.FindElem();
                    continue;
                }
            }                                                                          
            else                                                                       
            {                                                                          
                switch(Tag)	                                                             
                {                                                                        
                    case 0://null-data 					
                    case 255://don't-case                                                                   
                                    break;
                    case 3://boolean
                    {
                        str = DataNode.GetAttrib("Value");
                        if(str == "0")  *lptmp++ = 0;
                        else            *lptmp++ = 0xFF;
                    }
                    break;
                    case 4://bit-string  
                    {	                                             
                        str = DataNode.GetAttrib("Value");
                        if((str.length()) % 2 != 0)
                        {
                            Over = -1;
                        }
                        lptmp += EnCodeBit(str, str.length() * 4, lptmp);
                    }
                    break;
                    case 5://double-long
                    case 6://double-long-unsigned
                    {
                        str = DataNode.GetAttrib("Value");
                        lptmp += EnCodeSizedUnsigned(strtoll(str.c_str(), nullptr, 16), 4, lptmp);
                    }
                    break;
                    case 9://octet-string
                    {
                        str = DataNode.GetAttrib("Value");
                        if((str.length())%2 != 0)
                        {
                            Over = -1;
                        }
			
			    lptmp += EnCodeOctetStr(str, lptmp);
			   if(flag==2)
			   {
			   	//if(str.Length !=24)
				//	return 403;
				memcpy(ase_gcm_128->S,lptmp-16,16);
    				ase_gcm_128->aes_wrap_String((char*)mSecurityOption.SecurityMaterial.MK);
				memcpy(lptmp-16,(unsigned char *)&ase_gcm_128->S[57],24);
				*(lptmp-17)=24;
                                lptmp+=8;
				flag=0;
                           }
                    }
                    break;
                    case 10://visible-string
                    {
                        str = DataNode.GetAttrib("Value");
                        lptmp += EnCodeVisibleStr(str, lptmp);
                    }
                    break;
                    case 13://bcd                                                          
                    case 15://integer                                                      
                    case 17://unsigned                                                     
                    case 22://enum
                    {
                        str = DataNode.GetAttrib("Value");
                        lptmp += EnCodeSizedUnsigned(atoi(str.c_str()), 1, lptmp);
                    }
                    break;
                    case 16://long                                                         
                    case 18://long-unsigned
                    {
                        str = DataNode.GetAttrib("Value");
                        try
                        {
                            lptmp += EnCodeSizedUnsigned(atoll(str.c_str()), 2, lptmp);
                        }
                        catch(...)
                        {
                            return false;
                        }
                    }
                    break;
                    case 19://compact-array
                    break;
                    case 20://long64       
                    {
                        //str = GetText(Node);
                        //long long Value = atoi64(str);
                        //str = CurrentNode->GetAttribute("Value");
                        //lptmp += EnCodeFloat64(StrToFloatDef(str,0),lptmp);
                        str = DataNode.GetAttrib("Value");
                        long long Value = strtoll(str.c_str(), nullptr, 16);
                        lptmp += EnCodeSizedInteger(Value, 8, lptmp);
                    }
                    break;
                    case 21://long64-unsigned
                    {
                        //str = GetText(Node);
                        //long long Value = atoi64(str);
                        //str = CurrentNode->GetAttribute("Value");
                        //lptmp += EnCodeFloat64(StrToFloatDef(str,0),lptmp);
						str = DataNode.GetAttrib("Value");
						long long Value = strtoll(str.c_str(), nullptr, 16);
                        lptmp += EnCodeSizedInteger(Value, 8, lptmp);
                    }
                    break;                                                                 
                    case 23://float32 
                    {
                        str = DataNode.GetAttrib("Value");
                        lptmp += EnCodeFloat32((float)atof(str.c_str()), lptmp);
                    }
                    break;
                    case 24://float64 
                    {
                        str = DataNode.GetAttrib("Value");
                        lptmp += EnCodeFloat64(atof(str.c_str()),lptmp);
                    }
                    break;
                    case 25://date_time   
                    {
                        str = DataNode.GetAttrib("Value");
                        lptmp += EnCodeDateTime(str,lptmp);
                    }
                    break;
                    case 26://date  
                    {
                        str = DataNode.GetAttrib("Value");
                        lptmp += EnCodeDate(str,lptmp);
                    }
                    break;
                    case 27://time
                    {
                        str = DataNode.GetAttrib("Value");
                        lptmp += EnCodeTime(str,lptmp);
                    }
                    break;
                }//switch                                                                

            }//else

            /* yanshiqi
			while(Over == 0)
            {
                if(DataNode == CurrentNode) Over = 1;
                else
                {
                    if(CurrentNode->NextSibling() == NULL)
                    {
                        CurrentNode = CurrentNode->ParentNode;
                    }
                    else
                    {
                        CurrentNode = CurrentNode->NextSibling();
                        break;
                    }
                }
            }*/ 
        }while(Over == 0);
        if(Over == -1)
        {
            return false;
        }
        else
        {
            Len = lptmp - lpDest;
            return true;
        }
    }
    catch( ... )
    {
      return false;
    }
    //return 0;
};


int CStack62056::ProcessAARE(CMarkup& OutRoot, const unsigned char *lpAare, unsigned int AareLen)
{
	    unsigned char	EK_Buf[KEY_LEN];

    if(mCosemAppState != ASSOCIATION_PENDING)
    {
        return 703;
    }
    //解析AARE
    unsigned int lentmp;
    unsigned int AareApduLen;
    bool RespondingAPTitlePresent = false;

    const unsigned char *lptmp = lpAare;
    lentmp = DeCodeLength(&AareApduLen, lptmp);
    if(lentmp + AareApduLen > AareLen)
    {
        return 704;
    }
    lptmp += lentmp;

    mAareParams.AssociationResult = REJECTED_TRANSIENT;
    mAareParams.ResponderAcseRequirementsPresent = false;
    mAareParams.ResponderAcseRequirements = true;
    
	OutRoot.IntoElem();
    bool bRet = OutRoot.AddElem("XML");
	OutRoot.IntoElem();
    bRet = OutRoot.AddElem("COSEM_OPEN_CNF");
	OutRoot.IntoElem();
    //WriteAddr(Root);

    while(lptmp < (lpAare + AareLen))
    {
        switch(*lptmp++)
        {
            case 0x80:
            {
                if(!(*lptmp++ == 0x02 && *lptmp++ == 0x07 && *lptmp++ == 0x80))
                {
                    return 705;
                }
                else
                {
					bRet = OutRoot.AddElem("ACSE_Protocol_Version");
                    OutRoot.AddAttrib("Value", "80");
                }
            }
            break;
            case 0xA1:
            {
                if(*lptmp++ == 0x09 && *lptmp++ == 0x06 && *lptmp++ == 0x07
                    && *lptmp++ == 0x60 && *lptmp++ == 0x85 && *lptmp++ == 0x74 && *lptmp++ == 0x05
                    && *lptmp++ == 0x08 && *lptmp++ == 0x01)
                {           
                    if(*lptmp == LN_NOCIPER || *lptmp == SN_NOCIPER || *lptmp == LN_CIPER || *lptmp == SN_CIPER)
                    {
                        mAareParams.ApplicationContextName = (APPLICATION_CONTEXT_NAME)*lptmp++;
						bRet = OutRoot.AddElem("Application_Context_Name");
                        mAareParams.MechanismName = LOW_LEVEL;
						char szBuf[32] = { 0 };
						sprintf_s(szBuf, "%d", mAareParams.ApplicationContextName);
						OutRoot.AddAttrib("Value", szBuf);
                    }
                    else
                    {
                        return 707;
                    }
                }
                else
                {
                    return 706;
                }
            }
            break;
            case 0xA2:
            {           
                if(*lptmp++ == 0x03 && *lptmp++ == 0x02 && *lptmp++ == 0x01)
                {
                    if(*lptmp == ACCEPTED || *lptmp == REJECTED_PERMANENT || *lptmp == REJECTED_TRANSIENT)
                    {
                        mAareParams.AssociationResult = (ASSOCIATION_RESULT)(*lptmp++);
						bRet = OutRoot.AddElem("Result");
						char szBuf[32] = { 0 };
						sprintf_s(szBuf, "%d", mAareParams.AssociationResult);
						OutRoot.AddAttrib("Value", szBuf);                       
                    }
                    else
                    {
                        return 709;
                    }
                }
                else
                {
                    return 708;
                }
            }
            break;
            case 0xA3:
            {
                if(*lptmp++ == 0x05)
                {
                    int Diagnostic = *lptmp++;
                    if(Diagnostic == 0xA1 || Diagnostic == 0xA2)
                    {
                        if(*lptmp++ == 0x03 && *lptmp++ == 0x02 && *lptmp++ == 0x01)
                        {
                            if(Diagnostic == 0xA1)
                            {
                                if(!(*lptmp == 0 || *lptmp == 1 || *lptmp == 2 || *lptmp == 11 || *lptmp == 12
                                || *lptmp == 13 || *lptmp == 14))
                                {
                                    return 711;
                                }
                                Diagnostic = 1;
                            }
                            else
                            {
                                if(!(*lptmp == 0 || *lptmp == 1 || *lptmp == 2))
                                {
                                    return 711;
                                }
                                Diagnostic = 2;
                            }
                            if(mAarqParams.MechanismName > LOW_LEVEL)
                            {
                                if(*lptmp != 14)
                                {
                                    return 711;
                                }
                            }
                            Diagnostic = Diagnostic * 100 + *lptmp++;
							bRet = OutRoot.AddElem("Result_Source_Diagnostic");
							char szBuf[32] = { 0 };
							sprintf_s(szBuf, "%d", Diagnostic);
							OutRoot.AddAttrib("Value", szBuf);
                        }
                        else
                        {
                            return 710;
                        }
                    }
                    else
                    {
                        return 710;
                    }
                }
                else
                {
                    return 710;
                }


                /*if(*lptmp++ == 0x05 && *lptmp++ == 0xA1 && *lptmp++ == 0x03)
                {
                    if(*lptmp == 0x01 || *lptmp == 0x02)
                    {
                        int Diagnostic = *lptmp++;
                        if(*lptmp++ != 0x01)
                        {
                            return 710;
                        }
                        else
                        {
                            if(Diagnostic == 1)
                            {
                                if(!(*lptmp == 0 || *lptmp == 1 || *lptmp == 2 || *lptmp == 11 || *lptmp == 12
                                || *lptmp == 13 || *lptmp == 14))
                                {
                                    return 711;
                                }
                            }
                            else
                            {
                                if(!(*lptmp == 0 || *lptmp == 1 || *lptmp == 2))
                                {
                                    return 711;
                                }
                            }
                            Diagnostic = Diagnostic * 100 + *lptmp++;
                        }
                    
                        CMarkup& Node = Root->AddChild("Result_Source_Diagnostic");
                        Node->SetAttribute("Value", IntToStr(Diagnostic));
                    }
                    else
                    {
                        return 711;
                    }
                }
                else
                {
                    return 710;
                } */
            }
            break;
            case 0xA4:
            {
                unsigned int len1, len2;
                lentmp = DeCodeLength(&len1, lptmp);
                if(len1 != 0x0A)
                {
                    return 720;
                }
                lptmp += lentmp;
                if(*lptmp++ != 0x04)
                {
                    return 720;
                }
                lentmp = DeCodeLength(&len2, lptmp);
                if(len2 != 0x08)
                {
                    return 720;
                }
                lptmp += lentmp;
                memcpy(mAareParams.RespondingAPTitle, lptmp, SYS_T_LEN);
                lptmp += SYS_T_LEN;
                RespondingAPTitlePresent = true;
            }
            break;
            case 0x88:
            {
                mAareParams.ResponderAcseRequirementsPresent = true;
                if(*lptmp++ == 0x02 && *lptmp++ == 0x07)
                {
					bRet = OutRoot.AddElem("Responder_Acse_Requirement");
                    if(*lptmp == 0x80)
                    {
                        //ResponderAcseRequirement = true;
                        mAareParams.ResponderAcseRequirements = true;
						OutRoot.AddAttrib("Value", "80");
                    }
                    else if(*lptmp == 0x00)
                    {
                        mAareParams.ResponderAcseRequirements = false;
						OutRoot.AddAttrib("Value", "00");
                    }
                    else
                    {
                        return 712;
                    }
                    lptmp++;
                }
                else
                {
                    return 712;
                }
            }break;
            case 0x89:
            {
                if(*lptmp++ == 0x07 && *lptmp++ == 0x60 && *lptmp++ == 0x85 && *lptmp++ == 0x74 && *lptmp++ == 0x05
                    && *lptmp++ == 0x08 && *lptmp++ == 0x02)
                {
                    if((*lptmp == LOWEST_LEVEL || *lptmp == LOW_LEVEL || *lptmp == HIGH_LEVEL_CMAC) && *lptmp == mAarqParams.MechanismName) 
                    {
                        mAareParams.MechanismName = (SECURITY_MECHANISM_NAME)(*lptmp);
						bRet = OutRoot.AddElem("Security_Mechanism_Name");
						char szBuf[32] = { 0 };
						sprintf_s(szBuf, "%d", *lptmp++);
						OutRoot.AddAttrib("Value", szBuf);
                    }
                    else
                    {
                        return 714;
                    }
                }
                else
                {
                    return 713;
                }
            }break;
            case 0xAA:
            {
                unsigned int len1, len2;
                lentmp = DeCodeLength(&len1, lptmp);
                lptmp += lentmp;
                if(*lptmp++ == 0x80)
                {
                    lentmp = DeCodeLength(&len2, lptmp);
                    lptmp += lentmp;
                    //CMarkup& Node = Root->AddChild("Responding_Authentication_Value");


                    if(len2 < MAX_PASSWORD_LEN)
                    {
                        memset(mAareParams.RespondingAuthenticationValue, 0, MAX_PASSWORD_LEN);
                        memcpy(mAareParams.RespondingAuthenticationValue, lptmp, len2);
                        mAareParams.RespondingAuthenticationValueLen = len2;
                    }
                    else
                    {
                        memcpy(mAareParams.RespondingAuthenticationValue, lptmp, MAX_PASSWORD_LEN);
                        mAareParams.RespondingAuthenticationValueLen = MAX_PASSWORD_LEN;
                    }
                    //此值不需要写入XML中
                    lptmp += len2;
                }
                else
                {
                    return 715;
                }
            }break;
            case 0xBE:
            {
                unsigned int len1, len2, len3;
                unsigned char XdlmsInitRes[256];
                unsigned short  XdlmsInitResLen = 0;

                lentmp = DeCodeLength(&len1, lptmp);
                lptmp += lentmp;
                if(*lptmp++ != 0x04)
                {
                    return 716;
                }
                lentmp = DeCodeLength(&len2, lptmp);
                lptmp += lentmp;


                if((mAarqParams.ApplicationContextName == LN_CIPER || mAarqParams.ApplicationContextName == SN_CIPER)&&(mSecurityOption.SecurityPolicy != NO_SECURITY)) //cipher
                {
                    if(*lptmp++ != GLO_INITIATE_RESPONSE)
                    {
                        return 717;
                    }
                    lentmp = DeCodeLength(&len3, lptmp);
                    lptmp += lentmp;

                    if(!(*lptmp == SC_U_A || *lptmp == SC_U_E || *lptmp == SC_U_AE || *lptmp == SC_B_A || *lptmp == SC_B_E || *lptmp == SC_B_AE))
                    {
                        return 719;
                    }

                    unsigned char SC = *lptmp++;
                    unsigned char FC[FC_LEN] = {0};
                    memcpy(FC, lptmp, FC_LEN);
                    lptmp += FC_LEN;
                    switch(SC)
                    {
                        case SC_U_A:
                        {
                            //APDU || T
                            unsigned char T_A[T_LEN] = {0};
                            XdlmsInitResLen = len3 - SH_LEN - T_LEN;
                            memcpy(ase_gcm_128->S, lptmp, XdlmsInitResLen);
                            lptmp += XdlmsInitResLen;
                            memcpy(T_A, lptmp, T_LEN);

                            //check T
				ase_gcm_128->Decrypt_StringData(SC_U_A, (char*)mSecurityOption.SecurityMaterial.EK, (char*)mAareParams.RespondingAPTitle, (char*)FC,
					(char*)mSecurityOption.SecurityMaterial.AK, XdlmsInitResLen, (char*)ase_gcm_128->T);
                            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_A, mAareParams.RespondingAPTitle,
                            //                FC, mSecurityOption.SecurityMaterial.EK,
                            //                mSecurityOption.SecurityMaterial.AK, XdlmsInitRes, XdlmsInitResLen);

                            if(memcmp(ase_gcm_128->T, T_A, T_LEN) != 0)
                            {
                                return 721;
                            }
                            lptmp = XdlmsInitRes;
                        }
                        break;
                        case SC_U_E:
                        {
                            //CIPHER
                            XdlmsInitResLen = len3 - SH_LEN;
                            memcpy(ase_gcm_128->S, lptmp, XdlmsInitResLen);
				ase_gcm_128->Decrypt_StringData(SC_U_E, (char*)mSecurityOption.SecurityMaterial.EK, (char*)mAareParams.RespondingAPTitle, (char*)FC,
					(char*)mSecurityOption.SecurityMaterial.AK, XdlmsInitResLen, (char*)ase_gcm_128->T);

                            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_E, mAareParams.RespondingAPTitle,
                            //                FC, mSecurityOption.SecurityMaterial.EK,
                            //                mSecurityOption.SecurityMaterial.AK, XdlmsInitRes, XdlmsInitResLen);

                            //memcpy(ase_gcm_128->S, ase_gcm_128->TextAfter, ase_gcm_128->TextAfterLen);
                            lptmp = ase_gcm_128->S;
                        }
                        break;
                        case SC_U_AE:
                        {
                            //CIPHER || T
                            unsigned char T_AE[T_LEN] = {0};
                            XdlmsInitResLen = len3 - SH_LEN - T_LEN;
                            memcpy(ase_gcm_128->S, lptmp, XdlmsInitResLen);
                            lptmp += XdlmsInitResLen;
                            memcpy(T_AE, lptmp, T_LEN);
				ase_gcm_128->Decrypt_StringData(SC_U_AE, (char*)mSecurityOption.SecurityMaterial.EK, (char*)mAareParams.RespondingAPTitle, (char*)FC,
					(char*)mSecurityOption.SecurityMaterial.AK, XdlmsInitResLen, (char*)ase_gcm_128->T);
	
                            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_AE, mAareParams.RespondingAPTitle,
                             //               FC, mSecurityOption.SecurityMaterial.EK,
                             //               mSecurityOption.SecurityMaterial.AK, XdlmsInitRes, XdlmsInitResLen);

                            if(memcmp(ase_gcm_128->T, T_AE, T_LEN) != 0)
                            {
                                return 721;
                            }
                            //memcpy(ase_gcm_128->TextAfter, ase_gcm_128->S, ase_gcm_128->TextAfterLen);
                            lptmp = ase_gcm_128->S;
                        }
                        break;
                        case SC_B_A:break;
                        case SC_B_E:break;
                        case SC_B_AE:break;
                    }
/*                    unsigned long Counter = (((unsigned long)FC[0]<<24)|((unsigned long)FC[1]<<16)|((unsigned long)FC[2]<<8)|((unsigned long)FC[3]));
                    Counter += 1;
                    FC[0] = (unsigned char)(Counter>>24);
                    FC[1] = (unsigned char)(Counter>>16);
                    FC[2] = (unsigned char)(Counter>>8);
                    FC[3] = (unsigned char)(Counter);
                    memcpy(mSecurityOption.SecurityMaterial.IV.params.FC, FC, FC_LEN);  */
                }


                if(!(*lptmp++ == INITIATE_RESPONSE && *lptmp++ == 0x00))
                {
                    return 717;
                }
                bRet = OutRoot.AddElem("DLMS_Version_Number");
                if(*lptmp < mAarqParams.XdlmsInitiateReq.DlmsVersionNumber)
                {
                    mAareParams.XdlmsInitiateRes.DlmsVersionNumber = *lptmp++;
                }
                else
                {
                    mAareParams.XdlmsInitiateRes.DlmsVersionNumber = mAarqParams.XdlmsInitiateReq.DlmsVersionNumber;
                    lptmp++;
                }

				char szBuf[64] = { 0 };
				sprintf_s(szBuf, "%d", mAareParams.XdlmsInitiateRes.DlmsVersionNumber);
                OutRoot.AddAttrib("Value", szBuf);
                if(!(*lptmp++ == 0x5F && *lptmp++ == 0x1F && *lptmp++ == 0x04 && *lptmp++ == 0x00))
                {
                    return 718;
                }
                for(int i = 0; i < 3; i ++)
                {
                    mAareParams.XdlmsInitiateRes.Conformance.Val[i] = *lptmp++;
                }
                for(int i = 0; i < 3; i ++)
                {
                    if(mAareParams.XdlmsInitiateRes.Conformance.Val[i] > mAarqParams.XdlmsInitiateReq.Conformance.Val[i])
                    {
                        return 722;
                    }
                }
        
                bRet = OutRoot.AddElem("DLMS_Conformance");
				char szBufTemp[64] = { 0 };
				sprintf_s(szBufTemp, "%02x%02x%02x", mAareParams.XdlmsInitiateRes.Conformance.Val[0],
					mAareParams.XdlmsInitiateRes.Conformance.Val[1],
					mAareParams.XdlmsInitiateRes.Conformance.Val[2]);

                OutRoot.AddAttrib("Value", szBufTemp);

                unsigned int size;
                lptmp += DeCodeSizedUnsigned(&size, 2, lptmp);
                if(size == 0)
                {
                    #ifdef CLIENT_LONG_TRANSFER_SUPPORT
                    mClientMaxSendPduSize = 65535;
                    #else
                    mClientMaxSendPduSize = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT - 3;
                    #endif
                }
                else
                {
                    #ifdef CLIENT_LONG_TRANSFER_SUPPORT
                    mClientMaxSendPduSize = size;
                    #else
                    //如果客户端不允许分帧,则强制pdu的长度小于一帧中信息域的长度(考虑llchead)
                    mClientMaxSendPduSize = (DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT - 3) <= size ? (DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT - 3): size;
                    #endif
                }
                mAareParams.XdlmsInitiateRes.ServiceMaxReceivedPduSize = mClientMaxSendPduSize;
                //----------------------------------------------------------
                mApduToHdlcBuff = new unsigned char[mClientMaxSendPduSize];
                mApduToHdlcLen = 0;
                //--------------------------------------------------------------

				bRet = OutRoot.AddElem("Server_Max_Receive_Pdu_Size");
				sprintf_s(szBuf, "%d", size);
				OutRoot.AddAttrib("Value", szBuf);
            
                unsigned int vaa;
                lptmp += DeCodeSizedUnsigned(&vaa, 2, lptmp);
				bRet = OutRoot.AddElem("Vaa_Name");
				sprintf_s(szBuf, "%d", vaa);
				OutRoot.AddAttrib("Value", szBuf);
                goto AAREOVER;
            }
            //break;
            default:
            break;
        }
    }

AAREOVER:
    if(mAareParams.ApplicationContextName == LN_CIPER || mAareParams.ApplicationContextName == SN_CIPER)
    {
        if(RespondingAPTitlePresent == false)
        {
            ResetCosemAppAssociation();
            return 723; 
        }
    }

    if(mAareParams.AssociationResult == ACCEPTED)
    {
        //成功建立
        mCosemAppState = ASSOCIATED;
    }
    else
    {
        //建立失败
        ResetCosemAppAssociation();
    }

    if(mAarqParams.MechanismName > LOW_LEVEL)
    {
        /* yanshiqi
		mOutRoot = OutRoot->ChildNodes->Get(0)->CloneNode(true);
        OutRoot->ChildNodes->Delete(0);*/


        //检查一致性模块
        if(mAareParams.XdlmsInitiateRes.Conformance.bits.Action == 0)
        {
            return 731;
        }
        mCosemAppState = HLS_ASSOCIATION_PENDING;

        //f(StoC)
                                 if(mAarqParams.XdlmsInitiateReq.DedicatedKeyLen == 0)
					memcpy(EK_Buf,mSecurityOption.SecurityMaterial.EK,KEY_LEN);
				    else
					memcpy(EK_Buf,mSecurityOption.SecurityMaterial.DK,KEY_LEN);
	unsigned char type = SC_U_A;				
	  memcpy(ase_gcm_128->S,&type,SC_LEN);		
	  memcpy(ase_gcm_128->S+SC_LEN,mSecurityOption.SecurityMaterial.AK,KEY_LEN);
        memcpy(ase_gcm_128->S+KEY_LEN+SC_LEN,mAareParams.RespondingAuthenticationValue,mAareParams.RespondingAuthenticationValueLen);
        ase_gcm_128->Encrypt_StringData(SC_U_A, (char*)EK_Buf, (char*)mAarqParams.CallingAPTitle, (char*)mSecurityOption.SecurityMaterial.IV.params.FC,
			(char*)mSecurityOption.SecurityMaterial.AK, mAareParams.RespondingAuthenticationValueLen, (char*)ase_gcm_128->T);

       // ase_gcm_128->AES_GCM_Encryption(TYPE_ENCRYPTION, SC_U_A, mAarqParams.CallingAPTitle,
        //                                    mSecurityOption.SecurityMaterial.IV.params.FC, mSecurityOption.SecurityMaterial.EK,
       //                                     mSecurityOption.SecurityMaterial.AK, mAareParams.RespondingAuthenticationValue, mAareParams.RespondingAuthenticationValueLen);

        mAarqParams.F_StoC[0] = SC_U_A;
        memcpy(mAarqParams.F_StoC + SC_LEN, mSecurityOption.SecurityMaterial.IV.params.FC, FC_LEN);
        memcpy(mAarqParams.F_StoC + SH_LEN, ase_gcm_128->T, T_LEN);

        //调用15类方法1
        int Result = MakeHLSApdu();
        if(Result != 0)
        {
            return Result;
        }

        //-----------------------------------------------------------------------------------
        if(mSupportLayerType == HDLC)
        {
            return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
        }
        else if(mSupportLayerType == UDPIP)
        {
            return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
        }
        else
        {
            return 1;
        }  
    }
    else
    {
        //保存XML
        //OutRoot = mOutRoot->CloneNode(true);
        return -1;
    }
}




int CStack62056::ProcessRLRE(CMarkup& OutRoot, const unsigned char *lpRlre, unsigned int RlreLen)
{
    unsigned char	EK_Buf[KEY_LEN];
    if(mCosemAppState != ASSOCIATION_RELEASE_PENDING)
    {
        return 900;
    }
    //解析RLRE
    unsigned int lentmp;
    unsigned int RlreApduLen;

    const unsigned char *lptmp = lpRlre;
    lentmp = DeCodeLength(&RlreApduLen, lptmp);
    if(lentmp + RlreApduLen > RlreLen)
    {
        return 901;
    }
    lptmp += lentmp;

    bool ResponderAcseRequirement = false;

	OutRoot.IntoElem();
    bool bRet = OutRoot.AddElem("XML");
	OutRoot.IntoElem();
    bRet = OutRoot.AddElem("COSEM_RELEASE_CNF");
	
    while(lptmp < (lpRlre + RlreLen))
    {
        switch(*lptmp++)
        {
            case 0x80:
            {
                if(!(*lptmp++ == 0x01 && (*lptmp == RLRQ_REASON_NORMAL || *lptmp == RLRQ_REASON_URGENT || *lptmp == RLRQ_REASON_USER_DEFINED)))
                {
                    return 902;
                }
                else
                {
                    bRet = OutRoot.AddElem("RELEASE_RESPONSE_REASON");
					char szBuf[64] = { 0 };
					sprintf_s(szBuf, "%d", *lptmp++);
					OutRoot.AddAttrib("Value", szBuf);
                }
            }
            break;
            case 0xBE:
            {
                unsigned int len1, len2, len3;
                unsigned char XdlmsInitRes[256] = {0};
                unsigned short  XdlmsInitResLen = 0;

                
                lentmp = DeCodeLength(&len1, lptmp);
                lptmp += lentmp;
                if(*lptmp++ != 0x04)
                {
                    return 903;
                }
                lentmp = DeCodeLength(&len2, lptmp);
                lptmp += lentmp;

                if((mAarqParams.ApplicationContextName == LN_CIPER || mAarqParams.ApplicationContextName == SN_CIPER)&&(mSecurityOption.SecurityPolicy != NO_SECURITY)) //cipher
                {
                    if(*lptmp++ != GLO_INITIATE_RESPONSE)
                    {
                        return 717;
                    }
                    lentmp = DeCodeLength(&len3, lptmp);
                    lptmp += lentmp;

                    if(!(*lptmp == SC_U_A || *lptmp == SC_U_E || *lptmp == SC_U_AE || *lptmp == SC_B_A || *lptmp == SC_B_E || *lptmp == SC_B_AE))
                    {
                        return 719;
                    }
			/*加密密钥的选择*/		
			if(mAarqParams.XdlmsInitiateReq.DedicatedKeyLen == 0)
					memcpy(EK_Buf,mSecurityOption.SecurityMaterial.EK,KEY_LEN);
				    else
					memcpy(EK_Buf,mSecurityOption.SecurityMaterial.DK,KEY_LEN);
					
                    unsigned char SC = *lptmp++;
                    unsigned char FC[FC_LEN] = {0};
                    memcpy(FC, lptmp, FC_LEN);
                    lptmp += FC_LEN;
                    switch(SC)
                    {
                        case SC_U_A:
                        {
                            //APDU || T
                            unsigned char T_A[T_LEN] = {0};
                            XdlmsInitResLen = len3 - SH_LEN - T_LEN;
                            memcpy(ase_gcm_128->S, lptmp, XdlmsInitResLen);
                            lptmp += XdlmsInitResLen;
                            memcpy(T_A, lptmp, T_LEN);

                            //check T	
				 ase_gcm_128->Decrypt_StringData(SC_U_A, (char*)EK_Buf, (char*)mAareParams.RespondingAPTitle,
					 (char*)FC, (char*)mSecurityOption.SecurityMaterial.AK,XdlmsInitResLen, (char*)ase_gcm_128->T);

                            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_A, mAareParams.RespondingAPTitle,
                              //              FC, mSecurityOption.SecurityMaterial.EK,
                             //               mSecurityOption.SecurityMaterial.AK, XdlmsInitRes, XdlmsInitResLen);

                            if(memcmp(ase_gcm_128->T, T_A, T_LEN) != 0)
                            {
                                return 721;
                            }
                            lptmp = XdlmsInitRes;
                        }
                        break;
                        case SC_U_E:
                        {
                            //CIPHER
                            XdlmsInitResLen = len3 - SH_LEN;
                            memcpy(ase_gcm_128->S, lptmp, XdlmsInitResLen);
				ase_gcm_128->Decrypt_StringData(SC_U_E, (char*)EK_Buf, (char*)mAareParams.RespondingAPTitle,
					(char*)FC, (char*)mSecurityOption.SecurityMaterial.AK,XdlmsInitResLen, (char*)ase_gcm_128->T);

                            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_E, mAareParams.RespondingAPTitle,
                             //               FC, mSecurityOption.SecurityMaterial.EK,
                             //               mSecurityOption.SecurityMaterial.AK, XdlmsInitRes, XdlmsInitResLen);

                            //memcpy(XdlmsInitRes, ase_gcm_128->TextAfter, ase_gcm_128->TextAfterLen);
                            lptmp = XdlmsInitRes;
                        }
                        break;
                        case SC_U_AE:
                        {
                            //CIPHER || T
                            unsigned char T_AE[T_LEN] = {0};
                            XdlmsInitResLen = len3 - SH_LEN - T_LEN;
                            memcpy(ase_gcm_128->S, lptmp, XdlmsInitResLen);
                            lptmp += XdlmsInitResLen;
                            memcpy(T_AE, lptmp, T_LEN);
				ase_gcm_128->Decrypt_StringData(SC_U_AE, (char*)EK_Buf, (char*)mAareParams.RespondingAPTitle,
					(char*)FC, (char*)mSecurityOption.SecurityMaterial.AK,XdlmsInitResLen, (char*)ase_gcm_128->T);

                            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_AE, mAareParams.RespondingAPTitle,
                             //               FC, mSecurityOption.SecurityMaterial.EK,
                              //              mSecurityOption.SecurityMaterial.AK, XdlmsInitRes, XdlmsInitResLen);

                            if(memcmp(ase_gcm_128->T, T_AE, T_LEN) != 0)
                            {
                                return 721;
                            }
                            //memcpy(XdlmsInitRes, ase_gcm_128->TextAfter, ase_gcm_128->TextAfterLen);
                            memcpy(XdlmsInitRes,ase_gcm_128->S,XdlmsInitResLen);
                            lptmp = XdlmsInitRes;
                        }
                        break;
                        case SC_B_A:break;
                        case SC_B_E:break;
                        case SC_B_AE:break;
                    }
                }
                
                if(!(*lptmp++ == 0x08 && *lptmp++ == 0x00))
                {
                    return 904;
                }
                bRet = OutRoot.AddElem("DLMS_Version_Number");
                if(*lptmp < mAarqParams.XdlmsInitiateReq.DlmsVersionNumber)
                {
                    mAarqParams.XdlmsInitiateReq.DlmsVersionNumber = *lptmp++;
                }
                else
                {
                    lptmp++;
                }
				char szBuf[64] = { 0 };
				sprintf_s(szBuf, "%d", mAarqParams.XdlmsInitiateReq.DlmsVersionNumber);
				OutRoot.AddAttrib("Value", szBuf);
                if(!(*lptmp++ == 0x5F && *lptmp++ == 0x1F && *lptmp++ == 0x04 && *lptmp++ == 0x00))
                {
                    return 718;
                }

        
				bRet = OutRoot.AddElem("DLMS_Conformance");
				char szBufTemp[64] = { 0 };
				sprintf_s(szBufTemp, "%02x%02x%02x", *lptmp, *(lptmp + 1), *(lptmp + 2));
				OutRoot.AddAttrib("Value", szBufTemp);
                lptmp += 3;
                
                unsigned int size;
                lptmp += DeCodeSizedUnsigned(&size, 2, lptmp);

				bRet = OutRoot.AddElem("Server_Max_Receive_Pdu_Size");
				sprintf_s(szBuf, "%d", size);
				OutRoot.AddAttrib("Value", szBuf);
            
                unsigned int vaa;
                lptmp += DeCodeSizedUnsigned(&vaa, 2, lptmp);
				bRet = OutRoot.AddElem("Vaa_Name");
				sprintf_s(szBuf, "%d", vaa);
				OutRoot.AddAttrib("Value", szBuf);
                goto RLREOVER;
            }
            break;
            default:
            break;
        }
    }
    //
RLREOVER:
    ResetCosemAppAssociation();

    return -1;
}


int CStack62056::ProcessEventNotificationInd(CMarkup& OutRoot, const unsigned char *lpNotificationInd, unsigned int NotificationIndLen)
{
    const unsigned char *lptmp = lpNotificationInd;
    unsigned int tmplen;

	OutRoot.IntoElem();
    bool bRet = OutRoot.AddElem("XML");
	OutRoot.IntoElem();
	bRet = OutRoot.AddElem("EVENT_NOTIFICATION_IND");

    if(*lptmp++ != 0x00)
    {
        //解析时间
        std::string DateTime;
        if(*lptmp++ == 0x09)
        {
            lptmp += DeCodeOctetStr(DateTime, lptmp);
        }
        else
        {
            return 554;
        }
        bRet = OutRoot.AddElem("Time");
        OutRoot.AddAttrib("Value", DateTime.c_str());
    }
    unsigned int uValue;
    int iValue;

    unsigned short ClassId;
    lptmp += DeCodeSizedUnsigned(&uValue, 2, lptmp);
    ClassId = uValue;
                     
    std::string InstanceId;
    lptmp += DeCodeSizedOctetStr(InstanceId, 6, lptmp);

    char AttributeId;
    lptmp += DeCodeSizedInteger((__int64 *)&iValue, 1, lptmp);
    AttributeId = iValue;

	char szBuf[64] = { 0 };
    bRet = OutRoot.AddElem("COSEM_Attribute_Descriptor");
	OutRoot.IntoElem();
	bRet = OutRoot.AddElem("COSEM_Class_Id");
	sprintf_s(szBuf, "%d", ClassId);
	OutRoot.AddAttrib("Value", szBuf);

	bRet = OutRoot.AddElem("COSEM_Instance_Id");
	OutRoot.AddAttrib("Value", InstanceId);

	bRet = OutRoot.AddElem("COSEM_Attribute_Id");
	sprintf_s(szBuf, "%d", AttributeId);
	OutRoot.AddAttrib("Value", szBuf);

	bRet = OutRoot.AddElem("Value");
    tmplen = NotificationIndLen - (lptmp - lpNotificationInd);
    lptmp += DecodeAndWriteData(tmplen, lptmp, OutRoot);

    //assert(lptmp - lpNotificationInd != NotificationIndLen);

    return -1;
}

int CStack62056::ProcessPushData(CMarkup& OutRoot, const unsigned char *lpNotificationInd, unsigned int NotificationIndLen)
{
    const unsigned char *lptmp = lpNotificationInd;
    unsigned int tmplen;

	OutRoot.IntoElem();
    bool bRet = OutRoot.AddElem("XML");
	OutRoot.IntoElem();
	bRet = OutRoot.AddElem("PUSH_DATA");

    //Client address
    bRet = OutRoot.AddElem("ClientAddr");
    OutRoot.AddAttrib("Value", mAddr.HDLC_ADDR2.ClientAddr);

    //Long id (short id)
    std::string LongId;
    lptmp += DeCodeSizedOctetStr(LongId, 4, lptmp);
    bRet = OutRoot.AddElem("LongID");
	OutRoot.AddAttrib("Value", LongId);

    //datetime
    std::string DateTime;
    lptmp++;
    lptmp += DeCodeSizedOctetStr(DateTime, 12, lptmp);
	bRet = OutRoot.AddElem("Time");
	OutRoot.AddAttrib("Value", DateTime);

    bRet = OutRoot.AddElem("Value");
    //Data of Push object
    tmplen = NotificationIndLen - (lptmp - lpNotificationInd);
    lptmp += DecodeAndWriteData(tmplen, lptmp, OutRoot);

    //assert(lptmp - lpNotificationInd != NotificationIndLen);

    return -1;
}

int CStack62056::ProcessReadRes(CMarkup& OutRoot, const unsigned char *lpReadRes, unsigned int ReadResLen)
{
    //1.检查状态
        if(mAareParams.MechanismName > LOW_LEVEL)
    {
        if(mCosemAppState != HLS_ASSOCIATED)
        {
            return 319;
        }
    }
    else
    {
        if(mCosemAppState != ASSOCIATED)
        {
            return 319;
        }
    }

    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == LN_NOCIPER || mAarqParams.ApplicationContextName == LN_CIPER)
    {
        return 318;
    }

    //3.检查一致性模块
    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Read == 0)
    {
        return 318;
    }

    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == false)
    {
        return 330;
    }
    //----------------
    unsigned int lentmp, Count;
    int Result;
    const unsigned char *lptmp = lpReadRes;

    lptmp += DeCodeLength(&Count, lptmp);

	OutRoot.IntoElem();
    bool bRet = OutRoot.AddElem("XML");
	OutRoot.IntoElem();
	bRet = OutRoot.AddElem("READ_CNF");
	OutRoot.IntoElem();
    bRet = OutRoot.AddElem("Results");
	OutRoot.IntoElem();
    
    for(unsigned int i = 0; i < Count; i ++)
    {
        bRet = OutRoot.AddElem("Result");
        unsigned char Choice = *lptmp++;
        if(Choice == 0x00)
        {
            Result = DecodeAndWriteData(lentmp, lptmp, OutRoot);
            if(Result != 0)
            {
                return Result;
            }
            lptmp += lentmp;
        }
        else if(Choice == 0x01)
        {
            Result = DecodeAndWriteDataAccessResult(lentmp, lptmp, OutRoot);
            if(Result != 0)
            {
                return Result;
            }
            lptmp += lentmp;
        }
        else
        {
            return 417;
        }
        if(lptmp - lpReadRes > (int)ReadResLen)
        {
            return 418;
        }
    }
    return 0;
}

int CStack62056::ProcessWriteRes(CMarkup& OutRoot, const unsigned char *lpWriteRes, unsigned int WriteResLen)
{
    //1.检查状态
       if(mAareParams.MechanismName > LOW_LEVEL)
    {
        if(mCosemAppState != HLS_ASSOCIATED)
        {
            return 319;
        }
    }
    else
    {
        if(mCosemAppState != ASSOCIATED)
        {
            return 319;
        }
    }

    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == LN_NOCIPER || mAarqParams.ApplicationContextName == LN_CIPER)
    {
        return 318;
    }

    //3.检查一致性模块
    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Write == 0)
    {
        return 318;
    }

    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == false)
    {
        return 330;
    }
    //----------------
    unsigned int lentmp, Count;
    int Result;
    const unsigned char *lptmp = lpWriteRes;

    lptmp += DeCodeLength(&Count, lptmp);


	OutRoot.IntoElem();
	bool bRet = OutRoot.AddElem("XML");
	OutRoot.IntoElem();
    bRet = OutRoot.AddElem("READ_CNF");
	OutRoot.IntoElem();
    bRet = OutRoot.AddElem("Results");
	OutRoot.IntoElem();
    
    for(unsigned int i = 0; i < Count; i ++)
    {
        bRet = OutRoot.AddElem("Result");
        unsigned char Choice = *lptmp++;
        if(Choice == 0x00)
        {
        }
        else if(Choice == 0x01)
        {
            Result = DecodeAndWriteDataAccessResult(lentmp, lptmp, OutRoot);
            if(Result != 0)
            {
                return Result;
            }
            lptmp += lentmp;
        }
        else
        {
            return 417;
        }
        if(lptmp - lpWriteRes > (int)WriteResLen)
        {
            return 418;
        }
    }
    return 0;
}


int CStack62056::ProcessInformationReportInd(CMarkup& OutRoot, const unsigned char *lpInformationReportInd, unsigned int InformationReportIndLen)
{
    const unsigned char *lptmp = lpInformationReportInd;
    unsigned int tmplen;


	OutRoot.IntoElem();
	bool bRet = OutRoot.AddElem("XML");
	OutRoot.IntoElem();
    bRet = OutRoot.AddElem("INFORMATION_REPORT_IND");
	OutRoot.IntoElem();

    if(*lptmp++ != 0x00)
    {
        //解析时间
        //AnsiString DateTime;
        //lptmp += DeCodeSizedOctetStr(DateTime, 12, lptmp);
        //Node = Root->AddChild("Time");
        //Node->SetAttribute("Value", DateTime);
    }
    
    bRet = OutRoot.AddElem("Variable_Access_Specifications");
    lptmp += DeCodeLength(&tmplen, lptmp);
    for(unsigned int i = 0; i < tmplen; i ++)
    {
        lptmp += DecodeAndWriteVariableAccessSpecification(tmplen,lptmp, OutRoot);
    }

    bRet = OutRoot.AddElem("Values");
    lptmp += DeCodeLength(&tmplen, lptmp);
    for(unsigned int i = 0; i < tmplen; i ++)
    {
        lptmp += DecodeAndWriteData(tmplen,lptmp, OutRoot);
    }
       
    return -1;
}


int CStack62056::ProcessConfirmedServiceError(CMarkup& OutRoot, const unsigned char *lpConfirmedServiceError, unsigned int ConfirmedServiceErrorLen)
{
    return 0;
//    if(GetResLen < 2)
//        return 502;
//
//    int Result;
//    const unsigned char *lptmp = lpGetRes;
//    unsigned int tmplen;// = GetResLen;
//    int ResponseType = *lptmp++;
//    if(ResponseType < GET_RES_NORMAL || ResponseType > GET_RES_WITH_LIST)
//    {
//        return 503;
//    }
//    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;
//    InvokeIdAndPriority.Val = *lptmp++;
}
//---

//---
void CStack62056::WriteDataResHead(int InvokeId, bool Priority, int ResponseType, CMarkup& Root)
{
	char szBuf[64] = { 0 };
	Root.IntoElem();
    bool bRet = Root.AddElem("Invoke_Id");
	sprintf_s(szBuf, "%d", InvokeId);
    Root.AddAttrib("Value", szBuf);

    bRet = Root.AddElem("Priority");
	sprintf_s(szBuf, "%d", Priority?1:0);
	Root.AddAttrib("Value", szBuf);

    bRet = Root.AddElem("Response_Type");
	sprintf_s(szBuf, "%d", ResponseType);
	Root.AddAttrib("Value", szBuf);
	Root.OutOfElem();
}

//Len既是输入也是输出   Parent 为Value节点
int CStack62056::DecodeAndWriteData(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent,
	                    std::string NodeName)
{
    if(lpSrc == NULL || Len == 0)
    {
        return 800;
    }

	const unsigned char *lptmp = lpSrc;

	unsigned int LayerChildNum[32] = {0};
    LayerChildNum[0] = 1;
	int depth = 0;
	int Over = 0;//0--继续  1--完毕  -1--出错
    int Tag = 0;

    std::string Str;
    unsigned int Count;	

	Parent.IntoElem();
    bool bRet = Parent.AddElem(NodeName);

	do
	{
		Tag = *lptmp++;
		if(depth > 0)		LayerChildNum[depth]--;
		switch(Tag)
		{
			case 1:
			{
				char szBuf[64] = { 0 };
				lptmp += DeCodeLength(&Count, lptmp);
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "array");
				sprintf_s(szBuf, "%d", Count);
				Parent.AddAttrib("Value", szBuf);
				depth++;
				LayerChildNum[depth] = Count;
			}
			break;
			case 2:
			{
				char szBuf[64] = { 0 };
				lptmp += DeCodeLength(&Count, lptmp);
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "structure");
				sprintf_s(szBuf, "%d", Count);
				Parent.AddAttrib("Value", szBuf);
				depth++;
				LayerChildNum[depth] = Count;
			}
			break;
			case 3:
			{
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "boolean");
				if (*lptmp++ == 0x00)
				{
					Parent.AddAttrib("Value", "0");
				}
				else
				{
					Parent.AddAttrib("Value", "1");
				}
			}
			break;
			case 4:
			{
				lptmp += DeCodeBit(Str, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "bit-string");
				Parent.AddAttrib("Value", Str);
			}
			break;
			case 5:
			{
				int Value = 0;
				lptmp += DeCodeSizedInteger((__int64 *)&Value, 4, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				sprintf_s(szBuf, "%lld", (__int64)Value);
				Parent.AddAttrib("Type", "double-long");
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 6:
			{
				unsigned int Value = 0;
				lptmp += DeCodeSizedUnsigned(&Value, 4, lptmp);
				lptmp += DeCodeSizedInteger((__int64 *)&Value, 4, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				sprintf_s(szBuf, "%lld", (__int64)Value);
				Parent.AddAttrib("Type", "double-long-unsigned");
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 9:
			{
				lptmp += DeCodeOctetStr(Str, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "octet-string");
				Parent.AddAttrib("Value", Str);
			}
			break;
			case 10:
			{
				lptmp += DeCodeVisibleStr(Str, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "visible-string");
				Parent.AddAttrib("Value", Str);
			}
			break;
			case 13:
			{
				char Value = 0;
				lptmp += DeCodeSizedInteger((__int64 *)&Value, 1, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "bcd");
				sprintf_s(szBuf, "%d", Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 15:
			{
				char Value = 0;
				lptmp += DeCodeSizedInteger((__int64 *)&Value, 1, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "integer");
				sprintf_s(szBuf, "%d", Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 16:
			{
				short Value = 0;
				lptmp += DeCodeSizedInteger((__int64 *)&Value, 2, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "long");
				sprintf_s(szBuf, "%d", Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 17: //unsigned
			{
				unsigned char Value = 0;
				lptmp += DeCodeSizedUnsigned((unsigned int *)&Value, 1, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "unsigned");
				sprintf_s(szBuf, "%d", Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 18:
			{
				unsigned short Value = 0;
				lptmp += DeCodeSizedUnsigned((unsigned int *)&Value, 2, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "long-unsigned");
				sprintf_s(szBuf, "%d", Value);
				Parent.AddAttrib("Value",szBuf);
			}
			break;
			case 19:
				break;
			case 20: //long64
			{
				//int Value = (*(Data + 4) << 24) + (*(Data + 5) << 16) + (*(Data + 6) << 8) +  (*(Data + 7) << 0);
				//Data += 8;
				//long long int Value = 0;
				//Data += DeCodeSizedInteger((void *)&Value, 8,Data);
				//AddAttrTextChildNode(Node, NodeName, "Type", "Integer64", IntToStr(Value));
				long long int Value = 0;
				lptmp += DeCodeSizedInteger((__int64 *)&Value, 8, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "long64");
				sprintf_s(szBuf, "%lld", Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 21: //long64-unsigned
			{
				//unsigned int Value = (*(Data + 4) << 24) + (*(Data + 5) << 16) + (*(Data + 6) << 8) +  (*(Data + 7) << 0);
				//Data += 8;
				//unsigned long long int Value = 0;
				//Data += DeCodeSizedInteger((void *)&Value, 8,Data);
				//Data += DeCodeSizedInteger(&Value, 2,Data);
				//AddAttrTextChildNode(Node, NodeName, "Type", "Unsigned64", IntToStr(Value));
				unsigned long long int Value = 0;
				lptmp += DeCodeSizedInteger((__int64 *)&Value, 8, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "long64-unsigned");
				sprintf_s(szBuf, "%lld", Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 22:
			{
				unsigned char Value = 0;
				lptmp += DeCodeSizedUnsigned((unsigned int *)&Value, 1, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "enum");
				sprintf_s(szBuf, "%lld", (__int64)Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 23:
			{
				float Value;
				lptmp += DeCodeFloat32(&Value, lptmp);
				//AddAttrTextChildNode(Node, NodeName, "Type", "float32", AnsiString((char *)Str));
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "float32");
				sprintf_s(szBuf, "%f", Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 24:
			{
				float Value;
				lptmp += DeCodeFloat64(&Value, lptmp);
				//AddAttrTextChildNode(Node, NodeName, "Type", "float32", AnsiString((char *)Str));
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "float64");
				sprintf_s(szBuf, "%lf", Value);
				Parent.AddAttrib("Value", szBuf);
			}
			break;
			case 25:
			{
				lptmp += DeCodeDateTime(Str, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "date_time");
				Parent.AddAttrib("Value", Str);
			}
			break;
			case 26:
			{
				lptmp += DeCodeDate(Str, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "date");
				Parent.AddAttrib("Value", Str);
			}
			break;
			case 27:
			{
				lptmp += DeCodeTime(Str, lptmp);
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "time");
				Parent.AddAttrib("Value", Str);
			}
			break;
			case 0:
			{
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "null-data");
			}
			break;
			case 255:
			{
				char szBuf[64] = { 0 };
				Parent.IntoElem();
				sprintf_s(szBuf, "L%d", depth);
				bRet = Parent.AddElem(szBuf);
				Parent.AddAttrib("Type", "dont-care");
			}
			break;
			default:
			{
				Over = -1;
			}
			break;
		}//switch

		if (Over == -1)	break;

		while (LayerChildNum[depth] == 0 && depth > 0)
		{
			depth--;
			Parent.OutOfElem();
		}
		if (depth == 0) break;
	}while (/*(unsigned int)(lptmp - lpSrc) > Len */lpSrc + Len > lptmp && Over == 0);

	Len = (lptmp - lpSrc);
	return 0;
};

int CStack62056::DecodeAndWriteGetDataResult(unsigned int &Len, const unsigned char *lpSrc, CMarkup& ResultNode)
{
    const unsigned char *lptmp = lpSrc;
    int Result;
    unsigned int tmplen = Len;
	bool bRet = false;
	char szBuf[64] = { 0 };
    switch(*lptmp++)
    {
        case 0x00:
            //Node = ResultNode->AddChild("Data");
            //解析数据 。。。
            Result = DecodeAndWriteData(tmplen, lptmp, ResultNode);
            if(Result != 0)
            {
                return Result;
            }
            lptmp += tmplen;
            break;
        case 0x01:
			ResultNode.IntoElem();
            bRet = ResultNode.AddElem("Data_Access_Result");
			sprintf_s(szBuf, "%d", *lptmp++);
            bRet = ResultNode.AddAttrib("Value", szBuf);
            return -1;
        default:return 600;
    }

    Len = lptmp - lpSrc;
    return 0;
}

int CStack62056::ProcessGetRes(CMarkup& OutRoot, const unsigned char *lpGetRes, unsigned int GetResLen)
{
    //1.检查状态
    if((mAareParams.MechanismName <= LOW_LEVEL && mCosemAppState != ASSOCIATED)
    || (mAareParams.MechanismName >= HIGH_LEVEL && mCosemAppState != HLS_ASSOCIATED))
    {
        return 319;
    }

    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == SN_NOCIPER || mAarqParams.ApplicationContextName == SN_CIPER)
    {
        return 318;
    }

    //3.检查一致性模块
    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Get == 0)
    {
        return 318;
    }

    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == false)
    {
        return 330;
    }
    //--------------------
    if(GetResLen < 2)
        return 502; 

    int Result;
    const unsigned char *lptmp = lpGetRes;
    unsigned int tmplen;// = GetResLen;
    int ResponseType = *lptmp++;
    if(ResponseType < GET_RES_NORMAL || ResponseType > GET_RES_WITH_LIST)
    {
        return 503;
    }
    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;
    InvokeIdAndPriority.Val = *lptmp++;


    if(ResponseType == GET_RES_NORMAL || ResponseType == GET_RES_WITH_LIST)
    {
        if(mBlockReceive.Flag == true)//客户端是否处于分块流程
        {
            //认为服务器决定终止传输,并在其中说明原因 此时GetRes中应为Data-Access-Result,否则的话还是错误
            //此处不做这么细致的判断
            mBlockReceive.Flag = false;
            DeleteBlockRecvBuff();
        }


		OutRoot.IntoElem();
        bool bRet = OutRoot.AddElem("XML");
		OutRoot.IntoElem();
		bRet = OutRoot.AddElem("GET_CNF");
		OutRoot.IntoElem();
        WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, ResponseType, OutRoot);

        if(ResponseType == GET_RES_NORMAL)
        {
            if(GetResLen < 1)
            {
                return 502;
            }
            tmplen = GetResLen - (lptmp - lpGetRes);
            bRet = OutRoot.AddElem("Result");
            Result = DecodeAndWriteGetDataResult(tmplen, lptmp, OutRoot);
            if(Result != 0)
            {
                return Result;
            }
            lptmp += tmplen;
        }
        else
        {
            unsigned int ResultsCount, LengthEncodeBytes;
            LengthEncodeBytes = DeCodeLength(&ResultsCount, lptmp);
            lptmp += LengthEncodeBytes;
			bRet = OutRoot.AddElem("Results");
			OutRoot.IntoElem();
            for(unsigned int i = 0; i < ResultsCount; i ++)
            {
                bRet = OutRoot.AddElem("Result");
                Result = DecodeAndWriteGetDataResult(tmplen, lptmp, OutRoot);
                if(Result != 0)
                {
                    return Result;
                }
                lptmp += tmplen;
            }            
        } 
        //XmlToStr();
        return -1;
    }
    else//GET_RES_WITH_DATABLOCK
    {
        bool LastBlock = *lptmp++;
        unsigned int BlockNumber;
        lptmp += DeCodeSizedUnsigned(&BlockNumber, 4, lptmp);
        bool DataOrDataAccessResult = *lptmp++;//略过data
        if(DataOrDataAccessResult == 1)
        {
            //int DataAccessResult = *lptmp++;
            //异常情形,如果此时有块接收在进行中,结束;
            if(mBlockReceive.Flag == true)
            {
                mBlockReceive.Flag = false;
                DeleteBlockRecvBuff();
            }
            //直接解析最后一块
            bool bRet = OutRoot.AddElem("XML");
			OutRoot.IntoElem();
            bRet = OutRoot.AddElem("GET_CNF");
			OutRoot.IntoElem();
            WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority,
                    mLastDataRequest.RequestType, OutRoot);
            OutRoot.AddElem("Result");
            lptmp += DecodeAndWriteDataAccessResult(tmplen, lptmp, OutRoot);
            return -1;
        }
        else if(DataOrDataAccessResult == 0)
        {
            unsigned int RawDataLen;
            lptmp += DeCodeLength(&RawDataLen, lptmp);
            if(mBlockReceive.Flag == false)
            {
                if(BlockNumber != 1)
                {
                    return 507;
                }

                Result = InitBlockRecvBuff();
                if(Result != 0)
                {
                    return Result;
                }
                mBlockReceive.Flag = true;
                mBlockReceive.BlockNumber = 1;
                memcpy(mBlockRecvBuff, lptmp, RawDataLen);
                mBlockRecvLen = RawDataLen;
            }
            else
            {
                if(BlockNumber != mBlockReceive.BlockNumber + 1)
                {
                    return 508;
                }

                mBlockReceive.BlockNumber++;

                if(RawDataLen + mBlockRecvLen > mBlockRecvBuffSize)
                {
                    int Result = ReallocBlockRecvBuff();
                    if(Result != 0)
                    {
                        return Result;
                    }
                }
                //缓存内容 。。。
                memcpy(mBlockRecvBuff + mBlockRecvLen, lptmp, RawDataLen);
                mBlockRecvLen += RawDataLen;
            }

            if(LastBlock == false)
            {
                //发送getreqnext
                unsigned char *lptmp = mApduToHdlcBuff;
                *lptmp++ = GET_REQUEST;
                *lptmp++ = GET_REQ_NEXT;
                InvokeIdAndPriority.bits.ServiceClass = true;
                *lptmp++ = InvokeIdAndPriority.Val;
                lptmp += EnCodeSizedUnsigned(mBlockReceive.BlockNumber, 4, lptmp);
                mApduToHdlcLen = lptmp - mApduToHdlcBuff;

                //新增，
                DoEncryption(GET_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);


                if(mSupportLayerType == HDLC)
                {
                    return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else if(mSupportLayerType == UDPIP)
                {
                    return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else
                {
                    return 1;
                }              
            }
            else
            {
                mBlockReceive.Flag = false;

				OutRoot.IntoElem();
                bool bRet = OutRoot.AddElem("XML");
				OutRoot.IntoElem();
                bRet = OutRoot.AddElem("GET_CNF");
				OutRoot.IntoElem();


                if((GET_REQUEST_TYPE)mLastDataRequest.RequestType == GET_REQ_NORMAL)
                {
                    WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, GET_RES_NORMAL, OutRoot);
					bRet = OutRoot.AddElem("Result");
                    lptmp = mBlockRecvBuff;
                    /*if(*lptmp == 0)
                    {
                        lptmp++;
                        tmplen = mBlockRecvLen - (lptmp - mBlockRecvBuff);
                        Result = DecodeAndWriteData(tmplen, lptmp, ResultNode);
                        if(Result != 0)
                        {
                            return Result;
                        }
                        //assert(tmplen != mBlockRecvLen);
                        if(tmplen != mBlockRecvLen)
                        {
                            //???
                        }
                        DeleteBlockRecvBuff();
                    }
                    else if(*lptmp == 1)
                    {
                        lptmp++;
                        tmplen = mBlockRecvLen - (lptmp - mBlockRecvBuff);
                        int Result = DecodeAndWriteDataAccessResult(tmplen, lptmp, ResultNode);
                        if(Result != 0)
                        {
                            return Result;
                        }
                        //assert(tmplen != mBlockRecvLen);
                        if(tmplen != mBlockRecvLen)
                        {
                            //???
                        }
                        DeleteBlockRecvBuff();
                    }
                    else
                    {
                        return 801;
                    }*/
                    tmplen = mBlockRecvLen;
                    Result = DecodeAndWriteData(tmplen, lptmp, OutRoot);//DecodeAndWriteGetDataResult(tmplen, lptmp, ResultNode);
                    if(Result != 0)
                    {
                        return Result;
                    }
                    lptmp += tmplen;
                }
                else //GET_RES_WITH_LIST
                {
                    WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, GET_RES_WITH_LIST, OutRoot);

                    lptmp = mBlockRecvBuff;
                    unsigned int ResultsCount;
                    lptmp += DeCodeLength(&ResultsCount, lptmp);
                    bRet = OutRoot.AddElem("Results");
					OutRoot.IntoElem();
                    for(unsigned int i = 0; i < ResultsCount; i ++)
                    {
                        bRet = OutRoot.AddElem("Result");
                        Result = DecodeAndWriteGetDataResult(tmplen, lptmp, OutRoot);
                        if(Result != 0)
                        {
                            return Result;
                        }
                        lptmp += tmplen;
                    }  
                }
                return -1;
            }
        }
        else
        {
            return 549;
        }  
    }
}


int CStack62056::ProcessSetRes(CMarkup& OutRoot, const unsigned char *lpSetRes, unsigned int SetResLen)
{
    //1.检查状态
    if((mAareParams.MechanismName <= LOW_LEVEL && mCosemAppState != ASSOCIATED)
    || (mAareParams.MechanismName >= HIGH_LEVEL && mCosemAppState != HLS_ASSOCIATED))
    {
        return 319;
    }

    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAarqParams.ApplicationContextName == SN_NOCIPER || mAarqParams.ApplicationContextName == SN_CIPER)
    {
        return 318;
    }

    //3.检查一致性模块
    if(mAarqParams.XdlmsInitiateReq.Conformance.bits.Set == 0)
    {
        return 318;
    }

    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == false)
    {
        return 330;
    }
    //-------------------------
    int ResponseType = *lpSetRes++;
    if(ResponseType < SET_RES_NORMAL || ResponseType > SET_RES_WITH_LIST)
    {
        return 545;
    }
    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;
    InvokeIdAndPriority.Val = *lpSetRes++;
    SetResLen -= 2;

    int Result;
    if(ResponseType == SET_RES_NORMAL || ResponseType == SET_RES_WITH_LIST)
    {
        if(mBlockTransfer.Flag == true)
        {  
            mBlockTransfer.Flag = false;
            DeleteApduBuff();
        }

		OutRoot.IntoElem();
        bool bRet = OutRoot.AddElem("XML");
		OutRoot.IntoElem();
        bRet = OutRoot.AddElem("SET_CNF");
		OutRoot.IntoElem();
        WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, ResponseType, OutRoot);

        unsigned int tmplen;

        if(ResponseType == SET_RES_NORMAL)
        {
			bRet = OutRoot.AddElem("Result");
            Result = DecodeAndWriteDataAccessResult(tmplen, lpSetRes, OutRoot);
            if(Result != 0)
            {
                return Result;
            }
        }
        else //SET_RES_WITH_LIST
        {
            unsigned int ResultsCount, LengthEncodeBytes;
            LengthEncodeBytes = DeCodeLength(&ResultsCount, lpSetRes);
            lpSetRes += LengthEncodeBytes;
			bRet = OutRoot.AddElem("Results");
			OutRoot.IntoElem();
            for(unsigned int i = 0; i < ResultsCount; i ++)
            {
				bRet = OutRoot.AddElem("Result");
                Result = DecodeAndWriteDataAccessResult(tmplen, lpSetRes, OutRoot);
                if(Result != 0)
                {
                    return Result;
                }
                lpSetRes += tmplen;
            }              
        }
        //XmlToStr();
        return -1;
    }
    else
    {
        if(mBlockTransfer.Flag == false)
        {
            return 547;
        }

        if(ResponseType == SET_RES_DATABLOCK)
        {
            if(mBlockTransfer.Offset == mApduLen)//应该接收的是最后一块的响应
            {
                return 548;
            }
            unsigned int BlockNumber;
            lpSetRes += DeCodeSizedUnsigned(&BlockNumber, 4, lpSetRes);
            if(BlockNumber != mBlockTransfer.BlockNumber)
            {
                return 546;
            }
            unsigned char *lptmp = mApduToHdlcBuff;
            *lptmp++ = SET_REQUEST;
            *lptmp++ = SET_REQ_WITH_DATABLOCK;
            InvokeIdAndPriority.bits.ServiceClass = true;
            *lptmp++ = InvokeIdAndPriority.Val;
            mBlockTransfer.BlockNumber++;
            //-----------------------------------------

            //计算加密增量
            unsigned char EncryptionIncrement = 0;
            switch(mSecurityOption.SecurityPolicy)
            {  
                case ALL_MESSAGES_AUTHENTICATED:EncryptionIncrement = 21; break;
                case ALL_MESSAGES_ENCRYPTED:EncryptionIncrement = 9;break;
                case ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED:EncryptionIncrement = 21;break;
                default:EncryptionIncrement = 0;break;
            }
            
            mBlockTransfer.RawDataLen = CalcRawDataLen(mClientMaxSendPduSize - EncryptionIncrement, lptmp - mApduToHdlcBuff + 5);//加上last-block 和 block-number
            //一次发送不完,不是最后一块
            if(mApduLen - mBlockTransfer.Offset > mBlockTransfer.RawDataLen)
            {
                *lptmp++ = 0x00;//last-block
                lptmp += EnCodeSizedUnsigned(mBlockTransfer.BlockNumber, 4, lptmp);
                lptmp += EnCodeLength(mBlockTransfer.RawDataLen, lptmp);
                memcpy(lptmp, mApduBuff + mBlockTransfer.Offset, mBlockTransfer.RawDataLen);
                lptmp += mBlockTransfer.RawDataLen;
                mBlockTransfer.Offset += mBlockTransfer.RawDataLen;
                mApduToHdlcLen = mClientMaxSendPduSize - EncryptionIncrement;
            }
            else
            {
                *lptmp++ = 0xFF;//last-block
                lptmp += EnCodeSizedUnsigned(mBlockTransfer.BlockNumber, 4, lptmp);
                lptmp += EnCodeLength(mApduLen - mBlockTransfer.Offset, lptmp);
                memcpy(lptmp, mApduBuff + mBlockTransfer.Offset, mApduLen - mBlockTransfer.Offset);
                lptmp += mApduLen - mBlockTransfer.Offset;
                //mBlockTransfer.Flag = false;
                mApduToHdlcLen = lptmp - mApduToHdlcBuff;
            }

            //新增，
            DoEncryption(SET_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);
            //通知HDLC层发送
            if(mSupportLayerType == HDLC)
            {
                return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
            }
            else if(mSupportLayerType == UDPIP)
            {
                return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
            }
            else
            {
                return 1;
            }
        }
        else if(ResponseType == SET_RES_LAST_DATABLOCK)
        { 
            //解析内容 result
            int AccessResult = *lpSetRes++;                
            unsigned int BlockNumber;
            lpSetRes += DeCodeSizedUnsigned(&BlockNumber, 4, lpSetRes);
            if(BlockNumber != mBlockTransfer.BlockNumber)
            {
                return 546;
            }

            mBlockTransfer.Flag = false; //就算没有发完,收到该pdu也认为服务器需要提前结束流程
            DeleteApduBuff();
            
			OutRoot.IntoElem();
            bool bRet = OutRoot.AddElem("XML");
			OutRoot.IntoElem();
            bRet = OutRoot.AddElem("SET_CNF");
            WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, SET_RES_NORMAL, OutRoot);
			OutRoot.IntoElem();
			bRet = OutRoot.AddElem("Result");
			bRet = OutRoot.AddElem("Data_Access_Result");
			char szBuf[32] = { 0 };
			sprintf_s(szBuf, "%d", AccessResult);
			OutRoot.AddAttrib("Value", szBuf);

            return -1;

        }
        else //if(ResponseType == SET_RES_LAST_DATABLOCK_WITH_LIST)
        {   
            //解析内容 result
            unsigned int count;
            lpSetRes += DeCodeLength(&count, lpSetRes);
            
			OutRoot.IntoElem();
			bool bRet = OutRoot.AddElem("XML");
			bRet = OutRoot.AddElem("SET_CNF");
            WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, SET_RES_NORMAL, OutRoot);
			bRet = OutRoot.AddElem("Results");
			OutRoot.IntoElem();
            for(unsigned int i = 0; i < count ; i ++)
            {
                unsigned int tmplen;
				bRet = OutRoot.AddElem("Result");
                Result = DecodeAndWriteDataAccessResult(tmplen, lpSetRes, OutRoot);
                if(Result != 0)
                {
                    return Result;
                }
                lpSetRes += tmplen;
            }                        

            unsigned int BlockNumber;
            lpSetRes += DeCodeSizedUnsigned(&BlockNumber, 4, lpSetRes);
            if(BlockNumber != mBlockTransfer.BlockNumber)
            {
                return 546;
            }  

            mBlockTransfer.Flag = false; //就算没有发完,收到该pdu也认为服务器需要提前结束流程
            DeleteApduBuff();
            
            return -1;    
        }
        //return 0;
    }
}


int CStack62056::ProcessActionRes(CMarkup& OutRoot, const unsigned char *lpActionRes, unsigned int ActionResLen)
{
	unsigned char EK_Buf[KEY_LEN];
    //1.检查状态
    if((mAareParams.MechanismName <= LOW_LEVEL && mCosemAppState != ASSOCIATED)
    || (mAareParams.MechanismName >= HIGH_LEVEL && mCosemAppState != HLS_ASSOCIATION_PENDING && mCosemAppState != HLS_ASSOCIATED))
    {
        return 319;
    }

    //2. 检查该 连接使用的 是 LN  or  SN
    if(mAareParams.ApplicationContextName == SN_NOCIPER || mAareParams.ApplicationContextName == SN_CIPER)
    {
        return 318;
    }

    //3.检查一致性模块
    if(mAareParams.XdlmsInitiateRes.Conformance.bits.Action == 0)
    {
        return 318;
    }

    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == false)
    {
        return 330;
    }

    
    //--------------------------   
    int ResponseType = *lpActionRes++;
    if(ResponseType < ACTION_RES_NORMAL || ResponseType > ACTION_RES_NEXT_PBLOCK)
    {
        return 545;
    }
    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;
    InvokeIdAndPriority.Val = *lpActionRes++;
    ActionResLen -= 2;


    if(ResponseType == ACTION_RES_NORMAL || ResponseType == ACTION_RES_WITH_LIST)
    {
        if(mBlockTransfer.Flag == true)
        {
            mBlockTransfer.Flag = false;
            DeleteApduBuff();
        }
		OutRoot.IntoElem();
		bool bRet = OutRoot.AddElem("XML");
		bRet = OutRoot.AddElem("ACTION_CNF");
        WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, ResponseType, OutRoot);
		OutRoot.OutOfElem();
        unsigned int tmplen = ActionResLen;
        int Result;
        if(ResponseType == ACTION_RES_NORMAL)
        {
            Result = DecodeAndWriteActionResponsewithOptionalData(tmplen, lpActionRes, OutRoot);
            if(Result != 0)
            {
                return Result;
            }

            if(mCosemAppState == HLS_ASSOCIATION_PENDING)
            {
                //判断f(CtoS)
				bRet = OutRoot.FindChildElem("Result");
				if (bRet != false)
				{
					OutRoot.IntoElem();
					bRet = OutRoot.FindChildElem("Data");
					if (bRet != false)
					{
						OutRoot.IntoElem();
						bRet = OutRoot.FindChildElem("L0");
						if (bRet != false)
						{
							//取出f(CtoS)
							std::string strTag = OutRoot.GetAttrib("Type");
							if (GetTag(strTag.c_str()) == 0x09)
							{
								std::string str = OutRoot.GetAttrib("Value");
								if (str.length() != (SH_LEN + T_LEN) * 2)
								{
									return 735;
								}
								unsigned char F_CtoS[SH_LEN + T_LEN] = { 0 };
								unsigned char *lptmp = F_CtoS;
								for (unsigned int i = 1; i < (SH_LEN + T_LEN) * 2; i = i + 2)
								{
									*lptmp++ = HexToInt(str.substr(i, 2));
								}
								//f(StoC)
								if (mAarqParams.XdlmsInitiateReq.DedicatedKeyLen == 0)
									memcpy(EK_Buf, mSecurityOption.SecurityMaterial.EK, KEY_LEN);
								else
									memcpy(EK_Buf, mSecurityOption.SecurityMaterial.DK, KEY_LEN);
								memcpy(ase_gcm_128->S, &F_CtoS[0], 1);
								memcpy(ase_gcm_128->S + SC_LEN, mSecurityOption.SecurityMaterial.AK, KEY_LEN);
								memcpy(ase_gcm_128->S + KEY_LEN + SC_LEN, mAarqParams.CallingAuthenticationValue, mAarqParams.CallingAuthenticationValueLen);
								ase_gcm_128->Encrypt_StringData(F_CtoS[0], (char*)EK_Buf, (char*)mAareParams.RespondingAPTitle,
									(char*)&F_CtoS[1], (char*)mSecurityOption.SecurityMaterial.AK, mAarqParams.CallingAuthenticationValueLen, (char*)ase_gcm_128->T);

								// ase_gcm_128->AES_GCM_Encryption(TYPE_ENCRYPTION, F_CtoS[0], mAareParams.RespondingAPTitle,
								 //                                    &F_CtoS[1], mSecurityOption.SecurityMaterial.EK,
								//                                     mSecurityOption.SecurityMaterial.AK, mAarqParams.CallingAuthenticationValue, mAarqParams.CallingAuthenticationValueLen);


								 //yanshiqi OutRoot->ChildNodes->Delete(0);
								 //将mOutRoot克隆到节点下
								//yanshiqi OutRoot->ChildNodes->Add(mOutRoot->CloneNode(true));
								OutRoot.OutOfElem();
								OutRoot.ResetMainPos();
								OutRoot.FindElem();

								OutRoot.IntoElem();
								OutRoot.ResetMainPos();
								OutRoot.FindElem();

								bRet = OutRoot.FindChildElem("Result_Source_Diagnostic");
								if (memcmp(&F_CtoS[SH_LEN], ase_gcm_128->T, T_LEN) == 0)
								{
									OutRoot.IntoElem();
									OutRoot.SetAttrib("Value", "100");
									mCosemAppState = HLS_ASSOCIATED;
								}
								else
								{
									OutRoot.FindChildElem("Result");
									OutRoot.IntoElem();
									OutRoot.SetAttrib("Value", "1");
									OutRoot.SetAttrib("Value", "113");
									ResetCosemAppAssociation();
								}
							}
							else
							{
								return 735;
							}
						}
						else
						{
							return 735;
						}
					}
					else
					{
						return 735;
					}
                }
                else
                {
                    return 735;
                }
            }
        }
        else
        {
            unsigned int ResultsCount, LengthEncodeBytes;
            LengthEncodeBytes = DeCodeLength(&ResultsCount, lpActionRes);
            lpActionRes += LengthEncodeBytes;
            ActionResLen -= LengthEncodeBytes;
            tmplen = ActionResLen;
            OutRoot.AddElem("Results");
            for(unsigned int i = 0; i < ResultsCount; i ++)
            { 
                Result = DecodeAndWriteActionResponsewithOptionalData(tmplen, lpActionRes, OutRoot);
                if(Result != 0)
                {
                    return Result;
                }
                lpActionRes += tmplen;
                ActionResLen -= tmplen;
                tmplen = ActionResLen;
            }             
        }       
        //XmlToStr();

        return -1;
    }
    else
    {
        if(mBlockTransfer.Flag == false)
        {
            return 547;
        }

        if(ResponseType == ACTION_RES_NEXT_PBLOCK)
        {
            if(mBlockTransfer.Offset == mApduLen)//应该接收的是最后一块的响应
            {
                return 548;
            }
            unsigned int BlockNumber;
            lpActionRes += DeCodeSizedUnsigned(&BlockNumber, 4, lpActionRes);
            if(BlockNumber != mBlockTransfer.BlockNumber)
            {
                return 546;
            }
            unsigned char *lptmp = mApduToHdlcBuff;
            *lptmp++ = ACTION_REQUEST;
            *lptmp++ = ACTION_REQ_WITH_PBLOCK;
            InvokeIdAndPriority.bits.ServiceClass = true;
            *lptmp++ = InvokeIdAndPriority.Val;
            mBlockTransfer.BlockNumber++;

            //---------------------------------------
            //计算加密增量
            unsigned char EncryptionIncrement = 0;
            switch(mSecurityOption.SecurityPolicy)
            {
                case ALL_MESSAGES_AUTHENTICATED:EncryptionIncrement = 21; break;
                case ALL_MESSAGES_ENCRYPTED:EncryptionIncrement = 9;break;
                case ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED:EncryptionIncrement = 21;break;
                default:EncryptionIncrement = 0;break;
            }
            //-----------------------------------------
            mBlockTransfer.RawDataLen = CalcRawDataLen(mClientMaxSendPduSize - EncryptionIncrement, lptmp - mApduToHdlcBuff + 5);
            //一次发送不完,不是最后一块
            if(mApduLen - mBlockTransfer.Offset > mBlockTransfer.RawDataLen)
            {
                *lptmp++ = 0x00;//last-block
                lptmp += EnCodeSizedUnsigned(mBlockTransfer.BlockNumber, 4, lptmp);
                lptmp += EnCodeLength(mBlockTransfer.RawDataLen, lptmp);
                memcpy(lptmp, mApduBuff + mBlockTransfer.Offset, mBlockTransfer.RawDataLen);
                lptmp += mBlockTransfer.RawDataLen;
                mBlockTransfer.Offset += mBlockTransfer.RawDataLen;
                mApduToHdlcLen = mClientMaxSendPduSize - EncryptionIncrement;
            }
            else
            {
                *lptmp++ = 0xFF;//last-block
                lptmp += EnCodeSizedUnsigned(mBlockTransfer.BlockNumber, 4, lptmp);
                lptmp += EnCodeLength(mApduLen - mBlockTransfer.Offset, lptmp);
                memcpy(lptmp, mApduBuff + mBlockTransfer.Offset, mApduLen - mBlockTransfer.Offset);
                lptmp += mApduLen - mBlockTransfer.Offset;
                //mBlockTransfer.Flag = false;
                mApduToHdlcLen = lptmp - mApduToHdlcBuff;
            }

            //新增，
            DoEncryption(ACTION_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);

            //通知HDLC层发送
            if(mSupportLayerType == HDLC)
            {
                return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
            }
            else if(mSupportLayerType == UDPIP)
            {
                return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
            }
            else
            {
                return 1;
            }
        }
        else //if(ResponseType == ACTION_RES_WITH_PBLOCK)
        {
            bool LastBlock = *lpActionRes++;
            unsigned int BlockNumber;
            lpActionRes += DeCodeSizedUnsigned(&BlockNumber, 4, lpActionRes);
            unsigned int tmplen;
            unsigned int RawDataLen;
            tmplen = DeCodeLength(&RawDataLen, lpActionRes);
            //if()
            lpActionRes += tmplen;
            ActionResLen -= 5 + tmplen;
            
            if(mBlockReceive.Flag == false)//可能是第一个action分块接收
            {
                if(mBlockTransfer.Flag == true)//看看之前是否有已经发送完的分块传输
                {
                    if(mBlockTransfer.Offset < mApduLen)
                    {
                        return 551;
                    }
                    else
                    {
                        mBlockTransfer.Flag = false;
                        DeleteApduBuff();
                    }
                }

                //开始一个新的ActionRes块传输流程 
                if(BlockNumber != 1)
                {
                    return 552; 
                }   
                
                int Result = InitBlockRecvBuff();
                if(Result != 0)
                {
                    return Result;
                }

                mBlockReceive.Flag = true;
                mBlockReceive.BlockNumber = 1;
                memcpy(mBlockRecvBuff, lpActionRes, RawDataLen);
                mBlockRecvLen = RawDataLen;  
            }
            else
            {
                //是后续块  
                if(BlockNumber != mBlockReceive.BlockNumber)
                {
                    return 553; 
                }
                                
                mBlockReceive.BlockNumber++;
                if(mBlockRecvLen + RawDataLen > mBlockRecvBuffSize)
                {
                    int Result = ReallocBlockRecvBuff();
                    if(Result != 0)
                    {
                        return Result;
                    }
                }
                memcpy(mBlockRecvBuff + mBlockRecvLen, lpActionRes, RawDataLen);
                mBlockRecvLen += RawDataLen;
            }
            if(LastBlock == true)
            {
                //解析 mBlockRecvBuff 中的内容
                //---------------
                mBlockReceive.Flag = false;
            
				OutRoot.IntoElem();
                bool bRet = OutRoot.AddElem("XML");
				OutRoot.IntoElem();
                bRet = OutRoot.AddElem("ACTION_CNF");
				OutRoot.IntoElem();


                if((ACTION_REQUEST_TYPE)mLastDataRequest.RequestType == ACTION_REQ_NORMAL)
                {
                    WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, ACTION_RES_NORMAL, OutRoot);
                    //CMarkup& ResultNode = Root->AddChild("Result");
                    unsigned char *lptmp = mBlockRecvBuff;
                    //unsigned int tmplen;
                    tmplen = ActionResLen;
                    int Result = DecodeAndWriteActionResponsewithOptionalData(tmplen, lptmp, OutRoot);
                    if(Result != 0)
                    {
                        return Result;
                    }    
                    return -1;
                }
                else 
                {
                    WriteDataResHead(InvokeIdAndPriority.bits.InvokeId, InvokeIdAndPriority.bits.Priority, ACTION_RES_WITH_LIST, OutRoot);

                    unsigned char *lptmp = mBlockRecvBuff;
                    unsigned int ResultsCount;
                    lptmp += DeCodeLength(&ResultsCount, lptmp);
                    OutRoot.AddElem("Results");
                    tmplen = ActionResLen;
                    for(unsigned int i = 0; i < ResultsCount; i ++)
                    {                                
                        int Result = DecodeAndWriteActionResponsewithOptionalData(tmplen, lptmp, OutRoot);
                        if(Result != 0)
                        {
                            return Result;
                        }
                        lptmp += tmplen;
                        ActionResLen -= tmplen;
                        tmplen = ActionResLen;
                    }  
                }
                DeleteBlockRecvBuff();
                //-------------
                return -1;
            }
            else
            {
                //发送actionreqnext
                unsigned char *lptmp = mApduToHdlcBuff;
                *lptmp++ = ACTION_REQUEST;
                *lptmp++ = ACTION_REQ_NEXT_PBLOCK;
                InvokeIdAndPriority.bits.ServiceClass = true;
                *lptmp++ = InvokeIdAndPriority.Val;
                lptmp += EnCodeLength(mBlockReceive.BlockNumber, lptmp);
                mApduToHdlcLen = lptmp - mApduToHdlcBuff;

                //新增，
                DoEncryption(ACTION_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen,mSecurityOption.SecurityPolicy);
                    
                if(mSupportLayerType == HDLC)
                {
                    return DlDataReq(OutRoot, I_COMPLETE, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else if(mSupportLayerType == UDPIP)
                {
                    return UdpDataReq(OutRoot, mApduToHdlcBuff, mApduToHdlcLen);
                }
                else
                {
                    return 1;
                }
            }
        }
    }
}

int CStack62056::ProcessGloRes(CMarkup& OutRoot, unsigned char ApduType, const unsigned char *lpRes, unsigned int ResLen)
{
    unsigned int len;
    unsigned int lentmp;
    unsigned int ApduLen;
    const unsigned char *lpApdu;
    const unsigned char *lpT;
    unsigned char EK_Buf[KEY_LEN];
    unsigned char tmp_Buf[PLAINTEXT_LEN];
   
    lentmp = DeCodeLength(&len, lpRes);
    lpRes += lentmp;

    if(!(*lpRes == SC_U_A || *lpRes == SC_U_E || *lpRes == SC_U_AE || *lpRes == SC_B_A || *lpRes == SC_B_E || *lpRes == SC_B_AE))
    {
        return 719;
    }
    if(ApduType>=ded_get_response)
		memcpy(EK_Buf,mSecurityOption.SecurityMaterial.DK,KEY_LEN);
	else
		memcpy(EK_Buf,mSecurityOption.SecurityMaterial.EK,KEY_LEN);

    unsigned char SC = *lpRes++;
    unsigned char FC[FC_LEN] = {0};
    memcpy(FC, lpRes, FC_LEN);
    lpRes += FC_LEN;
    switch(SC)
    {
        case SC_U_A:
        {
            //APDU || T  
            ApduLen = len - SH_LEN - T_LEN;
            lpApdu = lpRes;
            lpRes += ApduLen;
            lpT = lpRes;

            //check T
            memcpy(ase_gcm_128->S,&SC,1);
            memcpy(ase_gcm_128->S+SC_LEN,mSecurityOption.SecurityMaterial.AK,KEY_LEN);
            memcpy(ase_gcm_128->S+KEY_LEN+SC_LEN,lpApdu,ApduLen);
            memcpy(tmp_Buf,lpApdu,ApduLen);
	     ase_gcm_128->Encrypt_StringData(SC_U_A, (char*)EK_Buf, (char*)mAareParams.RespondingAPTitle,
			 (char*)FC, (char*)mSecurityOption.SecurityMaterial.AK, ApduLen, (char*)ase_gcm_128->T);

            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_A, mAareParams.RespondingAPTitle,
             //               FC, mSecurityOption.SecurityMaterial.EK,
             //               mSecurityOption.SecurityMaterial.AK, lpApdu, ApduLen);

            if(memcmp(lpT, ase_gcm_128->T, T_LEN) != 0)
            {
                return 733;
            }
            memcpy(ase_gcm_128->S,tmp_Buf,ApduLen);
        }
        break;
        case SC_U_E:
        {
            //CIPHER
            ApduLen = len - SH_LEN;
            lpApdu = lpRes;
	     memcpy(ase_gcm_128->S,lpApdu,ApduLen);
	     ase_gcm_128->Decrypt_StringData(SC_U_E, (char*)EK_Buf, (char*)mAareParams.RespondingAPTitle,
			 (char*)FC, (char*)mSecurityOption.SecurityMaterial.AK, ApduLen, (char*)ase_gcm_128->T);

            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_E, mAareParams.RespondingAPTitle,
             //               FC, mSecurityOption.SecurityMaterial.EK,
            //                mSecurityOption.SecurityMaterial.AK, lpApdu, ApduLen);
        }
        break;
        case SC_U_AE:
        {
            //CIPHER || T   
            ApduLen = len - SH_LEN - T_LEN;
            lpApdu = lpRes;
            lpRes += ApduLen;
            lpT = lpRes; 
	     memcpy(ase_gcm_128->S,lpApdu,ApduLen);
	     ase_gcm_128->Decrypt_StringData(SC_U_AE, (char*)EK_Buf, (char*)mAareParams.RespondingAPTitle,
			 (char*)FC, (char*)mSecurityOption.SecurityMaterial.AK, ApduLen, (char*)ase_gcm_128->T);

            //ase_gcm_128->AES_GCM_Encryption(TYPE_DECRYPTION, SC_U_AE, mAareParams.RespondingAPTitle,
            //                FC, mSecurityOption.SecurityMaterial.EK,
           //                 mSecurityOption.SecurityMaterial.AK, lpApdu, ApduLen);

            if(memcmp(ase_gcm_128->T, lpT, T_LEN) != 0)
            {
                return 733;
            }
        }
        break;
        case SC_B_A:break;
        case SC_B_E:break;
        case SC_B_AE:break;
    }
/*
    unsigned long Counter = (((unsigned long)FC[0]<<24)|((unsigned long)FC[1]<<16)|((unsigned long)FC[2]<<8)|((unsigned long)FC[3]));
    Counter += 1;
    FC[0] = (unsigned char)(Counter>>24);
    FC[1] = (unsigned char)(Counter>>16);
    FC[2] = (unsigned char)(Counter>>8);
    FC[3] = (unsigned char)(Counter);
    memcpy(mSecurityOption.SecurityMaterial.IV.params.FC, FC, FC_LEN); */
	
    switch(ApduType)
    {
    case GET_RESPONSE:
	case ded_get_response:		
		return ProcessGetRes(OutRoot, ase_gcm_128->S + 1, ApduLen - 1);
    case SET_RESPONSE:
	case ded_set_response:		
		return ProcessSetRes(OutRoot, ase_gcm_128->S + 1, ApduLen - 1);
    case ACTION_RESPONSE: 
	case ded_action_response: 
		return ProcessActionRes(OutRoot, ase_gcm_128->S + 1, ApduLen - 1);
    default:
		return -1; 
    }
}


void CStack62056::XmlToStr(CMarkup& OutXml, unsigned char **OUTData, unsigned int &OUTDataLen)
{
//    if(XML == NULL)
//    {
//        SetErrMsg(OUTData, OUTLength, "")
//    }

//    OutXml->SaveToFile("file.xml");
    //DateSeparator = '-';
    //ShortDateFormat = "yyyy/mm/dd";
    //AnsiString timestr =  Now().FormatString("hhmmdd");
    /*for(int i = 0; i < timestr.Length(); i++)
    {
        if(timestr.sub
    }*/
   //OutXml->SaveToFile("file" + IntToStr(FileNo++) + ".xml");

    std::string strOut = OutXml.GetDoc();

    //str->Position = 0;
    //AnsiString data = str->DataString;
    //str->Free();

    OUTDataLen = strOut.length() + 1;
    *OUTData = new unsigned char[OUTDataLen];
    strcpy((char*)*OUTData, strOut.c_str());
};

void CStack62056::WriteAddr(CMarkup& Root)
{
	Root.IntoElem();
    Root.AddElem("Protocol_Connection_Parameters");
	Root.IntoElem();
    if(mSupportLayerType == HDLC)
    {
        //CMarkup& Node = AddrNode->AddChild("Server_MAC_Address");
        //Node->SetAttribute("Value", IntToStr(mAddr.HDLC_ADDR2.ServerAddr));
        //Node = AddrNode->AddChild("Client_MAC_Address");
        //Node->SetAttribute("Value", IntToStr(mAddr.HDLC_ADDR2.ClientAddr));
		char szBuf[64] = { 0 };
		Root.AddElem("Server_Upper_MAC_Address");
		sprintf_s(szBuf, "%d", mAddr.HDLC_ADDR1.ServerUpperAddr);
		Root.AddAttrib("Value", szBuf);

		Root.AddElem("Server_Lower_MAC_Address");
		sprintf_s(szBuf, "%d", mAddr.HDLC_ADDR1.ServerLowerAddr);
		Root.AddAttrib("Value", szBuf);

		Root.AddElem("Client_MAC_Address");
		sprintf_s(szBuf, "%d", mAddr.HDLC_ADDR1.ClientAddr);
		Root.AddAttrib("Value", szBuf);
    }
    else
    {

    }

	Root.OutOfElem();
	Root.OutOfElem();
}

void CStack62056::WriteCosemOpenCnf(CMarkup& OutRoot, DL_CONNECT_RESULT ConnResult)
{
	OutRoot.IntoElem();
    OutRoot.AddElem("XML");
	OutRoot.IntoElem();
	OutRoot.AddElem("COSEM_OPEN_CNF");
	OutRoot.IntoElem();
    //WriteAddr(CosemNode);

    OutRoot.AddElem("Result");
    
    switch(ConnResult)
    {
        case CONNECT_OK:
            OutRoot.AddAttrib("Value", "0");
			OutRoot.IntoElem();
			OutRoot.AddElem("Result_Source_Diagnostic");
			OutRoot.AddAttrib("Value", "200");
            break;
        case CONNECT_NOK_REMOTE:
			OutRoot.AddAttrib("Value", "2");
			OutRoot.IntoElem();
			OutRoot.AddElem("Result_Source_Diagnostic");
			OutRoot.AddAttrib("Value", "201");
            break;
        case CONNECT_NOK_LOCAL:
			OutRoot.AddAttrib("Value", "1");
			OutRoot.IntoElem();
			OutRoot.AddElem("Result_Source_Diagnostic");
			OutRoot.AddAttrib("Value", "101");
            break;
        case CONNECT_NO_RESPONSE:
			OutRoot.AddAttrib("Value", "2");
			OutRoot.IntoElem();
			OutRoot.AddElem("Result_Source_Diagnostic");
			OutRoot.AddAttrib("Value", "201");
            break;

    }
	OutRoot.OutOfElem();
	OutRoot.OutOfElem();
	OutRoot.OutOfElem();
	OutRoot.OutOfElem();
}

void CStack62056::WriteCosemReleaseCnf(CMarkup& OutRoot, DL_DISC_RESULT Result)
{
	OutRoot.IntoElem();
    OutRoot.AddElem("XML");
	OutRoot.IntoElem();
    OutRoot.AddElem("COSEM_RELEASE_CNF");
	OutRoot.IntoElem();
    //WriteAddr(CosemNode);

	OutRoot.AddElem("Result");
    switch(Result)
    {
        case DISC_OK:
			OutRoot.AddAttrib("Value", "0");
            break;
	    case DISC_NOK:
			OutRoot.AddAttrib("Value", "1");
			OutRoot.IntoElem();
            OutRoot.AddElem("Failure_Type");
			OutRoot.AddAttrib("Value", "1");
            break;
	    case DISC_NO_RESPONSE:
			OutRoot.AddAttrib("Value", "1");
			OutRoot.IntoElem();
			OutRoot.AddElem("Failure_Type");
			OutRoot.AddAttrib("Value", "1");
            break;
        default:break;
    }
	OutRoot.OutOfElem();
	OutRoot.OutOfElem();
	OutRoot.OutOfElem();
	OutRoot.OutOfElem();
}

void CStack62056::MakeXdlmsInitReq(unsigned char *XdlmsInitReq, unsigned int &XdlmsInitReqLen)
{
    //unsigned char XdlmsInitReq[256];
    //unsigned int  XdlmsInitReqLen = 0;
    unsigned char *lptmp = XdlmsInitReq;

    *lptmp++ = INITIATE_REQUEST;
    if(mAarqParams.XdlmsInitiateReq.DedicatedKeyLen == 0)
    {
        *lptmp++ = 0x00;
    }
    else
    {
        *lptmp++ = 0x01;
        lptmp += EnCodeLength(mAarqParams.XdlmsInitiateReq.DedicatedKeyLen, lptmp);
        memcpy(lptmp, mAarqParams.XdlmsInitiateReq.DedicatedKey, mAarqParams.XdlmsInitiateReq.DedicatedKeyLen);
        lptmp += mAarqParams.XdlmsInitiateReq.DedicatedKeyLen;
    }

    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == true)
    {
        *lptmp++ = 0x00;
    }
    else
    {
        *lptmp++ = 0x01;
        *lptmp++ = 0x00;
    }

    *lptmp++ = 0x00;
    *lptmp++ = mAarqParams.XdlmsInitiateReq.DlmsVersionNumber;
    *lptmp++ = 0x5F;   *lptmp++ = 0x1F;   *lptmp++ = 0x04;    *lptmp++ = 0x00;
    memcpy(lptmp, mAarqParams.XdlmsInitiateReq.Conformance.Val, 3); 
    lptmp += 3;

    lptmp += EnCodeSizedUnsigned(mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize, 2, lptmp);
    XdlmsInitReqLen = lptmp - XdlmsInitReq;

    //新增，如果application context name为加密，则结合security option对Initiate.req进行加密

//    DoEncryption(INITIATE_REQUEST, XdlmsInitReq, &XdlmsInitReqLen,3);
	if(mSecurityOption.SecurityPolicy == NO_SECURITY)
	{
    	DoEncryption(INITIATE_REQUEST, XdlmsInitReq, &XdlmsInitReqLen,0);
	}
	else
	{
    	DoEncryption(INITIATE_REQUEST, XdlmsInitReq, &XdlmsInitReqLen,3);
	}
}
int CStack62056::MakeAarqApdu()
{
    /*unsigned char pt_test[31] = {0x01, 0x01, 0x10, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x00, 0x06, 0x5f, 0x1f, 0x04,
    0x00, 0x00, 0x7e, 0x1f, 0x04, 0xb0};*/
    //-----------------------------------------
    unsigned char *lptmp;
    
    unsigned char XdlmsInitReq[256];
    unsigned int  XdlmsInitReqLen = 0;
    MakeXdlmsInitReq(XdlmsInitReq, XdlmsInitReqLen);
    /*unsigned char *lptmp = XdlmsInitReq;

    *lptmp++ = INITIATE_REQUEST;
    if(mAarqParams.XdlmsInitiateReq.DedicatedKeyLen == 0)
    {
        *lptmp++ = 0x00;
    }
    else
    {
        *lptmp++ = 0x01;
        lptmp += EnCodeLength(mAarqParams.XdlmsInitiateReq.DedicatedKeyLen, lptmp);
        memcpy(lptmp, mAarqParams.XdlmsInitiateReq.DedicatedKey, mAarqParams.XdlmsInitiateReq.DedicatedKeyLen);
        lptmp += mAarqParams.XdlmsInitiateReq.DedicatedKeyLen;
    }

    if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == true)
    {
        *lptmp++ = 0x00;
    }
    else
    {
        *lptmp++ = 0x01;
        *lptmp++ = 0x00;
    }

    *lptmp++ = 0x00;
    *lptmp++ = mAarqParams.XdlmsInitiateReq.DlmsVersionNumber;
    *lptmp++ = 0x5F;   *lptmp++ = 0x1F;   *lptmp++ = 0x04;    *lptmp++ = 0x00;
    memcpy(lptmp, mAarqParams.XdlmsInitiateReq.Conformance.Val, 3); 
    lptmp += 3;
    
    lptmp += EnCodeSizedUnsigned(mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize, 2, lptmp);
    XdlmsInitReqLen = lptmp - XdlmsInitReq;

    //新增，如果application context name为加密，则结合security option对Initiate.req进行加密

    DoEncryption(INITIATE_REQUEST, XdlmsInitReq, &XdlmsInitReqLen);
    */

    //-----------------------------------------------------
    unsigned char AarqApdu[256];
    unsigned int  AarqApduLen = 0;
    lptmp = AarqApdu;

    *lptmp++ = 0xA1;   *lptmp++ = 0x09;   *lptmp++ = 0x06;   *lptmp++ = 0x07;
    *lptmp++ = 0x60;   *lptmp++ = 0x85;   *lptmp++ = 0x74;   *lptmp++ = 0x05;
    *lptmp++ = 0x08;   *lptmp++ = 0x01;   *lptmp++ = mAarqParams.ApplicationContextName;

    if(mAarqParams.ApplicationContextName == LN_CIPER || mAarqParams.MechanismName > LOW_LEVEL)
    {
        *lptmp++ = 0xA6;   *lptmp++ = MAN_ID_LEN + MAN_NUM_LEN + 2;   *lptmp++ = 0x04;   *lptmp++ = MAN_ID_LEN + MAN_NUM_LEN;
        //memcpy(lptmp, mSecurityOption.SecurityMaterial.IV.params.ManId, MAN_ID_LEN);
        //lptmp += MAN_ID_LEN;
        //memcpy(lptmp, mSecurityOption.SecurityMaterial.IV.params.ManNum, MAN_NUM_LEN);
        //lptmp += MAN_NUM_LEN;
        memcpy(lptmp, mAarqParams.CallingAPTitle, AP_TITLE_LEN);
        lptmp += AP_TITLE_LEN;
    }


    if(mAarqParams.SenderAcseRequirementsPresent)
    {
        *lptmp++ = 0x8A;   *lptmp++ = 0x02;   *lptmp++ = 0x07;
        if(mAarqParams.SenderAcseRequirements)
        {
            *lptmp++ = 0x80;
        }
        else
        {
            *lptmp++ = 0x00;
        }
    }

    if(mAarqParams.SenderAcseRequirementsPresent || mAarqParams.MechanismName != LOWEST_LEVEL)
    {
        *lptmp++ = 0x8B;   *lptmp++ = 0x07;
        *lptmp++ = 0x60;   *lptmp++ = 0x85;   *lptmp++ = 0x74;   *lptmp++ = 0x05;
        *lptmp++ = 0x08;   *lptmp++ = 0x02;   *lptmp++ = mAarqParams.MechanismName;
    }

    if(mAarqParams.MechanismName != LOWEST_LEVEL)
    {
        *lptmp++ = 0xAC;   *lptmp++ = mAarqParams.CallingAuthenticationValueLen + 2;
        *lptmp++ = 0x80;   *lptmp++ = mAarqParams.CallingAuthenticationValueLen;
        memcpy(lptmp, mAarqParams.CallingAuthenticationValue, mAarqParams.CallingAuthenticationValueLen);
        lptmp += mAarqParams.CallingAuthenticationValueLen;
    }

    *lptmp++ = 0xBE;
    lptmp += EnCodeLength(XdlmsInitReqLen + 2, lptmp);
    *lptmp++ = 0x04;
    lptmp += EnCodeLength(XdlmsInitReqLen, lptmp);
    AarqApduLen = lptmp - AarqApdu;
    //-----------------------------------------------------
    int Result = InitApduBuff();
    if(Result != 0)
    {
        return Result;
    }

    lptmp = mApduBuff;
    *lptmp++ = 0x60;
    lptmp += EnCodeLength(XdlmsInitReqLen + AarqApduLen, lptmp);

    if(XdlmsInitReqLen + AarqApduLen + (lptmp - mApduBuff) > mClientMaxSendPduSize)
    {
        return 317;
    }
    
    memcpy(lptmp, AarqApdu, AarqApduLen);
    lptmp += AarqApduLen;
    memcpy(lptmp, XdlmsInitReq, XdlmsInitReqLen);
    lptmp += XdlmsInitReqLen;
    mApduLen = lptmp - mApduBuff;

    return 0;
}


int CStack62056::MakeHLSApdu()
{
    //------------------------------------
    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;
    //
    
    unsigned char *lptmp = mApduToHdlcBuff;
    *lptmp++ = ACTION_REQUEST;
    *lptmp++ = ACTION_REQ_NORMAL;
    InvokeIdAndPriority.Val = 0;
    InvokeIdAndPriority.bits.InvokeId = 0;
    InvokeIdAndPriority.bits.Priority = NORMAL;
    InvokeIdAndPriority.bits.ServiceClass = CONFIRMED;
    *lptmp++ = InvokeIdAndPriority.Val;
    
    //
    mLastDataRequest.ServiceType = ACTION_REQUEST;
    mLastDataRequest.InvokeIdAndPriority.Val = InvokeIdAndPriority.Val;
    mLastDataRequest.RequestType = ACTION_REQ_NORMAL;

    //
    lptmp += EnCodeSizedUnsigned(15, 2, lptmp);
    lptmp += EnCodeSizedOctetStr("0000280000FF", 6, lptmp);
    lptmp += EnCodeSizedInteger(1, 1, lptmp);


    //
    *lptmp++ = 0x01; //Optional -Data
    *lptmp++ = 0x09;
    lptmp += EnCodeLength(SH_LEN + T_LEN, lptmp); //EnCodeOctetStr(AnsiString((const char *)mAarqParams.F_StoC), SH_LEN + T_LEN, lptmp);
    memcpy(lptmp, mAarqParams.F_StoC, SH_LEN + T_LEN);
    lptmp += SH_LEN + T_LEN; 
    mApduToHdlcLen = lptmp - mApduToHdlcBuff;


    //// 加密处理
    DoEncryption(ACTION_REQUEST, mApduToHdlcBuff, &mApduToHdlcLen, mSecurityOption.SecurityPolicy);


    return 0;
    //--------------------------------------------------------
}



int CStack62056::MakeRlrqApdu()
{
    //-----------------------------------------------------
    unsigned char RlrqApdu[64];
    unsigned int  RlrqApduLen = 0;
    unsigned char *lptmp = RlrqApdu;

    if(mRlrqParams.ReleaseRequestReasonPresent == true)
    {
        *lptmp++ = 0x80;   *lptmp++ = 0x01;   *lptmp++ = mRlrqParams.ReleaseRequestReason;
    }

    if((mAarqParams.ApplicationContextName == LN_CIPER || mAarqParams.ApplicationContextName == SN_CIPER)&& mSecurityOption.SecurityPolicy > NO_SECURITY)
    {
        unsigned char XdlmsInitReq[256];
        unsigned int  XdlmsInitReqLen = 0;
        MakeXdlmsInitReq(XdlmsInitReq, XdlmsInitReqLen);
        

        *lptmp++ = 0xBE;
        lptmp += EnCodeLength(XdlmsInitReqLen + 2, lptmp);
        *lptmp++ = 0x04;
        lptmp += EnCodeLength(XdlmsInitReqLen, lptmp);
        memcpy(lptmp, XdlmsInitReq, XdlmsInitReqLen);
        lptmp += XdlmsInitReqLen;
    }
    RlrqApduLen = lptmp - RlrqApdu;
    
    //-----------------------------------------------------
    int Result = InitApduBuff();
    if(Result != 0)
    {
        return Result;
    }

    lptmp = mApduBuff;
    *lptmp++ = 0x62;
    lptmp += EnCodeLength(RlrqApduLen, lptmp);

    if(RlrqApduLen + (lptmp - mApduBuff) > mClientMaxSendPduSize)
    {
        return 401;
    }

    if(RlrqApduLen > 0)
    {
        memcpy(lptmp, RlrqApdu, RlrqApduLen);
        lptmp += RlrqApduLen;
    }
    mApduLen = lptmp - mApduBuff;

    return 0;
}

int CStack62056::InitApduBuff()
{
    if(mApduBuff != NULL)
    {
        delete [] mApduBuff;
    }

    if(mXmlLen >= mClientMaxSendPduSize)
    {
        mApduBuff = new unsigned char[mXmlLen + BUFF_REDUNDANCE];
    }
    else
    {
        mApduBuff = new unsigned char[mClientMaxSendPduSize + BUFF_REDUNDANCE];
    }
    if(mApduBuff == NULL)
    {
        return 379;
    }

    mApduLen = 0;

    return 0;
}

void CStack62056::DeleteApduBuff()
{
    if(mApduBuff != NULL)
    {
        delete [] mApduBuff;
        mApduBuff = NULL;
    }
    mApduLen = 0;
}

int CStack62056::InitBlockRecvBuff()
{
    if(mBlockRecvBuff != NULL)
    {
        delete [] mBlockRecvBuff;
    } 

    mBlockRecvBuff = new unsigned char[DEFAULT_CLIENT_MAX_RECEIVED_PDU_SIZE];
    if(mBlockRecvBuff == NULL)
    {
        return 379;
    }

    mBlockRecvLen = 0;
    mBlockRecvBuffSize = DEFAULT_CLIENT_MAX_RECEIVED_PDU_SIZE;
    return 0;
}

void CStack62056::DeleteBlockRecvBuff()
{
    if(mBlockRecvBuff != NULL)
    {
        delete [] mBlockRecvBuff;
        mBlockRecvBuff = NULL;
    }
    mBlockRecvLen = 0;
    mBlockRecvBuffSize = 0;
}

int CStack62056::ReallocBlockRecvBuff()
{
    unsigned char *lptmp = new unsigned char[mBlockRecvBuffSize * 2];
    if(lptmp == NULL)
    {
        return 379;
    }

    memcpy(lptmp, mBlockRecvBuff, mBlockRecvLen);

    delete [] mBlockRecvBuff;
    mBlockRecvBuff = lptmp;
    mBlockRecvBuffSize *= 2;
    return 0;
}
//------------------------COSEMApp end-------------------------------//
//------------------------HDLC-------------------------------//
//HDLC层提供给COSEMApp层调用的服务函数
int CStack62056::DlConnectReq(CMarkup& OutRoot, unsigned char *Infor, unsigned int InforLen)
{
    if(mMacState != NDM)
    {
        //考虑到COSEMApp层的重复AA连接处理，此处做特殊处理
        ResetHdlcConnection();
    }


    if(mAddr.HDLC_ADDR1.ServerUpperAddr == ALL_STATION_ADDRESS || mAddr.HDLC_ADDR1.ServerLowerAddr == ALL_STATION_ADDRESS)
    {
        MakeSNRM(OutRoot, false);

        mMacState = NRM;
    }
    else
    {
        MakeSNRM(OutRoot, true);

        mMacState = WAIT_CONNECTION;
    }

    return 0;
}

int CStack62056::DlDisconnectReq(CMarkup& OutRoot, unsigned char *Infor, unsigned int InforLen)
{
    if(mMacState == NDM)
    {
        //暂不作处理，依然发送DISC帧
    }
    
    if(mAddr.HDLC_ADDR1.ServerUpperAddr == ALL_STATION_ADDRESS || mAddr.HDLC_ADDR1.ServerLowerAddr == ALL_STATION_ADDRESS)
    {
        MakeDISC(OutRoot, false);

        ResetHdlcConnection();
    }
    else
    {
        MakeDISC(OutRoot, true);

        if(mMacState != NDM)
        {
            mMacState = WAIT_DISCONNECTION;
        }
    }
    return 0;
}

int CStack62056::DlDataReq(CMarkup& OutRoot, FRAME_TYPE FrameType, unsigned char *lpPdu, unsigned int PduLen)//数据
{
    //客户端仅允许出现这两种类型，至于客户端的分帧流程，协议中不支持，代码暂不支持
    //故而此处涉及到需要协调 最大发送pdu大小 和 最大信息域长度  之间的关系，必须满足 最大发送pdu大小 <= 最大信息域长度
    if(FrameType == UI)
    {
        if((PduLen + 3) > (unsigned int)HdlcParamsNegotiated.MaximumInformationFieldLengthTransmit)
        {
            return 196;
        }
        MakeUI(OutRoot, false, lpPdu, PduLen);
    }
    else //if(FrameType == I_COMPLETE)
    {
        //if(mAddr.HDLC_ADDR1.ServerUpperAddr == 0x3FFF || mAddr.HDLC_ADDR1.ServerLowerAddr == 0x3FFF)
        //{
        //    return 701;
        //}

        if(mMacState != NRM)
        {
            return 197;
        }
        if(mPermission == false)
        {
            return 198;
        }

        if((PduLen + 3) > (unsigned int)HdlcParamsNegotiated.MaximumInformationFieldLengthTransmit)
        {
            #ifdef CLIENT_LONG_TRANSFER_SUPPORT
            LongTransferSend.Flag = true;
            LongTransferSend.Buff = lpPdu;
            LongTransferSend.Len = PduLen;
            LongTransferSend.Offset = 0;
            
            MakeLongTransferSend(OutRoot, true);
            return 0;
            #else
            return 196;
            #endif
        }
        else
        {
            MakeI(OutRoot, false, true, true, lpPdu, PduLen);
        }
    }
    return 0;
}
//-----------------------------------------------------------------------
#ifdef CLIENT_LONG_TRANSFER_SUPPORT
void CStack62056::MakeLongTransferSend(CMarkup& OutRoot, bool First)
{
    bool SegmentBit, PorFBit, LlcHead, Flag;
    unsigned short MaxLen, ActualLen;     
    for(unsigned char i = 0; i < HdlcParamsNegotiated.WindowSizeTransmit; i ++)
    {
        if(i == 0 && First == true)
        {
            LlcHead = true;
            MaxLen = HdlcParamsNegotiated.MaximumInformationFieldLengthTransmit - 3;
        }
        else
        {
            LlcHead = false;
            MaxLen = HdlcParamsNegotiated.MaximumInformationFieldLengthTransmit;
        }

        if(LongTransferSend.Len - LongTransferSend.Offset > MaxLen)
        {
            SegmentBit = true;
            if(i == HdlcParamsNegotiated.WindowSizeTransmit - 1)
            {
                PorFBit = true;
                Flag = false;
            }
            else
            {
                PorFBit = false;
                Flag = true;
            }
            ActualLen = MaxLen;
        }
        else
        {
            SegmentBit = false;
            PorFBit = true;
            Flag = false;
            LongTransferSend.Flag = false;
            ActualLen = LongTransferSend.Len - LongTransferSend.Offset; 
        }

        MakeI(OutRoot, SegmentBit, PorFBit, LlcHead, LongTransferSend.Buff + LongTransferSend.Offset, ActualLen);

        
        if(LongTransferSend.Flag == true)
        {
            LongTransferSend.Offset += ActualLen;
        }
        else
        {
            LongTransferSend.Buff = NULL;
            LongTransferSend.Len = 0;
            LongTransferSend.Offset = 0;
        }

        if(Flag == false)
        {
            break;
        }
    }
}
#endif

int CStack62056::ProcessI(CMarkup& OutRoot, bool Segment, unsigned char RecvNo, unsigned char SendNo,
                    bool PorF, const unsigned char *Infor, unsigned int InforLen)
{
    if(mMacState != NRM)
    {
        return 210;
    }
    
    if(false == CheckSequenceNumber(RecvNo, SendNo))
    {
        return 202;//断开重连
    }

    mPermission = true;

    RetringMode = false;
    RetryTimes = 0;

    //单帧  或者  分帧流程的第一个帧
    if(false == LongTransferReceive.Flag)
    {
        if(InforLen <= 3 || *Infor != 0xE6 || *(Infor + 1) != 0xE7 || *(Infor + 2) != 0x00)
        {
            return 203;
        }
    }
    
    if(true == Segment && false == LongTransferReceive.Flag) //开始一个分帧流程
    {
        if(LongTransferReceive.Buff == NULL)
        {
            if(mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize != 0)
            {
                LongTransferReceive.Buff = new unsigned char[mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize];
            }
            else
            {
                LongTransferReceive.Buff = new unsigned char[MAX_CLIENT_RECEIVED_PDU_SIZE];   
            }
            if(LongTransferReceive.Buff == NULL)
            {
                return 204;
            }
        }
        //LongTransferReceive.Len = 0;

        
        memcpy(LongTransferReceive.Buff, Infor + 3, InforLen - 3);
        LongTransferReceive.Len = (InforLen - 3);

        LongTransferReceive.Flag = true;
    }
    else if(true == Segment && true == LongTransferReceive.Flag)  //分帧流程中的后续帧
    {
        if(mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize != 0)
        {
            if((LongTransferReceive.Len + InforLen) > (unsigned int)mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize)
            {
                return 205;
            }
        }
        else
        {
            if((LongTransferReceive.Len + InforLen) > MAX_CLIENT_RECEIVED_PDU_SIZE)
            {
                return 205;
            }    
        }
        memcpy(LongTransferReceive.Buff + LongTransferReceive.Len, Infor, InforLen);
        LongTransferReceive.Len += InforLen;
    }
    else if(false == Segment && true == LongTransferReceive.Flag) //分帧流程中的最后一帧
    {
        if(PorF == true)
        {
            if(mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize != 0)
            {
                if((LongTransferReceive.Len + InforLen) > (unsigned int)mAarqParams.XdlmsInitiateReq.ClientMaxReceivedPduSize)
                {
                    return 205;
                }
            }
            else
            {
                if((LongTransferReceive.Len + InforLen) > MAX_CLIENT_RECEIVED_PDU_SIZE)
                {
                    return 205;
                }    
            }
            memcpy(LongTransferReceive.Buff + LongTransferReceive.Len, Infor, InforLen);
            LongTransferReceive.Len += InforLen;

            LongTransferReceive.Flag = false;  //此处不释放,待连接断开时再释放

            return DlDataInd(OutRoot, I_COMPLETE, LongTransferReceive.Buff, LongTransferReceive.Len);
        }
        else
        {
            //可能是一个单独的帧,应该判断为错误,因为只能夹着UI帧,不会夹I帧
            return 211;
        }
    }
    else//if(false == Segment && false == LongTransfer_Recv)
    {
        return DlDataInd(OutRoot, I_COMPLETE, Infor + 3, InforLen - 3);
    }


    if(Segment && PorF)
    {
        MakeRR(OutRoot);  //发送RR帧
    }

    return 0;
}

int CStack62056::ProcessUI(CMarkup& OutRoot, bool PorF, const unsigned char *Infor, unsigned int InforLen)
{
    if(InforLen <= 3)
    {
        return 203;
    }

    if(*Infor != 0xE6 || *(Infor + 1) != 0xE7 || *(Infor + 2) != 0x00)
    {
        return 203;
    }
    
    return DlDataInd(OutRoot, UI, Infor + 3, InforLen - 3);
}

int CStack62056::ProcessRR(CMarkup& OutRoot, unsigned char RecvNo)
{
	if(NRM != mMacState)
    {
        return 206;
    }

    //错误！！如果是在重发过程中，那么两者不可能相等
    //mSendNo == RecvNo，说明表计收到了主台发的I帧，如果表计已经响应，帧是在表-->主台的路上丢失的，那么主台的RR不会与表的SS相等
    //故，发探测帧的时候，表就能察觉到主台没有收到自己的响应，就会发上次的响应帧，而不会发RR帧了
//    if(mSendNo == RecvNo)
//    {
//        if(RetringMode == false)  //正常情形下接收到正确RR帧
//        {
//            #ifdef CLIENT_LONG_TRANSFER_SUPPORT
//            if(true == LongTransferSend.Flag)
//            {
//                MakeLongTransferSend(OutRoot, false);
//            }
//            else
//            {
//                //丢弃或者正确序号处理
//                ///
//            }
//            return 0;
//            #else
//            //丢弃或者正确序号处理
//            ///
//            return 0;
//            #endif
//        }
//        else  //重发情形下接收到RR帧--只有当原始帧是I帧是才会形成此循环
//        //（如果原始帧是RR帧，即在长帧传输中主台发的RR帧，表没有响应，再发RR时，表计会回复I帧
//        {
//            //说明之前的I帧，表计已经正常响应，但在表-->主台的路上丢失了，怎么办？？
//            RetringMode = false;
//            RetryTimes = 0;
//        }   
//    }
//    else
//    {
//        //处理错误序号情况
//        //...
//        return 202;
//    }

    if(RetringMode == false)  //正常情形下接到的RR帧
    {
        if(mSendNo == RecvNo)
        {
            RetringMode = false;
            RetryTimes = 0;

            #ifdef CLIENT_LONG_TRANSFER_SUPPORT
            if(true == LongTransferSend.Flag)
            {
                MakeLongTransferSend(OutRoot, false);
            }
            else
            {
                //丢弃或者正确序号处理
                ///
            }
            return 0;
            #else
            //丢弃或者正确序号处理
            ///
            return 0;
            #endif

        }
        else
        {
            //处理错误序号情况
            //...
            return 202;
        }
    }
    else
    {
        if(mSendNo == RecvNo) //说明表已经接收到了主台发的I帧，但是未作响应
        //继续等待？？
        {
            RetryTimes++;
            return 0;   
        }
        else
        //该帧是测试RR的响应帧  
        //根据帧序号判断之前没有被回应的帧，终端是否收到
        //if((mSendNo == 0 && RecvNo == 7) || mSendNo > RecvNo)//说明终端没收到
        {
            RetryMode = SEND_REAL_FRAME;
            RetryTimes = 0;
            //重新发送I帧
            MakeLastI(OutRoot);
            return 0;
        }  
    }  
}

int CStack62056::ProcessDM(CMarkup& OutRoot, const unsigned char *Infor, unsigned int InforLen)
{
    if(mMacState == WAIT_DISCONNECTION) //正常断开
    {
        ResetHdlcConnection();
        return DlDisconnectCnf(OutRoot, DISC_OK, Infor, InforLen);
    }
    else if(mMacState == WAIT_CONNECTION) //正常断开
    {
        ResetHdlcConnection();
        return DlConnectCnf(OutRoot, CONNECT_NOK_REMOTE, Infor, InforLen);
    }
    else
    {
        return 207;
    }
}

int CStack62056::ProcessUA(CMarkup& OutRoot, const unsigned char *Infor, unsigned int InforLen)
{
    if(mMacState == WAIT_CONNECTION)
    {
        //获取negotiated HDLC 参数
        if(false == GetHdlcParams(Infor, InforLen))
        {
            //编码有误
            //return 208;
            //不做其他处理,认为依然建立成功
        }
        
        mMacState = NRM;      
        return DlConnectCnf(OutRoot, CONNECT_OK, Infor, InforLen);
    } 
    else if(mMacState == WAIT_DISCONNECTION) //正常断开
    {
        //断开hdlc层连接         
        ResetHdlcConnection();
        return DlDisconnectCnf(OutRoot, DISC_OK, Infor, InforLen);
    }
    else
    {
        return 201;
    }
}
/*************************************************
  Function:       CheckSequenceNumber
  Description:    判断I帧中的接收、发送序列号是否正确，
  					接收帧中的接收序列号应等于链接的发送序列号，同样接收帧中的发送序列号应等于链接的接收序列号
				  如果序列号正确，将链接的接收序列号增一
  Calls:          
  Called By:      ProcessI
  Input:          接收序列号和发送序列号
  Output:         无
  Return:         =TRUE 正确，=FALSE 不正确
  Others:         
*************************************************/
bool CStack62056::CheckSequenceNumber(unsigned char RecvNo, unsigned char SendNo)
{
	if(SendNo == mRecvNo && RecvNo == mSendNo)
	{
		if(mRecvNo != 0x07)
		{
			mRecvNo++;	
		}
		else
		{
			mRecvNo = 0;
		}
		return true;
	}
	else
	{
		return false;
	}
}

/*************************************************
  Function:       GetHdlcParams
  Description:    从SNRM帧的信息域获取HDLC参数,若某个参数未出现,使用协议默认参数
  Calls:          无
  Called By:      ProcessReceivedFrame
  
  Input:          信息域起始地址,信息域长度
  Output:         HDLC参数
  Return:         TRUE--解析成功,FALSE--解析失败
  Others:         // 其它说明
*************************************************/
bool CStack62056::GetHdlcParams(const unsigned char *Infor, unsigned int InforLen)
{
	unsigned char WS_T, WS_R;
    unsigned short MIFL_T, MIFL_R;
	unsigned char Length;
	const unsigned char *lpTmp;

	if(InforLen >= 3 && *Infor == 0x81 && *(Infor + 1) == 0x80)//at least 3 bytes
	{
		Length = *(Infor + 2);
		
		
		if(Length == 0 || Length == 3 || Length == 6 || Length == 7 || Length == 12 || Length == 15 || Length == 18
			|| Length == 4 || Length == 8 || Length == 10 || Length == 14 || Length == 16 || Length == 20)
		{    
            WS_T = DEFAULT_WINDOWSIZE_TRANSMIT;
            WS_R = DEFAULT_WINDOWSIZE_RECEIVE;
            MIFL_T = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT;
            MIFL_R = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_RECEIVE;

			lpTmp = Infor + 3;
			while(Length > 0)
			{
				switch(*lpTmp++)
				{
					case 0x05:
						if(*lpTmp == 0x01)
						{
							lpTmp++;
							MIFL_R = *lpTmp++;
							Length -= 3;
						}
						else if(*lpTmp == 0x02)
						{
							lpTmp++;
							//MIFL_R = ((*lpTmp++) << 8) + (*lpTmp++);
                            MIFL_R = *lpTmp;
                            lpTmp++;
                            MIFL_R = (MIFL_R << 8) + *lpTmp;
                            lpTmp++;
							Length -= 4;
						}
						else
						{
							return false;
						}
						break;
					case 0x06:
						if(*lpTmp == 0x01)
						{
							lpTmp++;
							MIFL_T = *lpTmp++;
							Length -= 3;
						}
						else if(*lpTmp == 0x02)
						{
							lpTmp++;
							//MIFL_T = ((*lpTmp++) << 8) + (*lpTmp++);
                            MIFL_T = *lpTmp;
                            lpTmp++;
                            MIFL_T = (MIFL_T << 8) + *lpTmp;
                            lpTmp++;
							Length -= 4;
						}
						else
						{
							return false;
						}
						break;
					case 0x07:
						if(*lpTmp == 0x04)
						{
							lpTmp += 4;
							WS_R = (*lpTmp++);
							Length -= 6;
						}						
						else
						{
							return false;
						}
						break;
					case 0x08:
						if(*lpTmp == 0x04)
						{
							lpTmp += 4;
							WS_T = (*lpTmp++);
							Length -= 6;
						}						
						else
						{
							return false;
						}
						break;
					default:
						return false;
				}//switch
			}//while

			if(WS_T == 0 || WS_R == 0 || MIFL_T <= 64 || MIFL_R <= 64)
			{
				return false;
			}

		}//if
		else
		{
			return false;
		}
	}
	else
	{
        WS_T = DEFAULT_WINDOWSIZE_TRANSMIT;
        WS_R = DEFAULT_WINDOWSIZE_RECEIVE;
        MIFL_T = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT;
        MIFL_R = DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_RECEIVE;
	}

	HdlcParamsNegotiated.WindowSizeTransmit = HdlcParamsDefault.WindowSizeTransmit <= WS_T ? HdlcParamsDefault.WindowSizeTransmit : WS_T;
	HdlcParamsNegotiated.WindowSizeReceive = HdlcParamsDefault.WindowSizeReceive <= WS_R ? HdlcParamsDefault.WindowSizeReceive : WS_R;
	HdlcParamsNegotiated.MaximumInformationFieldLengthTransmit = HdlcParamsDefault.MaximumInformationFieldLengthTransmit <= MIFL_T ? HdlcParamsDefault.MaximumInformationFieldLengthTransmit : MIFL_T;
	HdlcParamsNegotiated.MaximumInformationFieldLengthReceive = HdlcParamsDefault.MaximumInformationFieldLengthReceive <= MIFL_R ? HdlcParamsDefault.MaximumInformationFieldLengthReceive : MIFL_R;

	return true;
}

//------------------------------------------------------------

/*************************************************
  Function:       MakeFrame
  Description:    构造发送帧

  Calls:          
  Called By:      SendRR, SendI, SendUI, Send_*..
  Input:          服务器地址,客户端地址,分段位,控制域,信息域起始地址和长度
  Output:         无
  Return:         无
  Others:         
*************************************************/
void CStack62056::MakeFrame(CMarkup& OutRoot, bool Segment, unsigned char Ctrl)
{   
    unsigned char *lptmp = mOutFrame;  

    unsigned short FrameLen;
    if(mOutFrameLen > 0)
    {
        FrameLen = mOutFrameLen + 8 + mAddr.HDLC_ADDR1.ServerAddrLen;
    }
    else
    {
        FrameLen = 6 + mAddr.HDLC_ADDR1.ServerAddrLen;
    }

    *lptmp++ = 0x7E;
    
    if(FrameLen <= 0xFF)
    {
        *lptmp++ = 0xA0 + (Segment << 3);
        *lptmp++ = (unsigned char)FrameLen;
    }
    else
    {
        *lptmp++ = 0xA0 + (Segment << 3) + (FrameLen >> 8);
        *lptmp++ = (FrameLen & 0x00FF);
    }

    switch (mAddr.HDLC_ADDR1.ServerAddrLen)
    {
        case 1:
        {
            *lptmp++ = ((mAddr.HDLC_ADDR1.ServerUpperAddr << 1) & 0xFE) + 0x01;
        }break;
        case 2:
        {
            *lptmp++ = (mAddr.HDLC_ADDR1.ServerUpperAddr << 1) & 0xFE;
            *lptmp++ = ((mAddr.HDLC_ADDR1.ServerLowerAddr << 1) & 0xFE) + 0x01;
        }break;
        case 4:
        {
            *lptmp++ = (mAddr.HDLC_ADDR1.ServerUpperAddr >> 6) & 0xFE;
            *lptmp++ = (mAddr.HDLC_ADDR1.ServerUpperAddr << 1) & 0xFE;
            *lptmp++ = (mAddr.HDLC_ADDR1.ServerLowerAddr >> 6) & 0xFE;
            *lptmp++ = ((mAddr.HDLC_ADDR1.ServerLowerAddr << 1) & 0xFE) + 0x01;
        }break;
    }
    *lptmp++ = (mAddr.HDLC_ADDR2.ClientAddr << 1) + 0x01;

    *lptmp++ = Ctrl;

    unsigned short HCS = pppfcs16(0xffff, mOutFrame + 1, 4 + mAddr.HDLC_ADDR1.ServerAddrLen);
    *lptmp++ = (HCS >> 8) & 0xFF;
    *lptmp++ = HCS & 0xFF;



    if(mOutFrameLen > 0)
    {
        //memcpy(mOutDataBuff + sizeof(FRAME_SEND_HEAD), Infor, InforLen);
        lptmp += mOutFrameLen;
        unsigned short FCS = pppfcs16(0xffff, mOutFrame + 1, FrameLen - 2);
        *lptmp++ = (FCS >> 8) & 0xFF;
        *lptmp++ = FCS & 0xFF;
    }

    *lptmp++ = 0x7E;
    mOutFrameLen = FrameLen + 2;
   
    //写入XML文档
    WriteFrameToXml(OutRoot, mOutFrame, mOutFrameLen);
}


void CStack62056::WriteFrameToXml(CMarkup& OutRoot, unsigned char *lpFrame, unsigned int FrameLen)
{
    std::string FrameStr = ToDisplayChar(lpFrame, FrameLen);
	OutRoot.IntoElem();
    OutRoot.AddElem("Frame");
    OutRoot.AddAttrib("Value", FrameStr);
	OutRoot.OutOfElem();
}

void CStack62056::MakeSNRM(CMarkup& OutRoot, bool PorF)
{
    mOutFrameLen = MakeHdlcParamsField(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen);

    Ctrl_Format Ctrl;
	Ctrl.Val = SNRM_FRAME;
	Ctrl.bits.PorF = PorF;

	MakeFrame(OutRoot, false, Ctrl.Val);

    mPermission = !PorF;
}

void CStack62056::MakeDISC(CMarkup& OutRoot, bool PorF)
{
    Ctrl_Format Ctrl;
	Ctrl.Val = DISC_FRAME;
	Ctrl.bits.PorF = PorF;
    mOutFrameLen = 0;
	MakeFrame(OutRoot, false, Ctrl.Val);
    
    mPermission = !PorF;
}
//
void CStack62056::MakeUI(CMarkup& OutRoot, bool PorF, const unsigned char *Infor, unsigned int InforLen)
{
	Ctrl_Format Ctrl;
	Ctrl.Val = UI_FRAME;
	Ctrl.bits.PorF = PorF;

    *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen) = 0xE6;
    *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + 1) = 0xE6;
    *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + 2) = 0x00;

    memcpy(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + 3, Infor, InforLen);
    mOutFrameLen = InforLen + 3;
    
	MakeFrame(OutRoot, false, Ctrl.Val);

    mPermission = !PorF;
}

void CStack62056::MakeRR(CMarkup& OutRoot)
{
	Ctrl_Format Ctrl;
	Ctrl.Val = RR_FRAME;
 	Ctrl.bits.RRR = mRecvNo;
    Ctrl.bits.PorF = true;
    mOutFrameLen = 0;
	MakeFrame(OutRoot, false, Ctrl.Val);

    mPermission = !Ctrl.bits.PorF;
}


//客户端不支持分帧,故Segment始终为false, 而正常传输中的I帧PorF位始终为true
//此处预留这两个位,始终使用默认值
void CStack62056::MakeI(CMarkup& OutRoot, bool Segment, bool PorF, bool LlcHead, const unsigned char *Infor, unsigned int InforLen)
{
	Ctrl_Format Ctrl;
	Ctrl.Val = I_FRAME;
 	Ctrl.bits.RRR = mRecvNo;
	Ctrl.bits.SSS = mSendNo;
	Ctrl.bits.PorF = PorF;

    if(LlcHead == true)
    {
        *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen) = 0xE6;
        *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + 1) = 0xE6;
        *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + 2) = 0x00;
        mOutFrameLen = 3;
    }
    else
    {
        mOutFrameLen = 0;
    }

    if(InforLen > 0)
    {
        memcpy(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + mOutFrameLen, Infor, InforLen);
        mOutFrameLen += InforLen;
    }
    
	MakeFrame(OutRoot, Segment, Ctrl.Val);

    //
    if(LastIFrame.Infor != NULL)
    {
        delete [] LastIFrame.Infor;
    }
    LastIFrame.Infor = new unsigned char[InforLen];
    memcpy(LastIFrame.Infor, Infor, InforLen);
    LastIFrame.InforLen = InforLen;
    LastIFrame.RecvNo = mRecvNo;
    LastIFrame.SendNo = mSendNo;
    LastIFrame.Segment = Segment;
    LastIFrame.PorF = PorF;
    //
    
    if(mSendNo != 0x07)
    {
        mSendNo++;
    }
    else
    {
       mSendNo = 0;
    }

    mPermission = !Ctrl.bits.PorF;
}

void CStack62056::MakeLastI(CMarkup& OutRoot)
{
    Ctrl_Format Ctrl;
	Ctrl.Val = I_FRAME;
 	Ctrl.bits.RRR = LastIFrame.RecvNo;
	Ctrl.bits.SSS = LastIFrame.SendNo;
	Ctrl.bits.PorF = LastIFrame.PorF;

    *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen) = 0xE6;
    *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + 1) = 0xE6;
    *(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + 2) = 0x00;

    memcpy(mOutFrame + INFOR_OFFSET1 + mAddr.HDLC_ADDR1.ServerAddrLen + 3, LastIFrame.Infor, LastIFrame.InforLen);
    mOutFrameLen = LastIFrame.InforLen + 3;

	MakeFrame(OutRoot, LastIFrame.Segment, Ctrl.Val);

    mPermission = !Ctrl.bits.PorF;
}
unsigned char CStack62056::MakeHdlcParamsField(unsigned char *Dest)
{
    unsigned char HdlcParams[23] = {0x81, 0x80, 0x14, 0x05, 0x02, 0x00, 0x00,
                                                      0x06, 0x02, 0x00, 0x00,
                                                      0x07, 0x04, 0x00, 0x00, 0x00, 0x00,
                                                      0x08, 0x04, 0x00, 0x00, 0x00, 0x00};

    HdlcParams[5] = (HdlcParamsDefault.MaximumInformationFieldLengthTransmit >> 8) & 0xFF;
    HdlcParams[6] = HdlcParamsDefault.MaximumInformationFieldLengthTransmit & 0xFF;
    HdlcParams[9] = (HdlcParamsDefault.MaximumInformationFieldLengthReceive >> 8) & 0xFF;
    HdlcParams[10] = HdlcParamsDefault.MaximumInformationFieldLengthReceive & 0xFF;
    HdlcParams[16] = HdlcParamsDefault.WindowSizeTransmit;
    HdlcParams[22] = HdlcParamsDefault.WindowSizeReceive;
    
    memcpy(Dest, HdlcParams, 23 * sizeof(unsigned char));
    
    return 23;
}

//------------------------HDLC end-------------------------------//
//------------------------UDP-------------------------------//
int CStack62056::UdpDataReq(CMarkup& OutRoot, unsigned char *lpPdu, unsigned int PduLen)//数据
{
    AddUdpWrapper(OutRoot, lpPdu, PduLen); 
    return 0;
}
/*************************************************
  Function:       MakeFrame
  Description:    构造发送帧

  Calls:          
  Called By:      SendRR, SendI, SendUI, Send_*..
  Input:          服务器地址,客户端地址,分段位,控制域,信息域起始地址和长度
  Output:         无
  Return:         无
  Others:         
*************************************************/
void CStack62056::AddUdpWrapper(CMarkup& OutRoot, unsigned char *lpPdu, unsigned int PduLen)
{   
    unsigned char *lptmp = mOutFrame;

    //memcpy(lptmp, mAddr.UDPIP_ADDR.ID, sizeof(unsigned char) * ID_LEN);
    //lptmp += ID_LEN;
    
    *lptmp++ = 0x00;  *lptmp++ = 0x01;
    *lptmp++ = ((mAddr.UDPIP_ADDR.Client_wPort >> 8) & 0x00FF);
    *lptmp++ = (mAddr.UDPIP_ADDR.Client_wPort & 0x00FF);
    *lptmp++ = ((mAddr.UDPIP_ADDR.Server_wPort >> 8) & 0x00FF);
    *lptmp++ = (mAddr.UDPIP_ADDR.Server_wPort & 0x00FF);
    *lptmp++ = ((PduLen >> 8) & 0x00FF);
    *lptmp++ = (PduLen & 0x00FF);

    memcpy(lptmp, lpPdu, PduLen);
    mOutFrameLen = /*ID_LEN + */8 + PduLen;
    
    //写入XML文档
    WriteFrameToXml(OutRoot, mOutFrame, mOutFrameLen);    //modified by slx 2012-9-14
}
//------------------------UDP end-------------------------------//
//作为62056apdu的解码,在基本解码外再封装一层
int CStack62056::DecodeAndWriteDataAccessResult(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent)
{
	Parent.IntoElem();
    const unsigned char *lptmp = lpSrc;
    Parent.AddElem("Data_Access_Result");
    Parent.AddAttrib("Value", *lptmp++);
    Len = lptmp - lpSrc;
	Parent.OutOfElem();
    return 0;
}

//int CStack62056::DecodeAndWriteGetDataResult(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent)
//{
//    const unsigned char *lptmp = lpSrc;
//    CMarkup& Root = Parent->AddChild("Get_Data_Result");
//    int Choice = *lptmp++;
//    if(Choice == 0x00)
//    {
//        //Data
//
//    }
//    else if(Choice == 0x01)
//    {
//        unsigned int tmplen;
//        //Data-Access-Result
//        int Result = DecodeAndWriteDataAccessResult(tmplen, lptmp, Root);
//        if(Result != 0)
//        {
//            return Result;
//        }
//        lptmp += tmplen;
//        Len = lptmp - lpSrc;
//    }
//    else
//    {
//        return 600;
//    }
//    return 0;
//}

int CStack62056::DecodeAndWriteActionResult(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent)
{
    const unsigned char *lptmp = lpSrc;
	Parent.IntoElem();
    Parent.AddElem("Action_Result");
	char szBuf[64] = { 0 };
	sprintf_s(szBuf, "%d", *lptmp++);
    Parent.AddAttrib("Value", szBuf);
    Len = lptmp - lpSrc;
	Parent.OutOfElem();
    return 0;
}
int CStack62056::DecodeAndWriteActionResponsewithOptionalData(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent)
{
    unsigned int TotalLen = Len;
    unsigned int tmplen;
    const unsigned char *lptmp = lpSrc;
	Parent.IntoElem();
    Parent.AddElem("Result");
    int Result = DecodeAndWriteActionResult(tmplen, lptmp, Parent);
    if(Result != 0)
    {
        return Result;
    }
    lptmp += tmplen;
    TotalLen -= tmplen;
    
    if(*lptmp++ == 0x00)
    {
        return -1;
    }

    tmplen = TotalLen;
    Result = DecodeAndWriteGetDataResult(tmplen, lptmp, Parent);
    if(Result != 0)
    {
        return Result;
    }
    lptmp += tmplen;
    TotalLen -= tmplen;

    Len = lptmp - lpSrc;
	Parent.OutOfElem();
    return 0;
}

int CStack62056::DecodeAndWriteVariableAccessSpecification(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent)
{
    const unsigned char *lptmp = lpSrc;
	Parent.IntoElem();
    Parent.AddElem("Variable_Access_Specification");
	Parent.IntoElem();
    unsigned char Choice = *lptmp++;
    unsigned int valuetmp;
	char szBuf[64] = { 0 };

    switch(Choice)
    {
        case 0x02:
        {
            Parent.AddElem("variable-name");
            lptmp += DeCodeSizedUnsigned(&valuetmp, 2, lptmp);
			sprintf_s(szBuf, "%d", valuetmp);
            Parent.AddAttrib("Value", szBuf);
        }
        break;
        case 0x03:
        {
            Parent.AddElem("detailed-access");
			Parent.IntoElem();

			Parent.AddElem("variable-name");
            lptmp += DeCodeSizedUnsigned(&valuetmp, 2, lptmp);
			sprintf_s(szBuf, "%d", valuetmp);
			Parent.AddAttrib("Value", szBuf);

            std::string strtmp;
			Parent.AddElem("detailed-access");
            lptmp += DeCodeOctetStr(strtmp, lptmp);
			Parent.AddAttrib("Value", strtmp);
        }
        break;
        case 0x04:
        {
			Parent.AddElem("parameter");
			Parent.IntoElem();

			Parent.AddElem("variable-name");
            lptmp += DeCodeSizedUnsigned(&valuetmp, 2, lptmp);
			sprintf_s(szBuf, "%d", valuetmp);
			Parent.AddAttrib("Value", szBuf);

			Parent.AddElem("selector");
            lptmp += DeCodeSizedUnsigned(&valuetmp, 1, lptmp);
			sprintf_s(szBuf, "%d", valuetmp);
			Parent.AddAttrib("Value", szBuf);

            unsigned int lentmp;
            lptmp += DecodeAndWriteData(lentmp, lptmp, Parent, "parameter");
        }
        break;
        default:return 419;
    }
    Len = lptmp - lpSrc;
    return 0;
}


void CStack62056::DoEncryption(unsigned char ApduType, unsigned char *lpText, unsigned int *TextLen,unsigned char SecurityPolicy)
{
	unsigned char EK_Buf[KEY_LEN],type;
	unsigned char tmp_Buf[PLAINTEXT_LEN];
	
    //如application context name为加密结合security option对Initiate.req进行加密,高安全等级不带加密的intiiate不加密
    if((mAarqParams.ApplicationContextName == LN_CIPER || mAarqParams.ApplicationContextName == SN_CIPER || 
		((mAarqParams.MechanismName > LOW_LEVEL)&& ApduType!=INITIATE_REQUEST)) && SecurityPolicy != NO_SECURITY)//
    {
        unsigned EncryptionApduType = ApduType;
        switch(ApduType)
        {
            case INITIATE_REQUEST:EncryptionApduType = GLO_INITIATE_REQUEST;break;
            case GET_REQUEST:EncryptionApduType = GLO_GET_REQUEST;break;
            case SET_REQUEST:EncryptionApduType = GLO_SET_REQUEST;break;
            case ACTION_REQUEST:EncryptionApduType = GLO_ACTION_REQUEST;break;
        }

	if(mAarqParams.XdlmsInitiateReq.ResponseAllowed == FALSE)
		memcpy(EK_Buf,mSecurityOption.SecurityMaterial.BK,KEY_LEN);
	else if(mAarqParams.XdlmsInitiateReq.DedicatedKeyLen == 0 || EncryptionApduType==GLO_INITIATE_REQUEST)
		memcpy(EK_Buf,mSecurityOption.SecurityMaterial.EK,KEY_LEN);
	else
		memcpy(EK_Buf,mSecurityOption.SecurityMaterial.DK,KEY_LEN);

        //if(mSecurityOption.SecurityPolicy != NO_SECURITY)
        //{
            if(mSecurityOption.SecuritySuite == AES_GCM_128)
            {
                unsigned char *lptmp = lpText;
		  if(EncryptionApduType==GLO_INITIATE_REQUEST)
		  {
			memcpy(ase_gcm_128->S,lpText,*TextLen);
						ase_gcm_128->Encrypt_StringData(SC_U_AE, (char*)EK_Buf, mAarqParams.CallingAPTitle,
							(char*)mSecurityOption.SecurityMaterial.IV.params.FC, (char*)mSecurityOption.SecurityMaterial.AK, *TextLen, (char*)ase_gcm_128->T);

						//ase_gcm_128->AES_GCM_Encryption(TYPE_ENCRYPTION, SC_U_AE, mAarqParams.CallingAPTitle,
                                           // mSecurityOption.SecurityMaterial.IV.params.FC, mSecurityOption.SecurityMaterial.EK,
                                            //mSecurityOption.SecurityMaterial.AK, lpText, *TextLen);
                                            //TAG || LEN || SH || C || T 
                                            *lptmp++ = EncryptionApduType;//TAG
                                            lptmp += EnCodeLength(SH_LEN + *TextLen + T_LEN, lptmp);
                                            *lptmp++ = SC_U_AE;
                                            memcpy(lptmp, mSecurityOption.SecurityMaterial.IV.params.FC, FC_LEN);
                                            lptmp += FC_LEN;
                                            memcpy(lptmp, ase_gcm_128->S, *TextLen);
                                            lptmp += *TextLen;
                                            memcpy(lptmp, ase_gcm_128->T, T_LEN);
                                            lptmp += T_LEN;
                                            *TextLen = lptmp - lpText;
//			return;
		  }
		  else
		  {
          		switch(SecurityPolicy)
                        {
                        /*void AES_GCM_Encryption(unsigned char Type, unsigned char SC, unsigned char *Sys_T,
                                            unsigned char *FC, unsigned char *EK, unsigned char *AK, unsigned char *TextBefore, unsigned short TextBeforeLen);
                    */
                        case ALL_MESSAGES_AUTHENTICATED:
						type = SC_U_A;
						 memcpy(ase_gcm_128->S,&type,1);
	      					memcpy(ase_gcm_128->S+SC_LEN,mSecurityOption.SecurityMaterial.AK,KEY_LEN);
            					memcpy(ase_gcm_128->S+KEY_LEN+SC_LEN,lpText,*TextLen);
						memcpy(tmp_Buf,lpText,*TextLen);
						ase_gcm_128->Encrypt_StringData(SC_U_A, (char*)EK_Buf, mAarqParams.CallingAPTitle,
							(char*)mSecurityOption.SecurityMaterial.IV.params.FC, (char*)mSecurityOption.SecurityMaterial.AK, *TextLen, (char*)ase_gcm_128->T);
						//	AES_GCM_Encryption(TYPE_ENCRYPTION, SC_U_A, mAarqParams.CallingAPTitle,
                                          //  mSecurityOption.SecurityMaterial.IV.params.FC, mSecurityOption.SecurityMaterial.EK,
                                          //  mSecurityOption.SecurityMaterial.AK, lpText, *TextLen);
                                            //TAG || LEN || SH || APDU || T
                                            *lptmp++ = EncryptionApduType;//TAG
                                            lptmp += EnCodeLength(SH_LEN + *TextLen + T_LEN, lptmp);
                                            *lptmp++ = SC_U_A;
                                            memcpy(lptmp, mSecurityOption.SecurityMaterial.IV.params.FC, FC_LEN);
                                            lptmp += FC_LEN;
                                            memcpy(lptmp, tmp_Buf, *TextLen);
                                            lptmp += *TextLen;
                                            memcpy(lptmp, ase_gcm_128->T, T_LEN);
                                            lptmp += T_LEN;
                                            *TextLen = lptmp - lpText;
                                            break;
                        case ALL_MESSAGES_ENCRYPTED:
						memcpy(ase_gcm_128->S,lpText,*TextLen);
						ase_gcm_128->Encrypt_StringData(SC_U_E, (char*)EK_Buf, mAarqParams.CallingAPTitle,
							(char*)mSecurityOption.SecurityMaterial.IV.params.FC, (char*)mSecurityOption.SecurityMaterial.AK, *TextLen, (char*)ase_gcm_128->T);

						//ase_gcm_128->AES_GCM_Encryption(TYPE_ENCRYPTION, SC_U_E, mAarqParams.CallingAPTitle,
                                          //  mSecurityOption.SecurityMaterial.IV.params.FC, mSecurityOption.SecurityMaterial.EK,
                                          //  mSecurityOption.SecurityMaterial.AK, lpText, *TextLen);
                                            //TAG || LEN || SH || C
                                            *lptmp++ = EncryptionApduType;//TAG
                                            lptmp += EnCodeLength(SH_LEN + *TextLen, lptmp);
                                            *lptmp++ = SC_U_E;
                                            memcpy(lptmp, mSecurityOption.SecurityMaterial.IV.params.FC, FC_LEN);
                                            lptmp += FC_LEN;
                                            memcpy(lptmp, ase_gcm_128->S, *TextLen);
                                            lptmp += *TextLen;
                                            *TextLen = lptmp - lpText;
                                            break;
                        case ALL_MESSAGES_AUTHENTICATED_AND_ENCRYPTED:
						memcpy(ase_gcm_128->S,lpText,*TextLen);
						ase_gcm_128->Encrypt_StringData(SC_U_AE, (char*)EK_Buf, mAarqParams.CallingAPTitle,
							(char*)mSecurityOption.SecurityMaterial.IV.params.FC, (char*)mSecurityOption.SecurityMaterial.AK, *TextLen, (char*)ase_gcm_128->T);

						//ase_gcm_128->AES_GCM_Encryption(TYPE_ENCRYPTION, SC_U_AE, mAarqParams.CallingAPTitle,
                                           // mSecurityOption.SecurityMaterial.IV.params.FC, mSecurityOption.SecurityMaterial.EK,
                                            //mSecurityOption.SecurityMaterial.AK, lpText, *TextLen);
                                            //TAG || LEN || SH || C || T 
                                            *lptmp++ = EncryptionApduType;//TAG
                                            lptmp += EnCodeLength(SH_LEN + *TextLen + T_LEN, lptmp);
                                            *lptmp++ = SC_U_AE;
                                            memcpy(lptmp, mSecurityOption.SecurityMaterial.IV.params.FC, FC_LEN);
                                            lptmp += FC_LEN;
                                            memcpy(lptmp, ase_gcm_128->S, *TextLen);
                                            lptmp += *TextLen;
                                            memcpy(lptmp, ase_gcm_128->T, T_LEN);
                                            lptmp += T_LEN;
                                            *TextLen = lptmp - lpText;
                                            break;
                        }
                    }
				
			unsigned long Counter = (((unsigned long)mSecurityOption.SecurityMaterial.IV.params.FC[0]<<24)
									|((unsigned long)mSecurityOption.SecurityMaterial.IV.params.FC[1]<<16)
									|((unsigned long)mSecurityOption.SecurityMaterial.IV.params.FC[2]<<8)
									|((unsigned long)mSecurityOption.SecurityMaterial.IV.params.FC[3]));
			Counter += 1;
			mSecurityOption.SecurityMaterial.IV.params.FC[0] = (unsigned char)(Counter>>24);
			mSecurityOption.SecurityMaterial.IV.params.FC[1] = (unsigned char)(Counter>>16);
			mSecurityOption.SecurityMaterial.IV.params.FC[2] = (unsigned char)(Counter>>8);
			mSecurityOption.SecurityMaterial.IV.params.FC[3] = (unsigned char)(Counter);
       }
        //}
    }
}
