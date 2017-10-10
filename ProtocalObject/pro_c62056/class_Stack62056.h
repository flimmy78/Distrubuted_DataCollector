//---------------------------------------------------------------------------

#ifndef class_Stack62056H
#define class_Stack62056H
//---------------------------------------------------------------------------
#include <string>
//#include <msxmldom.hpp>
//#include <XMLDoc.hpp>
//#include <xmldom.hpp>
//#include <XMLIntf.hpp>
#include "class_AESGCM.h"
#include "Public.h"
#include "C62056_Struct.h"
#include "./xml/Markup.h"

//---------------------------------
#define CLIENT_LONG_TRANSFER_SUPPORT
//-------------------
//Ĭ�ϵĿͻ���������APDU�ĳ���.Ӧ�ý��̿��Ը���һ��ֵ,����ֵΪ0(��ʾΪ�����),��ʱʹ��Ĭ��ֵ
//����ֵ��Ϊ0,��С��Ĭ��ֵ,��ʹ��Ӧ�ý��̸���ֵ
#define DEFAULT_CLIENT_MAX_RECEIVED_PDU_SIZE (2048)   //�˲���ʵ�����������˷�����HDLC���֡�����е���󳤶�
#define MAX_CLIENT_RECEIVED_PDU_SIZE (65535 * 8)
//���ڿͻ�������͵�APDU����,�ɷ���������.���������������ֵΪ0,��ʹ�ÿͻ���HDLC�����:�������Ϣ�򳤶�(ԭ���ǿͻ����޷�֡)
//�����������������ֵ�ȿͻ��˷�����Ϣ��Ҫ��,����Ȼ���ոò���,������֡������Χʱ�ٸ���������ʾ

//--------------------------
#define DEFAULT_WINDOWSIZE_TRANSMIT 1
#define DEFAULT_WINDOWSIZE_RECEIVE 7
#define DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_TRANSMIT 1024
#define DEFAULT_MAXIMUMINFORMATIONFIELDLENGTH_RECEIVE 1024

#define ALL_STATION_ADDRESS 0x3FFF
#define CALLING_ADDRESS 0x3FFE

#define MAX_RETRY_TIMES 3//�������3��

//-----------------------------------------
typedef enum _SERVICE_CLASS
{
    UNCONFIRMED = 0x00,
    CONFIRMED
}SERVICE_CLASS;

typedef enum _PRIORITY
{
    NORMAL = 0x00,
    HIGH
}PRIORITY;

typedef enum _RETRY_MODE
{
    SEND_TEST_RR = 0x00,   //���Ͳ���RR֡
    SEND_REAL_FRAME//����������֡
}RETRY_MODE;

//���ڽ��շ�֡����
typedef struct _LONG_TRANSFER_RECEIVE
{
    bool Flag;
    unsigned char *Buff;
    unsigned int Len;
}LONG_TRANSFER_RECEIVE;

//���ڽ��շ�֡����
#ifdef CLIENT_LONG_TRANSFER_SUPPORT
typedef struct _LONG_TRANSFER_SEND
{
    bool Flag;
    unsigned char *Buff;
    unsigned int Len;
    unsigned int Offset;
}LONG_TRANSFER_SEND;
#endif
//
typedef struct _BLOCK_TRANSFER
{
    bool Flag;
    unsigned int BlockNumber;
    unsigned int Offset;//��apdubuff�е�ƫ����
    unsigned int RawDataLen;//ÿ�λ�������ݳ���
    unsigned int RawDataLenEncodeLen;//���ݳ���ֵ�ı��볤��
}BLOCK_TRANSFER;

typedef struct _BLOCK_RECEIVE
{
    bool Flag;
    unsigned int BlockNumber;
}BLOCK_RECEIVE;

typedef struct _LAST_DATA_REQUEST
{
    unsigned char ServiceType;
    INVOKE_ID_AND_PRIORITY InvokeIdAndPriority;
    unsigned short RequestType;
    unsigned int DescriptorsCount;  
}LAST_DATA_REQUEST;


typedef struct _LAST_I_FRAME
{
    bool Segment;
    bool PorF;
    unsigned char SendNo;
    unsigned char RecvNo;
    unsigned char *Infor;
    unsigned int InforLen;
}LAST_I_FRAME;



