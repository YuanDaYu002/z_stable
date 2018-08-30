#ifndef P2PSOCKET_H
#define P2PSOCKET_H

#ifdef P2P_MODE

#include "zmd_socket.h"

#include "zmutex.h"
#include "udt.h"

class P2PSocket : public ZmdSocket {
 public:
  P2PSocket();
  virtual ~P2PSocket();

  void SetHandle(int sock);
  int  GetHandle();
  bool IfWritable(int millisec);
  bool IfReadable(int millisec);
  //send some data, for none blocking use
  int SendSome(const void* buffer, int buflen);
  int SendData(const void* buffer, int buflen);
  int SendCmdData(const void* buffer, int buflen);
  //recv some data, for none blocking use
  int RecvSome(void* buffer, int len);
  int RecvN(void* buffer, int len);
  int RecvN(void* buffer, int len, int millisec);
  void SetBlockingSend(bool block);
  void SetBlockingRecv(bool block);
  virtual void Close();
  //for ZmdSocket
  int Read(void* buffer, int len) { return RecvSome(buffer, len);}
  int ReadN(void* buffer, int len) { return RecvN(buffer, len);}
  int WriteN(const void* buffer, int len) { return SendData(buffer, len);}
  int Write(const void* buffer, int len) { return SendSome(buffer, len);}
  int SetBlocking(bool blocking) { SetBlockingSend(blocking);
    SetBlockingRecv(blocking);
    return 0;}
  int  Select(bool& read, bool& write, int ms);
  bool IsAlive();
 protected:
  int  m_udtsock;
  ZMutex m_sndlock;
  int  m_eid; //epoll id
};
bool p2p_select_read(UDTSOCKET sock, int timeout);

bool p2p_select_write(UDTSOCKET sock, int timeout);
#endif // P2P_MODE

#endif /* end of include guard: P2PSOCKET_H */
