//---------------------------------------------------------------------------
#include "Class_StackList.h"
//---------------------------------------------------------------------------
#define MAX_PROTOCOLSTACK_COUNT 32
//-------------------------------------------
StackList::StackList()
{
    svec.reserve(MAX_PROTOCOLSTACK_COUNT);  //同时存在的最大协议栈数量
    svec.clear(); 
};

StackList::~StackList()
{
    if(svec.empty())    return;  
    
    std::vector<CStack62056 *>::iterator iter = svec.begin();
    std::vector<CStack62056 *>::iterator iter_end = svec.end();

    for (; iter != iter_end; ++iter)
    {
        delete (*iter);
    }
    svec.clear();
};

//bool StackList::Add(CStack62056 *Stack)
CStack62056 * StackList::Add(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr)
{
    std::vector<CStack62056 *>::iterator iter = svec.begin();
    std::vector<CStack62056 *>::iterator iter_end = svec.end();

    if(svec.begin() == svec.end())
    {
        CStack62056 *Stack = new CStack62056(SupportLayerType, ID, Addr);
        if(Stack == NULL)
            return NULL;

        //svec.push_back(Stack);
        svec.insert(svec.begin(), Stack);
        return Stack;
    }
    
    for (; iter != iter_end; ++iter)
    {
        if((*iter) != NULL)
        {
            if((*iter)->GetStackUsageFlag() == false)
            {
                (*iter)->Reset(SupportLayerType, ID, Addr);
                return (*iter);
            }
        }
        else if((*iter) == NULL)
        {
            CStack62056 *Stack = new CStack62056(SupportLayerType, ID, Addr);
            if(Stack == NULL)
                return NULL;

            //svec.push_back(Stack);
            svec.insert(svec.begin(), Stack);
            return Stack;
        }
        else
        {
            continue;
        }
    }

    //新增一个
    CStack62056 *Stack = new CStack62056(SupportLayerType, ID, Addr);
    if(Stack == NULL)
        return NULL;

    //svec.push_back(Stack);
    svec.insert(svec.end(), Stack);
    return Stack;
};

//bool StackList::Delete(CStack62056 *Stack)
bool StackList::Delete(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr)
{
    std::vector<CStack62056 *>::iterator iter = svec.begin();
    std::vector<CStack62056 *>::iterator iter_end = svec.end();

    for (; iter != iter_end; ++iter)
    {
        SUPPORT_LAYER tmpSupportLayerType = (*iter)->GetSupportLayerType();
        ADDR tmpAddr = (*iter)->GetAddr();
        unsigned char *tmpID = (*iter)->GetID();

        if(tmpSupportLayerType == SupportLayerType && memcmp(tmpID, ID, sizeof(unsigned char) * ID_LEN) == 0 && memcmp(&tmpAddr, &Addr, sizeof(ADDR)) == 0)
        {
            delete (*iter);
            svec.erase(iter);
            return true;
        }
    }
    return false;
};

CStack62056 * StackList::Find(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr)
{
    std::vector<CStack62056 *>::iterator iter = svec.begin();
    std::vector<CStack62056 *>::iterator iter_end = svec.end();

    for (; iter != iter_end; ++iter)
    {
        SUPPORT_LAYER tmpSupportLayerType = (*iter)->GetSupportLayerType();
        ADDR tmpAddr = (*iter)->GetAddr();
        unsigned char *tmpID = (*iter)->GetID(); 

        if(tmpSupportLayerType == SupportLayerType)
        {
            if(memcmp(tmpID, ID, sizeof(unsigned char) * ID_LEN) == 0)
            { 
                switch(SupportLayerType)
                {
                    case HDLC:
                    {
                        if(tmpAddr.HDLC_ADDR2.ServerAddr == Addr.HDLC_ADDR2.ServerAddr
                        && tmpAddr.HDLC_ADDR2.ClientAddr == Addr.HDLC_ADDR2.ClientAddr
                        && tmpAddr.HDLC_ADDR2.ServerAddrLen == Addr.HDLC_ADDR2.ServerAddrLen)
                        {
                            return (*iter);
                        }
                    }break;
                    case UDPIP:
                    {
                        if(tmpAddr.UDPIP_ADDR.Server_wPort == Addr.UDPIP_ADDR.Server_wPort
                        && tmpAddr.UDPIP_ADDR.Client_wPort == Addr.UDPIP_ADDR.Client_wPort
                        /*&& memcmp(tmpAddr.UDPIP_ADDR.ID, Addr.UDPIP_ADDR.ID, sizeof(unsigned char) * ID_LEN) == 0*/)
                        {
                            return (*iter);
                        }
                    }break;   //modified by slx 2012-9-14
                    default:break;
                }
            }

        }
    }
    return NULL;
};