//
const unsigned char DefaultMK[KEY_LEN] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
const unsigned char DefaultEK[KEY_LEN] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
const unsigned char DefaultAK[KEY_LEN] = {0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF};

//---------------------------------------
class CStack62056
{
    friend class CParseData;
    public:
        CStack62056(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);
        ~CStack62056();

    public:
        void Reset(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);
        SUPPORT_LAYER GetSupportLayerType();
        ADDR GetAddr();
        unsigned char * GetID();
        bool GetStackUsageFlag();
        //
        int SetParams(const unsigned char *Xml, unsigned int XmlLen);
        void PhyAbort();
        int ResponseTimeOut(unsigned char **OUTData, unsigned int &OUTDataLen);
        int ProcessServicePrimitive(unsigned char **OUTData, unsigned int &OUTDataLen,  
                                    const unsigned char *Xml, unsigned int XmlLen);
        int ProcessHDLCFrame(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    bool Segment, unsigned char RecvNo, unsigned char SendNo,
                                    bool PorF, FRAME_COMMAND FrameCommand,
                                    const unsigned char *Infor, unsigned int InforLen);
        int ProcessUdpWpdu(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    const unsigned char *lpPdu, unsigned int PduLen);
        //����  ��������
        unsigned char SetSecurity(SECURITY_OPTION SecurityOption);
    public:

    private:
        //***********Security****************************//
        void DoEncryption(unsigned char ApduType, unsigned char *lpText, unsigned int *TextLen,unsigned char SecurityPolicy);
    private:
        //***********COSEMApp****************************//
        //COSEMApp���ṩ��HDLC����õķ�����
        int DlConnectCnf(CMarkup& OutRoot, DL_CONNECT_RESULT ConnResult, const unsigned char *lpInfor, unsigned int InforLen);
        int DlDataInd(CMarkup& OutRoot, FRAME_TYPE FramType, const unsigned char *lpPdu, unsigned int PduLen);
        int DlDisconnectInd(CMarkup& OutRoot, DL_DISC_REASON Reason, bool Uss, const unsigned char *lpInfor, unsigned int InforLen);
        int DlDisconnectCnf(CMarkup& OutRoot, DL_DISC_RESULT Result, const unsigned char *lpInfor, unsigned int InforLen);
        //COSEMApp���ṩ��Udp Wrapper����õķ�����
        int UdpDataInd(CMarkup& OutRoot, const unsigned char *lpPdu, unsigned int PduLen);

        //
        int InitApduBuff();
        void DeleteApduBuff();
        int InitBlockRecvBuff();
        void DeleteBlockRecvBuff();
        int ReallocBlockRecvBuff();

        void ResetCosemAppAssociation();
        int ProcessCosemOpenReq(CMarkup& OutRoot, CMarkup& InRoot);
        int ProcessCosemReleaseReq(CMarkup& OutRoot, CMarkup& InRoot);
        int ProcessGetReq(CMarkup& OutRoot, CMarkup& InRoot);
        int ProcessSetReq(CMarkup& OutRoot, CMarkup& InRoot);
        int ProcessActionReq(CMarkup& OutRoot, CMarkup& InRoot);
        int ProcessTriggerEventNotificationSendingReq(CMarkup& OutRoot, CMarkup& InRoot);
        int ProcessReadReq(CMarkup& OutRoot, CMarkup& InRoot);
        int ProcessWriteReq(CMarkup& OutRoot, CMarkup& InRoot, bool Confirmed);

        int ParseAarq(CMarkup& Root);
        int ParseRlrq(CMarkup& Root);
        int ParseDataReqHead(int &InvokeId, bool &Priority, SERVICE_CLASS &ServiceClass, int &RequestType,
                                CMarkup& Root);
        int ParseCosemAttributeDescriptor(int &AttributeId, unsigned char *Dest, unsigned int &Len, CMarkup& Parent);
        bool ParseSelectiveAccessParameters(unsigned char *Dest, unsigned int &Len, CMarkup& Parent);
        int ParseCosemMethodDescriptor(unsigned char *Dest, unsigned int &Len, CMarkup& Parent);
        int ParseCosemAttributeDescriptors(unsigned char *Dest, unsigned int &Len,
                                unsigned int &DescriptorsCount, bool Attribute0Supported, CMarkup& Root);
        int ParseCosemMethodDescriptors(unsigned char *Dest, unsigned int &Len,
                                unsigned int &DescriptorsCount, CMarkup& Root);

