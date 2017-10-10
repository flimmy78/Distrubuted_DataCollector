#pragma once

#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <atomic>

#define MAKE_SOCKADDR_IN(var,adr,prt) \
    struct sockaddr_in var;\
    var.sin_family = AF_INET;\
    var.sin_addr.s_addr = (adr);\
    var.sin_port = (prt);\


#include "../include/common.h"
using namespace WSCOMMON;
namespace WSNET
{
	class CTCP
	{
		public:

			CTCP();
			virtual ~CTCP();

			// �����Ҫ���ʼ��
			int InitializeWinsockIfNecessary();

			// ����TCP socket����IP��ַ�Ͷ˿ڣ��˿ڿ���Ϊ0����ʾ��ϵͳ������䣩
			int SetupStreamSocket(const char* szIPAddress, unsigned int dwPort = 0);

			// �ر�socket���ɱ����رպ����Źر����ַ�ʽ��
			int CloseSocket(bool graceful);

			// ��ȡ���ͺͽ��ջ������Ĵ�С
			unsigned int GetSendBufferSize();
			unsigned int GetReceiveBufferSize();

			// ���÷��ͺͽ��ջ������Ĵ�С
			unsigned int SetSendBufferTo(unsigned int requestedSize);
			unsigned int SetReceiveBufferTo(unsigned int requestedSize);

			// ����SOCKETΪ�������߷�����ģʽ
			bool MakeSocketNonBlocking();
			bool MakeSocketBlocking();

			// ���к���OFFʱ���ر�SOCKET�������WAIT_TIME�����ڱ����ر� 
			bool MakeSocketLingerON(int socket);
			bool MakeSocketLingerOFF(int socket);

			int SendContent(unsigned char cCommand, const char* szSendBuf, 
				unsigned int nSendLen, unsigned int& nSendID);

			int RecvContent(unsigned char& cCommand, char* szRecvBuf,
				unsigned int& nRecvLen, unsigned int& nRecvID);

		protected:
			/*TCP�����ʽ
			-------------------------------------------------------------------------------
			| 4B(HEADER) | 1B(COMMAND) | 4B(SEQ) | 2B(LENGTH) | nB(DATA) | 1 (END) |
			-------------------------------------------------------------------------------*/
			// У��TCP���ݰ��Ƿ���Ϸ����ʽ���������ݰ���
			int CheckTCPHeader(const unsigned char* pTCPPacket);

			// ͬ��TCP���ݰ���ͷ���������ݰ�����
			int SyncTCPHeader(unsigned char* pTCPPacket);

			// ����TCP���ݣ�readSocketExact����ָ�����ȵ����ݺ�ŷ��أ�
			int ReadSocket(unsigned char* buffer, unsigned int bufferSize,
				struct timeval* timeout = NULL);
			int ReadSocketExact(unsigned char* buffer, unsigned int bufferSize,
				struct timeval* timeout = NULL);

			// ����TCP���ݣ�WriteSocketExact����ָ�����ȵ����ݺ�ŷ��أ�
			int WriteSocketExact(unsigned char* buffer, unsigned int bufferSize,
				struct timeval* timeout = NULL);
			int WriteSocket(unsigned char* buffer, unsigned int bufferSize,
				struct timeval* timeout = NULL);

			// ���÷��ͺͽ��ջ������Ĵ�С
			unsigned int GetBufferSize(int bufOptName);
			unsigned int SetBufferTo(int bufOptName, unsigned int requestedSize);

		protected:
			SOCKET m_sock = INVALID_SOCKET;
			bool m_bExit = false;
			std::atomic_uint m_nSeq = 0;
#ifdef WIN32
			CRITICAL_SECTION m_csTCPSend;
			CRITICAL_SECTION m_csTCPRecv;
#endif
	};

	class CClientTCP : public CTCP
	{
	public:

		CClientTCP();
		virtual ~CClientTCP();

		// ���ӷ�������0Ϊ�ɹ���-1ʧ��
		int connectServer(const char* szServer, unsigned int nPort);
	};
}