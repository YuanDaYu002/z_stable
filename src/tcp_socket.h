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
     * @brief 异步连接对端，需要配合  CheckAsyncConnection 使用
     * 
     * @author AlbertXiao (2015/6/1)
     * 
     * @param ip 对端ip
     * @param port 对端 port 
     * 
     * @return int 0 表示成功，但是连接未完成，1 
     *         表示连接完成，<0表示失败
     */
   int  ConnectPeerAsync(const char* ip, int port);

    /**
     * @brief 检查异步连接是否完成
     * 
     * @author AlbertXiao (2015/6/1)
     * 
     * @return int 0 表示没有完成，需要继续检查；1 
     *         表示连接成功；<0表示错误
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
