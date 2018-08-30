#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include "zmd_socket.h"

class TcpSocket: public ZmdSocket
{
 public:
   TcpSocket();
   ~TcpSocket();

   int  GetHandle();
   void SetHandle(int sock);
   bool IfReadable(int ms);
   bool IfWritable(int ms);
   int  Read(void* data, int len);
   int  ReadN(void* data, int len);
   int  WriteN(const void* data, int len);
   int  Write(const void* data, int len);
   void Close();
   int  Select(bool &read, bool &write, int ms);
   int	SetBlocking(bool block);
   int  ConnectPeer(const char* ip, int port, int timeout_ms=20*1000);
   int  ConnectPeerUN(const char* unix_path, int timeout_ms=20*1000);

    /**
     * @brief �첽���ӶԶˣ���Ҫ���  CheckAsyncConnection ʹ��
     * 
     * @author AlbertXiao (2015/6/1)
     * 
     * @param ip �Զ�ip
     * @param port �Զ� port 
     * 
     * @return int 0 ��ʾ�ɹ�����������δ��ɣ�1 
     *         ��ʾ������ɣ�<0��ʾʧ��
     */
   int  ConnectPeerAsync(const char* ip, int port);

    /**
     * @brief ����첽�����Ƿ����
     * 
     * @author AlbertXiao (2015/6/1)
     * 
     * @return int 0 ��ʾû����ɣ���Ҫ������飻1 
     *         ��ʾ���ӳɹ���<0��ʾ����
     */
   int  CheckAsyncConnection();

   int  GetDataSizeInReadBuffer();

   int  Htonl(int val);
   
   int  Htons(short val);

   int  Ntohl(int val);
   
   int  Ntohs(short val);

 protected:
   int m_sock;
};


#endif /* end of include guard: TCPSOCKET_H */
