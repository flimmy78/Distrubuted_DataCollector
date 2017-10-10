#ifndef _DLL_FILE_
    #define _DLL_FILE_
#endif
//---------------------------------------------------------------------------

#include "C62056.h"
//---------------------------------------------------------------------------
//#include <msxmldom.hpp>
//#include <XMLDoc.hpp>
//#include <xmldom.hpp>
//#include <XMLIntf.hpp>
//#include <assert.h>

#include "Class_StackList.h"
#include "calc_fcs.h"

//



//static int GetAddr(ADDR &Addr, SUPPORT_LAYER SupportLayerType, _di_IXMLNode Root);
//--------------------------------
static unsigned int CheckFrame(bool *Segment, unsigned int *ServerAddr, unsigned char *ServerAddrLen, unsigned char *ClientAddr,
            unsigned char *RecvNo, unsigned char *SendNo, bool *PorF, FRAME_COMMAND *FrameCommand,
	        unsigned int *InforOffset, unsigned int *InforLen, unsigned int *ActualFrameLen,
            const unsigned char *lpFrame, unsigned int FrameLen);

static bool CheckAddress(bool PorF, FRAME_COMMAND FrameCommand, unsigned int ServerAddrLen,
                unsigned short ServerAddrUpper, unsigned short *ServerAddrLower, unsigned char ClientAddr);

static unsigned int CheckUdpFrame(/*unsigned char *ID, */unsigned short *ServerwPort, unsigned short *ClientwPort,
	        unsigned int *InforOffset, unsigned int *InforLen, unsigned int *ActualFrameLen,
            const unsigned char *lpFrame, unsigned int FrameLen);
            
static bool CheckwPort(unsigned short ClientwPort, unsigned short *ServerwPort);
//--------------------------------
StackList *gStackList;
//unsigned int gXmlLen;
//--------------------------------

int _stdcall SetParams(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr, const unsigned char *Xml, unsigned int XmlLen)
{
    //assert(SupportLayerType != HDLC);//目前仅实现了HDLC类型的协议栈

    //assert(Xml == NULL || XmlLen == 0);

    int Result;    

    CStack62056 *Stack = gStackList->Find(SupportLayerType, ID, Addr);
    if(Stack == NULL)
    {
        Stack = gStackList->Add(SupportLayerType, ID, Addr);
        if(Stack == NULL)
        {
            return 2;
        }
    }

    Result = Stack->SetParams(Xml, XmlLen);

    return Result;
};
//----------------------------------------------------
void _stdcall ReleaseMemory(unsigned char **OUTData)
{
    if(*OUTData != NULL)
    {
        delete [] *OUTData;
        *OUTData = NULL;
    }
};

int _stdcall ResponseTimeOut(unsigned char **OUTData, unsigned int &OUTDataLen, SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr)
{
    //assert(SupportLayerType == HDLC);//目前仅实现了HDLC类型的协议栈
    *OUTData = NULL;
    
    CStack62056 *Stack = gStackList->Find(SupportLayerType, ID, Addr);
    if(Stack != NULL)
    {
        return Stack->ResponseTimeOut(OUTData, OUTDataLen);
    }
    else
    {
        return 9;
    } 
};

/*
返回值取值及含义定义如下：
 	=0，OutDataReturnData内容为帧
 	=-1，OutDataReturnData内容为XML文档
 	>0，OutDataReturnData内容为空，返回值为错误码
     
*/
int _stdcall ProcessServicePrimitive(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr,
                                    const unsigned char *Xml, unsigned int XmlLen)
{
    //assert(SupportLayerType == HDLC);//目前仅实现了HDLC类型的协议栈

    //assert(Xml != NULL || XmlLen > 0);

    int Result;
    *OUTData = NULL;
    
    CStack62056 *Stack = gStackList->Find(SupportLayerType, ID, Addr);
    if(Stack == NULL)
    {
        Stack = gStackList->Add(SupportLayerType,  ID, Addr);
        if(Stack == NULL)
        {
            return 2;
        }
    }

    Result = Stack->ProcessServicePrimitive(OUTData, OUTDataLen, Xml, XmlLen);

    return Result;
};

