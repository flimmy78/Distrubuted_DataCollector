//---------------------------------------------------------------------------

#ifndef class_HDLCH
#define class_HDLCH
//---------------------------------------------------------------------------
#include "C62056_Struct.h"
#include "Public.h"
//-----------------------------------------

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
		unsigned short FrameType : 4;
		unsigned short Segment : 1;
		unsigned short FrameLength : 11;
	}bits;
}Frame_Format;

typedef union _Ctrl_Format
{
	unsigned char Val;
	struct
	{
		unsigned char RRR : 3;
		unsigned char PorF : 1;
		unsigned char SSS : 3;
		unsigned char Res : 1;
	}bits;
}Ctrl_Format;

typedef struct _HDLC_PARAMS
{
	unsigned char WindowSizeTransmit, WindowSizeReceive;
	unsigned short MaximumInformationFieldLengthTransmit, MaximumInformationFieldLengthReceive;
}HDLC_PARAMS;


//------------------------------------------------------------
class CHDLC
{
    public:
        CHDLC(ADDR Addr);
        ~CHDLC();
    public:
        void Reset(ADDR Addr);
        int ProcessDlPrimitiveService(unsigned char *OUTData, unsigned int &OUTDataLen,
                                    DL_PRIMITIVESERVICE DlPrimitiveService);
        int ProcessFrame(unsigned char *OUTData, unsigned int &OUTDataLen,
                                    bool Segment, unsigned char RecvNo, unsigned char SendNo,
                                    bool PorF, FRAME_COMMAND FrameCommand,
                                    const unsigned char *Infor, unsigned int InforLen);
    public:
        //int Result;
        DL_PRIMITIVESERVICE mDlPrimitiveService;
    private:
        int ProcessI(bool Segment, unsigned char RecvNo, unsigned char SendNo,
                    bool PorF, const unsigned char *Infor, unsigned int InforLen);
        int ProcessUI(bool PorF, const unsigned char *Infor, unsigned int InforLen);
        int ProcessRR(unsigned char RecvNo);
        int ProcessDM(const unsigned char *Infor, unsigned int InforLen);
        int ProcessUA(const unsigned char *Infor, unsigned int InforLen);
        
        void MakeFrame(bool Segment, unsigned char Ctrl, const unsigned char *Infor, unsigned int InforLen);
        void MakeSNRM(bool PorF);
        void MakeDISC(bool PorF);
        void MakeUI(bool PorF, const unsigned char *Infor, unsigned int InforLen);
        void MakeRR();
        void MakeI(bool Segment, bool PorF, const unsigned char *Infor, unsigned int InforLen);
        unsigned char MakeHdlcParamsField(unsigned char *Dest);


        bool CheckSequenceNumber(unsigned char RecvNo, unsigned char SendNo);
        bool GetHdlcParams(const unsigned char *Infor, unsigned int InforLen);
    private:
        ADDR mAddr;
        MAC_STATE mMacState;
        HDLC_PARAMS HdlcParamsDefault;
        HDLC_PARAMS HdlcParamsNegotiated;

        unsigned char mRecvNo, mSendNo;
	
	    bool LongTransfer_Recv;//
        unsigned char *HdlcRecvBuff;//use to buffer the information field, len = maximum information field length_receive * window size_receive
	    unsigned int HdlcRecvBuffOccupiedLen;//occupied length
	
//	    bool LongTransfer_Send;
//	    unsigned char *HdlcSendBuff;
//	    unsigned int HdlcSendBuffLen;
//	    unsigned int HdlcSendBuffOffset;
        //--------------------------------

        unsigned char *mOutData;
        unsigned int mOutDataLen;
};
#endif