        int ParseVariableAccessSpecification(unsigned char *Dest, unsigned int &Len, CMarkup& Parent);
        bool ParseData(unsigned int &Len, unsigned char *lpDest, CMarkup& DataNode);
        //
        int ProcessAARE(CMarkup& OutRoot, const unsigned char *lpAare, unsigned int AareLen);
        int ProcessRLRE(CMarkup& OutRoot, const unsigned char *lpRlre, unsigned int RlreLen);
        int ProcessEventNotificationInd(CMarkup& OutRoot, const unsigned char *lpNotificationInd, unsigned int NotificationIndLen);
        int ProcessPushData(CMarkup& OutRoot, const unsigned char *lpNotificationInd, unsigned int NotificationIndLen);
        int ProcessGetRes(CMarkup& OutRoot, const unsigned char *lpGetRes, unsigned int GetResLen);
        int ProcessSetRes(CMarkup& OutRoot, const unsigned char *lpSetRes, unsigned int SetResLen);
        int ProcessActionRes(CMarkup& OutRoot, const unsigned char *lpActionRes, unsigned int ActionResLen);
        int ProcessGloRes(CMarkup& OutRoot, unsigned char ApduType, const unsigned char *lpRes, unsigned int ResLen);

        int ProcessReadRes(CMarkup& OutRoot, const unsigned char *lpReadRes, unsigned int ReadResLen);
        int ProcessWriteRes(CMarkup& OutRoot, const unsigned char *lpWriteRes, unsigned int WriteResLen);
        int ProcessInformationReportInd(CMarkup& OutRoot, const unsigned char *lpInformationReportInd, unsigned int InformationReportIndLen);
        int ProcessConfirmedServiceError(CMarkup& OutRoot, const unsigned char *lpConfirmedServiceError, unsigned int ConfirmedServiceErrorLen);

        void XmlToStr(CMarkup& OutXml, unsigned char **OUTData, unsigned int &OUTDataLen);
        void WriteAddr(CMarkup& Root);
        void WriteDataResHead(int InvokeId, bool Priority, int ResponseType, CMarkup& Root);


        void WriteCosemOpenCnf(CMarkup& OutRoot, DL_CONNECT_RESULT ConnResult);
        void WriteCosemReleaseCnf(CMarkup& OutRoot, DL_DISC_RESULT Result);
        void MakeXdlmsInitReq(unsigned char *XdlmsInitReq, unsigned int &XdlmsInitReqLen);
        int MakeAarqApdu();
        int MakeHLSApdu();
        int MakeRlrqApdu();
    private:
        int DecodeAndWriteDataAccessResult(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent);
        int DecodeAndWriteGetDataResult(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent);
        int DecodeAndWriteActionResult(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent);
        int DecodeAndWriteActionResponsewithOptionalData(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent);
        int DecodeAndWriteData(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent, std::string NodeName = "Data");

        int DecodeAndWriteVariableAccessSpecification(unsigned int &Len, const unsigned char *lpSrc, CMarkup& Parent);
    private:

        //***********HDLC****************************//
        //HDLC���ṩ��COSEMApp����õķ�����
        int DlConnectReq(CMarkup& OutRoot, unsigned char *Infor, unsigned int InforLen);
        int DlDisconnectReq(CMarkup& OutRoot, unsigned char *Infor, unsigned int InforLen);
        int DlDataReq(CMarkup& OutRoot, FRAME_TYPE FrameType, unsigned char *lpPdu, unsigned int PduLen);

        //UDP Wrapper���ṩ��COSEMApp����õķ�����
        int UdpDataReq(CMarkup& OutRoot, unsigned char *lpPdu, unsigned int PduLen);
        //
        void ResetHdlcConnection();
        int ProcessI(CMarkup& OutRoot, bool Segment, unsigned char RecvNo, unsigned char SendNo,
                    bool PorF, const unsigned char *Infor, unsigned int InforLen);
        int ProcessUI(CMarkup& OutRoot, bool PorF, const unsigned char *Infor, unsigned int InforLen);
        int ProcessRR(CMarkup& OutRoot, unsigned char RecvNo);
        int ProcessDM(CMarkup& OutRoot, const unsigned char *Infor, unsigned int InforLen);
        int ProcessUA(CMarkup& OutRoot, const unsigned char *Infor, unsigned int InforLen);