//---------------------------------------------------------------------------

/*
返回值取值及含义定义如下：
 	=0，OutDataReturnData内容为帧
 	=-1，OutDataReturnData内容为XML文档
 	>0，OutDataReturnData内容为空，返回值为错误码

*/
int _stdcall ProcessFrame(unsigned char **OUTData, unsigned int &OUTDataLen,
                                    SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN],
                                    const unsigned char *Frame, unsigned int FrameLen)
{
    //assert(SupportLayerType == HDLC);//目前仅实现了HDLC类型的协议栈

    //assert(Frame != NULL && FrameLen > 0);
    
	int Result;
    ADDR Addr;

    *OUTData = NULL;
    
    if(SupportLayerType == HDLC)
    {
        bool Segment;
        unsigned char ClientAddr;
        unsigned int ServerAddr;
        unsigned char ServerAddrLen;
        unsigned char RecvNo;
        unsigned char SendNo;
        bool PorF;
        FRAME_COMMAND FrameCommand;
        unsigned int InforOffset;
        unsigned int InforLen;
        unsigned int  ActualFrameLen;

        
        Result = CheckFrame(&Segment, &ServerAddr, &ServerAddrLen, &ClientAddr, &RecvNo, &SendNo, &PorF, &FrameCommand,
                            &InforOffset, &InforLen, &ActualFrameLen, Frame, FrameLen);
        if(Result != 0)//ok
        {
            return Result;
        }

        Addr.HDLC_ADDR2.ServerAddr = ServerAddr;
        Addr.HDLC_ADDR2.ClientAddr = ClientAddr;
        Addr.HDLC_ADDR2.ServerAddrLen = ServerAddrLen;

        CStack62056 *Stack = gStackList->Find(SupportLayerType, ID, Addr);
        if(Stack == NULL)
        {
            Stack = gStackList->Add(SupportLayerType,  ID, Addr);
            if(Stack == NULL)
            {         
                return 2;
            }
        }

        return Stack->ProcessHDLCFrame(OUTData, OUTDataLen, Segment, RecvNo, SendNo, PorF, FrameCommand,
                            Frame + InforOffset, InforLen);
    }
    else if(SupportLayerType == UDPIP)
    {
        //unsigned char ID[ID_LEN];
        unsigned short ClientwPort;
        unsigned short ServerwPort;
        unsigned int InforOffset;
        unsigned int InforLen;
        unsigned int ActualFrameLen;

    
        Result = CheckUdpFrame(/*ID, */&ServerwPort, &ClientwPort, &InforOffset, &InforLen, &ActualFrameLen, Frame, FrameLen);
        if(Result != 0)//ok
        {
            return Result;
        }

        //memcpy(Addr.ID, ID, ID_LEN);
        Addr.UDPIP_ADDR.Server_wPort = ServerwPort;
        Addr.UDPIP_ADDR.Client_wPort = ClientwPort;


        CStack62056 *Stack = gStackList->Find(SupportLayerType, ID, Addr);
        if(Stack == NULL)
        {
            Stack = gStackList->Add(SupportLayerType,  ID, Addr);
            if(Stack == NULL)
            {
                return 2;
            }
        }
        
        return Stack->ProcessUdpWpdu(OUTData, OUTDataLen, Frame + InforOffset, InforLen);
    } //modified by slx 2012-9-14
    else
    {
        return 1;
    }
};

void _stdcall PhyAbort(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr)
{
    //assert(SupportLayerType == HDLC);//目前仅实现了HDLC类型的协议栈

    CStack62056 *Stack = gStackList->Find(SupportLayerType, ID, Addr);
    if(Stack != NULL)
    {
        Stack->PhyAbort();
    }
};

//2013-9-10 新增  加密参数设置函数
unsigned char _stdcall SetSecurity(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr,
                                    SECURITY_OPTION SecurityOption)
{
    CStack62056 *Stack = gStackList->Find(SupportLayerType, ID, Addr);
    if(Stack == NULL)
    {
        Stack = gStackList->Add(SupportLayerType, ID, Addr);
        if(Stack == NULL)
        {
            return 2;
        }
    }
    if(Stack != NULL)
    {
        return Stack->SetSecurity(SecurityOption);
    }
    return 16;
};

