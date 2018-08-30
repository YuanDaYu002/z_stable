

#ifdef P2P_MODE

#include <stdio.h>
#include <unistd.h>
#include "zmutex.h"
#include "p2p_socket.h"
#include "plog.h"


#include "udt.h"

using namespace std;

bool p2p_select_write(UDTSOCKET sock, int timeout) {

  timeval tv;
  UDT::UDSET writefds;
  tv.tv_sec = 0;
  tv.tv_usec = timeout * 1000;

  UD_ZERO(&writefds);
  UD_SET(sock, &writefds);

  int res = UDT::select(0, NULL, &writefds, NULL, &tv);
  if ((res != UDT::ERROR) && (UD_ISSET(sock, &writefds)))
    return true;
  else
    return false;
}
//@timeout in milliseconds
bool p2p_select_read(UDTSOCKET sock, int timeout) {
  int64_t timeo;
  std::vector<UDTSOCKET> fds;
  std::vector<UDTSOCKET> readfds;
  std::vector<UDTSOCKET> excpfds;

  timeo = timeout;

  fds.push_back(sock);

  int res = UDT::selectEx(fds, &readfds, NULL, &excpfds, timeo);

  if (res == 0) {
    return false;
  } else if (res == UDT::ERROR) {
    plog("select error:%s\n", UDT::getlasterror().getErrorMessage());
    return false;
  }

  if (readfds.size() > 0 || excpfds.size() > 0)
    return true;
  return false;
}


P2PSocket::P2PSocket() : m_udtsock(-1) {
}
P2PSocket::~P2PSocket() {
}
void P2PSocket::SetHandle(int sock) {
  m_udtsock = sock;

  struct linger l;

  l.l_onoff = 1;
  l.l_linger = 0;
  if (0 != UDT::setsockopt(sock, 0, UDT_LINGER, &l, sizeof(l))) {
    plog("%s\n", "setsockopt linger error!!!!");
  }
}

void P2PSocket::Close() {
  if (m_udtsock > 0) {
    if (UDT::ERROR == UDT::close(m_udtsock)) {
      plog("P2PSocket close socket(%d) error!!!\n", m_udtsock);
    } else
      plog("P2PSocket %d %s\n", m_udtsock,   "closed");
    m_udtsock = -1;
  }
}
int  P2PSocket::GetHandle() {
  return m_udtsock;
}
bool P2PSocket::IfWritable(int millisec) {
  if (!IsAlive()) return false;
  return p2p_select_write(m_udtsock, millisec);
}
bool P2PSocket::IfReadable(int millisec) {
  if (!IsAlive()) return false;
  return p2p_select_read(m_udtsock, millisec);
}
int  P2PSocket::SendSome(const void* buffer, int buflen) {
  int ss;
  int ssize = 0;
  if (!IsAlive()) return -1;
  while (ssize < buflen) {
    if (UDT::ERROR == (ss = UDT::send(m_udtsock, (char*)buffer + ssize, buflen - ssize, 0))) {
      if (UDT::getlasterror().getErrorCode() == CUDTException::EASYNCSND)
        return ssize;
      plog("P2PSocket::SendSome:%s\n", UDT::getlasterror().getErrorMessage());
      return -1;
    }
    ssize += ss;
  }
  return ssize;
}
int  P2PSocket::SendData(const void* buffer, int buflen) {
  int ssize = 0;
  int ss;

  ZMutexLock l(&m_sndlock);

  while (ssize < buflen) {
    if (!IsAlive()) {
      plog("socket is not alive!!!\n");
      return -1;
    }
    if (UDT::ERROR == (ss = UDT::send(m_udtsock, (char*)buffer + ssize, buflen - ssize, 0))) {
      plog("P2PSocket::SendData:%s\n", UDT::getlasterror().getErrorMessage());
      return -1;
    } else if (ss == 0) {
      //normally , we would not come here untill UDT was something wrong,
      //so we wait 1 second, if not send may block, and we would recieve signal 6
      plog("sending timeout...\n");
      usleep(1000 * 1000);
    }
    ssize += ss;
  }
  return ssize;
}
void P2PSocket::SetBlockingSend(bool block) {
  UDT::setsockopt(m_udtsock, 0, UDT_SNDSYN, &block, sizeof(bool));
}
void P2PSocket::SetBlockingRecv(bool block) {
  UDT::setsockopt(m_udtsock, 0, UDT_RCVSYN, &block, sizeof(bool));
}
int P2PSocket::RecvSome(void* buffer, int len) {
  int rs;
  if (!IsAlive()) return -1;
  if (UDT::ERROR == (rs = UDT::recv(m_udtsock, (char*)buffer, len, 0))) {
    if (UDT::getlasterror().getErrorCode() == CUDTException::EASYNCRCV)
      return 0;
    plog("recv:%s\n", UDT::getlasterror().getErrorMessage());
    return -1;
  }
  return rs;
}
int P2PSocket::RecvN(void* buffer, int len) {
  int rsize = 0;
  int rs;
  while (rsize < len) {
    if (!IsAlive()) return -1;
    if (UDT::ERROR == (rs = UDT::recv(m_udtsock, (char*)buffer + rsize, len - rsize, 0))) {

      plog("recv:%s\n", UDT::getlasterror().getErrorMessage());
      return -1;
    }
    rsize += rs;
  }
  return rsize;
}
int  P2PSocket::Select(bool& read, bool& write, int ms) {
  std::set<UDTSOCKET> readfds;
  std::set<UDTSOCKET> writefds;

  if (!IsAlive())
    return -1;
  int eid = UDT::epoll_create();

  if (UDT::ERROR == UDT::epoll_add_usock(eid, m_udtsock)) {
    plog("epoll add error!\n");
    UDT::epoll_release(eid);
    return -1;
  }
  if (UDT::ERROR == UDT::epoll_wait(eid, &readfds, &writefds, ms)) {
    plog("epoll wait error!\n");
    UDT::epoll_remove_usock(eid, m_udtsock);
    UDT::epoll_release(eid);
    return -1;
  }
  UDT::epoll_remove_usock(eid, m_udtsock);
  UDT::epoll_release(eid);
  read = !readfds.empty();
  write = !writefds.empty();
  return 0;
}
int P2PSocket::RecvN(void* buffer, int size, int millisec) {
  int rsize = 0;
  int rs;
  while (rsize < size) {
    if (!IsAlive())
      return -1;
    if (UDT::ERROR == (rs = UDT::recv(m_udtsock, (char*)buffer + rsize, size - rsize, 0))) {

      plog("recv:%s\n", UDT::getlasterror().getErrorMessage());
      return -1;
    }
    rsize += rs;
    if (rsize < size && !IfReadable(1000)) {
      plog("timeout!!!\n");
      return -1;
    }
  }
  return rsize;
}

bool P2PSocket::IsAlive() {
  if (m_udtsock < 0 || UDT::getsockstate(m_udtsock) != CONNECTED) {
    return false;
  }
  return true;
}

#endif // P2P_MODE