        void WriteFrameToXml(CMarkup& OutRoot, unsigned char *lpFrame, unsigned int FrameLen);
        void MakeFrame(CMarkup& OutRoot, bool Segment, unsigned char Ctrl);
        void MakeSNRM(CMarkup& OutRoot, bool PorF);
        void MakeDISC(CMarkup& OutRoot, bool PorF);
        void MakeUI(CMarkup& OutRoot, bool PorF, const unsigned char *Infor, unsigned int InforLen);
        void MakeRR(CMarkup& OutRoot);
        void MakeI(CMarkup& OutRoot, bool Segment, bool PorF, bool LlcHead, const unsigned char *Infor, unsigned int InforLen);
        unsigned char MakeHdlcParamsField(unsigned char *Dest);
        void MakeLastI(CMarkup& OutRoot);
        #ifdef CLIENT_LONG_TRANSFER_SUPPORT
        void MakeLongTransferSend(CMarkup& OutRoot, bool First);
        #endif

        bool CheckSequenceNumber(unsigned char RecvNo, unsigned char SendNo);
        bool GetHdlcParams(const unsigned char *Infor, unsigned int InforLen);

        //***********UDP Wrapper****************************//   
        void AddUdpWrapper(CMarkup& OutRoot, unsigned char *lpPdu, unsigned int PduLen);
    private:
        void InitSecurity();
        void InitCosemAppParams();
        void InitAarqParams();
        void InitRlrqParams();
        void InitHdlcParams();
    private:
        SUPPORT_LAYER mSupportLayerType;
        ADDR mAddr;
        unsigned char mID[ID_LEN];
        unsigned int mXmlLen;

        //*******************Security******************//
        SECURITY_OPTION mSecurityOption;
        
        //*******************COSEMApp******************//
        COSEMAPP_STATE mCosemAppState;
        AARQ_PARAMS mAarqParams;
        AARE_PARAMS mAareParams;
        RLRQ_PARAMS mRlrqParams;
        unsigned int mClientMaxSendPduSize;//�˲���Ϊһ��Э�̺���������ݷ������ظ���AARE����ȡ��������������pdu�ߴ�
        //����ͻ��˵��������Ϣ�򳤶�֮��ȡ��С����Ϊ�ͻ��˲�֧�ַ�֡����һ��pduֻ����һ֡�з�����ϡ�
        //�������ظ���������pdu�ߴ���û����Сֵ������( > 11)
        //��Э��涨��descriptor�����ڵ�һ�����з��ͣ��� descriptor ��֮ǰ�ı������ С�����ֵ

        //ACTION_REQUEST_TYPE LastActionReqType;
        LAST_DATA_REQUEST mLastDataRequest;
        
        unsigned char *mApduBuff;
        unsigned int mApduLen;
	unsigned char flag;
        //����hdlc��apdu���ݺͳ���
        unsigned char *mApduToHdlcBuff;
        unsigned int mApduToHdlcLen;
        CMarkup mOutRoot;

        BLOCK_TRANSFER mBlockTransfer; 

        //���ڽ��շ������ķֿ�����
        unsigned char *mBlockRecvBuff;
        unsigned int mBlockRecvLen;
        unsigned int mBlockRecvBuffSize;

        BLOCK_RECEIVE mBlockReceive;
        //*******************HDLC********************//
        MAC_STATE mMacState;
        HDLC_PARAMS HdlcParamsDefault;
        HDLC_PARAMS HdlcParamsNegotiated;

        unsigned char mRecvNo, mSendNo;
        bool mPermission;

        bool RetringMode;//����I֡�ط�ģʽ
        RETRY_MODE RetryMode;//
        unsigned char RetryTimes;//�ط�����

        //��֡���̲���
        LONG_TRANSFER_RECEIVE LongTransferReceive;
        #ifdef CLIENT_LONG_TRANSFER_SUPPORT
        LONG_TRANSFER_SEND LongTransferSend;
        #endif

        LAST_I_FRAME LastIFrame;
        //���ڴ�����������Ӧ�ý��̷��͵�֡
        unsigned char *mOutFrame;
        unsigned int mOutFrameLen;
    private:
        CAESGCM128 *ase_gcm_128;
};
#endif