//--------------------------------------------------------------------------------------------
/*int GetAddr(ADDR &Addr, SUPPORT_LAYER SupportLayerType, _di_IXMLNode Root)
{
    _di_IXMLNode AddrNode = Root->ChildNodes->FindNode("Protocol_Connection_Parameters");
    if(AddrNode == NULL)
    {
        return 4;
    }

    if(SupportLayerType == HDLC)
    {
        _di_IXMLNode Node = AddrNode->ChildNodes->FindNode("Server_MAC_Address");
        if(Node == NULL)
        {
            Node = AddrNode->ChildNodes->FindNode("Server_Lower_MAC_Address");
            if(Node == NULL)
            {
                return 5;
            }
            try
            {
                Addr.HDLC_ADDR1.ServerLowerAddr = StrToInt(Node->GetAttribute("Value"));
            }
            catch(EConvertError &e)
            {
                return 6;
            }

            Node = AddrNode->ChildNodes->FindNode("Server_Upper_MAC_Address");
            if(Node == NULL)
            {
                return 5;
            }
            try
            {
                Addr.HDLC_ADDR1.ServerUpperAddr = StrToInt(Node->GetAttribute("Value"));
            }
            catch(EConvertError &e)
            {
                return 6;
            }              
        }
        else
        {
            try
            {
                Addr.HDLC_ADDR2.ServerAddr = StrToInt(Node->GetAttribute("Value"));
            }
            catch(EConvertError &e)
            {
                return 6;
            }
        }
    
        Node = AddrNode->ChildNodes->FindNode("Client_MAC_Address");
        if(Node == NULL)
        {
            return 7;
        }
        try
        {
            Addr.HDLC_ADDR2.ClientAddr = StrToInt(Node->GetAttribute("Value"));
        }
        catch(EConvertError &e)
        {
            return 8;
        }
        return 0;
    }
    else
    {
        return 1;
    }
}*/

    
/*************************************************
  Function:       CheckFrame
  Description:    解析接收帧
  Calls:          无
  Called By:

  Input:          帧起始地址和长度
  Output:         分段位,服务器地址,客户端地址,接收序列号,发送序列号,PF位,帧类型,信息域相对于起始位置的偏移量,信息域长度,帧实际长度
  Return:         unsigned int类型的错误码,0--解析成功,其他解析失败
  Others:         // 其它说明
*************************************************/
unsigned int CheckFrame(bool *Segment, unsigned int *ServerAddr, unsigned char *ServerAddrLen, unsigned char *ClientAddr,
            unsigned char *RecvNo, unsigned char *SendNo, bool *PorF, FRAME_COMMAND *FrameCommand,
	        unsigned int *InforOffset, unsigned int *InforLen, unsigned int *ActualFrameLen,
            const unsigned char *lpFrame, unsigned int FrameLen)
{

	Ctrl_Format *CtrlFormat;
	Frame_Format FrameFormat;
	unsigned short ServerAddrUpper, ServerAddrLower;
	*ActualFrameLen = FrameLen;

	if(0x7E != lpFrame[0] || 0x7E != lpFrame[FrameLen - 1])//opening flag or closing flag missing //*(lpFrame + FrameLen - 1)

	{
		return 101;
	}

	if(FrameLen < 9)//too short frame
	{
		return 102;
	}

	FrameFormat.Val = (lpFrame[1] << 8) + lpFrame[2];
	if((FrameFormat.bits.FrameLength + 2) != (signed int)FrameLen)//frame length dismatch
	{
		if((FrameFormat.bits.FrameLength + 2) < (signed int)FrameLen && 0x7E == *(lpFrame + FrameFormat.bits.FrameLength + 1))//more than one frames
		{
			*ActualFrameLen = FrameFormat.bits.FrameLength + 2;
		}
		else
		{
			return 103;
		}
	}
	
	if(0x0A != FrameFormat.bits.FrameType)//frame type sub-field error
	{
		return 104;
	}
	
	*Segment = FrameFormat.bits.Segment;
	
	if(pppfcs16(0xffff, lpFrame +  1, *ActualFrameLen - 4) != ((*(lpFrame + *ActualFrameLen - 3) << 8) + *(lpFrame + *ActualFrameLen - 2)))//fcs error
	{
		return 105;
	}

    if(0x01 == (*(lpFrame + 3) & 0x01))//client address
	{
		*ClientAddr = ((*(lpFrame + 3) >> 1) & 0x7F);
	}
	else
	{
		return 106;
	}	

	if(0x01 == (*(lpFrame + 4) & 0x01))//server address
	{
		*ServerAddrLen = 1;
		ServerAddrUpper = ((*(lpFrame + 4) >> 1) & 0x7F);
		ServerAddrLower = 0x00;
	}
	else if(0x01 == (*(lpFrame + 5) & 0x01))//
	{
		*ServerAddrLen = 2;
		ServerAddrUpper = ((*(lpFrame + 4) >> 1) & 0x7F);
		ServerAddrLower = ((*(lpFrame + 5) >> 1) & 0x7F);    
	}
	else if(0x01 == (*(lpFrame + 7) & 0x01))//
	{
		*ServerAddrLen = 4;
		ServerAddrUpper = (((*(lpFrame + 4) >> 1) & 0x7F) << 7) + ((*(lpFrame + 5) >> 1) & 0x7F);
		ServerAddrLower = (((*(lpFrame + 6) >> 1) & 0x7F) << 7) + ((*(lpFrame + 7) >> 1) & 0x7F);  
	}
	else
	{
		return 107;
	}
	  

	*InforOffset = 7 + *ServerAddrLen;
	if((8 + *ServerAddrLen) < (unsigned char)*ActualFrameLen)//information field existed
	{
		if(pppfcs16(0xffff, lpFrame + 1, 4 + *ServerAddrLen) != ((*(lpFrame + 5 + *ServerAddrLen) << 8) + *(lpFrame + 6 + *ServerAddrLen)))//fcs error
		{
			return 109;
		}

		*InforLen = *ActualFrameLen - 10 - *ServerAddrLen;
	}
	else
	{
		*InforLen = 0;
	}
	

    CtrlFormat = (Ctrl_Format *)(lpFrame + 4 + *ServerAddrLen);
	*PorF = (*CtrlFormat).bits.PorF;
	if(0x00 == (*CtrlFormat).bits.Res)
	{
		*RecvNo = (*CtrlFormat).bits.RRR;
		*SendNo = (*CtrlFormat).bits.SSS;
		*FrameCommand = I_FRAME;
	}
	else 
	{
		if(0x00 == (*CtrlFormat).bits.SSS)
		{			
			*RecvNo = (*CtrlFormat).bits.RRR;
			*SendNo = 0;
			*FrameCommand = RR_FRAME;
		}		
		else if(0x01 == (*CtrlFormat).bits.SSS)// SNRM, DISC, UA, UI
		{
			*RecvNo = 0;
			*SendNo = 0;
			if(0x04 == (*CtrlFormat).bits.RRR)
			{
				*FrameCommand = SNRM_FRAME;
			}
			else if(0x02 == (*CtrlFormat).bits.RRR)
			{
				*FrameCommand = DISC_FRAME;
			}
			else if(0x03 == (*CtrlFormat).bits.RRR)
			{
				*FrameCommand = UA_FRAME;
			}
			else if(0x00 == (*CtrlFormat).bits.RRR)
			{
				*FrameCommand = UI_FRAME;
			}
			else
			{
				return 110;
			}
		}
		else if(0x02 == (*CtrlFormat).bits.SSS)
		{			
			*RecvNo = (*CtrlFormat).bits.RRR;
			*SendNo = 0;
			*FrameCommand = RNR_FRAME;
		}
		else if(0x03 == (*CtrlFormat).bits.SSS && 0x04 == (*CtrlFormat).bits.RRR)// FRMR
		{
			*RecvNo = 0;
			*SendNo = 0;
			*FrameCommand = FRMR_FRAME;
		}
		else if(0x07 == (*CtrlFormat).bits.SSS && 0x00 == (*CtrlFormat).bits.RRR)// DM
		{
			*RecvNo = 0;
			*SendNo = 0;
			*FrameCommand = DM_FRAME;
		}
		else
		{
			return 110;
		}
	}


	if(I_FRAME != *FrameCommand && true == *Segment)
	{
		return 111;
	}

    if((I_FRAME == *FrameCommand || UI_FRAME == *FrameCommand) && InforLen == 0)
    {
        return 112;
    }

    //----------------
    if(CheckAddress(*PorF, *FrameCommand, *ServerAddrLen, ServerAddrUpper, &ServerAddrLower, *ClientAddr) == false)
	{
		return 108;
	}
	else
	{
		*ServerAddr = (ServerAddrUpper << 14) + ServerAddrLower;	
	}

    
	return 0;
}



/*************************************************
  Function:       CheckAddress
  Description:    验证接收帧中的地址是否合法
  Calls:          无
  Called By:      CheckFrame
  
  Input:          PF位,帧类型,服务器地址长度,服务器高地址,服务器低地址,客户端地址
  Output:         服务器低地址,如果服务器低地址为CALL ADDRESS,则转换为表本身的地址(暂未实现)
  Return:         TRUE--解析成功,FALSE--解析失败
  Others:         // 其它说明
*************************************************/
bool CheckAddress(bool PorF, FRAME_COMMAND FrameCommand, unsigned int ServerAddrLen,
                unsigned short ServerAddrUpper, unsigned short *ServerAddrLower, unsigned char ClientAddr)
{
	//
	if(0x00 == ClientAddr)
	{
		return false;//client address is NO_STATION address
	}
	
	if(0x7F == ClientAddr)//client address is ALL_STATION address
	{
		return false;
	}

	
	if(FrameCommand == I_FRAME || true == PorF)
	{
		switch(ServerAddrLen)
		{
			case 1:
			if(0x7F == ServerAddrUpper)
			{
				return false;
			}
			break;
			case 2:
			if(0x7F == ServerAddrUpper || 0x7F == *ServerAddrLower)
			{
				return false;
			}
			break;
			case 4:
			if(0x3FFF == ServerAddrUpper || 0x3FFF == *ServerAddrLower)
			{
				return false;
			}
			break;
			default:break;			
		}
		
		if(true == PorF)
		{
			if(0x00 == ServerAddrUpper || (ServerAddrLen > 1 && 0x00 == *ServerAddrLower))//server address is NO_STATION address
			{
				return false;
			}
		}		
	}
	
	
	if(ServerAddrUpper >= 0x02 && ServerAddrUpper <= 0x0F)
	{
		return false;
	}
	
	if(ServerAddrLen > 1 && *ServerAddrLower >= 0x01 && *ServerAddrLower <= 0x0F)
	{
		return false;
	}
	
	return true;
	
	
	/*
	//check the length of address 
	if(ServerAddrLen != MeterMacAddrLen)
	{
		switch(MeterMacAddrLen)
		{
			case 1:
			{
				if(ServerAddrLen == 2)
				{
					//The received message is not discarded if the received lower MAC Address is
					//equal to the ALL_STATION address only. In this case, the message shall be sent
					//to the Logical Device(s) designated by the upper MAC Address field
					if(0x7F != *ServerAddrLower)
					{
						return FALSE;
					}
				}
				else if(ServerAddrLen == 4)
				{
					//The received message is not discarded if the received lower and upper MAC
					//Addresses are both equal to the ALL_STATION address only.
					if(0x3FFF != *ServerAddrLower && 0x3FFF != ServerAddrUpper)
					{
						return FALSE;
					}
				}
				else
				{
					return FALSE;
				}
			}break;
			case 2:
			{			
				if(ServerAddrLen == 4)
				{
					//The received message may not be discarded if the received lower MAC Address
					//is equal to the ALL_STATION or to the CALLING physical device address only.
					//In the first case, the frame shall be accepted only if the upper MAC Address is
					//equal to the ALL_STATION address, in the second case the message shall be
					//taken into account only if the upper MAC Address is equal to the Management
					//Logical Device Address and the CALLING DEVICE layer parameter is set to
					//TRUE. In any other case, the received frame shall be discarded
					if(0x3FFF == *ServerAddrLower && 0x3FFF == ServerAddrUpper)
					{
					}
					else if(0x3FFE == *ServerAddrLower && 0x01 == ServerAddrUpper || TRUE == CallingDevice)
					{
						*ServerAddrLower = MeterMacAddr;
					}
					else 
					{
						return FALSE;
					}
				}
				else
				{
					return FALSE;
				}
			}break;
			case 4:
			{
				if(ServerAddrLen == 2)
				{
					//In this case, the value of the received one-byte lower and upper MAC addresses
					//shall be converted in the receiver into a two+two byte address, and the received
					//message shall be taken into account as if it was received in using a 4-byte
					//length Destination Address field
					//ServerAddrLen = 4?
				}
			
			}break;
			default:
			{
				return FALSE;
			}break;
		}//switch
	}//if
	else
	{
		switch(MeterMacAddrLen)
		{
			case 1:
			if(ServerAddrUpper == 0x7F || MeterMacAddr == ServerAddrUpper)
			{
				
			}
			else
			{
				return FALSE;
			}
			break;
			case 2:
			if(MeterMacAddr == ServerAddrLower || 0x7F == ServerAddrLower)
			{
			}
			else
			{
				return FALSE;
			}
			break;
			case 4:
			if(MeterMacAddr == ServerAddrLower || 0x7FFF == ServerAddrLower)
			{
			}
			else
			{
				return FALSE;
			}
			break;
			default:
			{
				return FALSE;
			}
		}
	}*/
	
	/*if(ServerAddrLen == 2)
	{
		if(0x7F != *ServerAddrLower && 0x7E != *ServerAddrLower && *ServerAddrLower != MeterMacAddr)
		{
			return FALSE;
		}
		else if(0x7E != *ServerAddrLower && 0x01 == ServerAddrUpper && TRUE == CallingDevice)
		{
			*ServerAddrLower = MeterMacAddr;	
		}	
	}
	else if(ServerAddrLen == 4)
	{
		if(0x7FFF != *ServerAddrLower && 0x7FFE != *ServerAddrLower && *ServerAddrLower != MeterMacAddr)
		{
			return FALSE;
		}
		else if(0x7FFE != *ServerAddrLower && 0x01 == ServerAddrUpper && TRUE == CallingDevice)
		{
			*ServerAddrLower = MeterMacAddr;	
		}		
	}*/
}

unsigned int CheckUdpFrame(/*unsigned char *ID, */unsigned short *ServerwPort, unsigned short *ClientwPort,
	        unsigned int *InforOffset, unsigned int *InforLen, unsigned int *ActualFrameLen,
            const unsigned char *lpFrame, unsigned int FrameLen)
{
	//unsigned short ServerAddrUpper, ServerAddrLower;
	*ActualFrameLen = FrameLen;

	//if(0x00 != lpFrame[8] || 0x01 != lpFrame[9])
    if(0x00 != lpFrame[0] || 0x01 != lpFrame[1])
	{
		return 240;
	}

    //memcpy(ID, lpFrame, sizeof(unsigned char) * ID_LEN);
    
    //*ServerwPort = (lpFrame[10] << 8) + lpFrame[11];
    *ServerwPort = (lpFrame[2] << 8) + lpFrame[3];
    //*ClientwPort = (lpFrame[12] << 8) + lpFrame[13];
    *ClientwPort = (lpFrame[4] << 8) + lpFrame[5];

    //*InforLen = (lpFrame[14] << 8) + lpFrame[15];
    *InforLen = (lpFrame[6] << 8) + lpFrame[7];
    *InforOffset = /*ID_LEN + */8;

	if(CheckwPort(*ClientwPort, ServerwPort) == false)
    {
        return 241;
    }
	return 0;
}

bool CheckwPort(unsigned short ClientwPort, unsigned short *ServerwPort)
{
    if(ClientwPort == 0x00)
        return false;

    if(*ServerwPort == 0x00 || (*ServerwPort >= 0x02 && *ServerwPort <= 0x0F))
        return false;
        
    return true;
}
