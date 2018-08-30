#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
/* According to POSIX.1-2001 */
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <stddef.h>

#include "tcp_socket.h"
#include "plog.h"

TcpSocket::TcpSocket() : m_sock(-1) 
{
}
TcpSocket::~TcpSocket() 
{
}
int  TcpSocket::GetHandle() 
{
    return m_sock;
}
void TcpSocket::SetHandle(int sock) 
{
    m_sock = sock;
}
bool TcpSocket::IfReadable(int ms) 
{
    fd_set readfds;

    FD_ZERO(&readfds);

    FD_SET(m_sock, &readfds);
    int maxfd = m_sock + 1;

    timeval to;
    to.tv_sec = ms / 1000;
    to.tv_usec = (ms % 1000) * 1000;

    int n = select(maxfd, &readfds, NULL, NULL, &to);

    if (n > 0 && FD_ISSET(m_sock, &readfds))
        return true;
    else
        return false;
}
bool TcpSocket::IfWritable(int ms) 
{
    fd_set writefds;

    FD_ZERO(&writefds);

    FD_SET(m_sock, &writefds);
    int maxfd = m_sock + 1;

    timeval to;
    to.tv_sec = ms / 1000;
    to.tv_usec = (ms % 1000) * 1000;

    int n = select(maxfd, NULL, &writefds, NULL, &to);

    if (n > 0 && FD_ISSET(m_sock, &writefds))
        return true;
    else
        return false;
}
int  TcpSocket::Read(void* data, int len) 
{
    return recv(m_sock, data, len, 0);
}

int TcpSocket::ReadN(void* buffer, int len) 
{
    int retLen = 0;
    int total = 0;

    while (total < len) 
    {
        retLen = recv(m_sock, (char*)buffer + total, len - total, 0);
        if (retLen <= 0) 
        {
            return retLen;
        }
        total += retLen;
    }
    return total;
}
int TcpSocket::WriteN(const void* buffer, int len) 
{
    int iRet;
    int total = 0;

    do 
    {
        iRet = send(m_sock, buffer+total, len-total, 0);
        if (iRet < 1) 
        {
            return -1;
        }
        total += iRet;
        len -= iRet;
    }while (len > 0);
    return total;
}
int TcpSocket::Write(const void* buffer, int len) 
{
    int iRet;

    iRet = send(m_sock, buffer, len, 0);

    if (iRet == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
        return 0;

    return iRet;
}
void TcpSocket::Close() 
{
    if(m_sock > 0) 
        close(m_sock);
    m_sock = -1;
}

int  TcpSocket::Select(bool& read, bool& write, int ms) 
{
    fd_set writefds, readfds;

    FD_ZERO(&writefds);
    FD_ZERO(&readfds);

    FD_SET(m_sock, &writefds);
    FD_SET(m_sock, &readfds);

    timeval to;
    to.tv_sec = 0;
    to.tv_usec = ms * 1000;

    read = write = false;

    if (select(m_sock + 1, &readfds, &writefds, NULL, &to) > 0) 
    {
        write = FD_ISSET(m_sock, &writefds);
        read  = FD_ISSET(m_sock, &readfds);
    }
    return 0;
}

int  TcpSocket::SetBlocking(bool block) 
{
    int flags = fcntl(m_sock, F_GETFL, 0);
    if (!block)
        fcntl(m_sock, F_SETFL, flags | O_NONBLOCK);
    else
        fcntl(m_sock, F_SETFL, flags & ~O_NONBLOCK);
    return 0;
}

int  TcpSocket::ConnectPeer(const char* ip, int port, int timeout_ms) 
{
    sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock < 0)
        return -1;

    SetBlocking(false);
    if (connect(m_sock, (sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        if (errno == EINPROGRESS && IfWritable(timeout_ms)) 
        {
            int error = -1;
            socklen_t len = sizeof(error);
            if (getsockopt(m_sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) 
            {
                SetBlocking(true);
                return 0;
            }
            close(m_sock);
            return -1;
        }
        close(m_sock);
        return -1;
    }
    SetBlocking(true);
    return 0;
}
int  TcpSocket::ConnectPeerUN(const char* unix_path, int timeout_ms) 
{
    struct sockaddr_un un;
    int size;

    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, unix_path);

    if ((m_sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
    {
        printf("socket failed!\n");
        return -1;
    }
    size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    SetBlocking(false);
    if (connect(m_sock, (sockaddr*)&un, size) < 0) 
    {
        if (errno == EINPROGRESS && IfWritable(timeout_ms)) 
        {
            int error = -1;
            socklen_t len = sizeof(error);
            if (getsockopt(m_sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) 
            {
                SetBlocking(true);
                return 0;
            }
            close(m_sock);
            return -1;
        }
        close(m_sock);
        return -1;
    }
    SetBlocking(true);
    return 0;
}

int  TcpSocket::ConnectPeerAsync(const char* ip, int port) 
{
    sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock < 0)
        return -1;

    SetBlocking(false);
    if (connect(m_sock, (sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        if (errno != EINPROGRESS) 
        {
            return -1;
            close(m_sock);
        }
        return 0;
    }
    return 1;
}
int TcpSocket::CheckAsyncConnection() 
{
    int error = -1;

    if (IfWritable(0)) 
    {
        socklen_t len = sizeof(error);
        if (getsockopt(m_sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) 
        {
            return 1;
        }
        close(m_sock);
        return -1;
    }
    return 0;
}
int  TcpSocket::GetDataSizeInReadBuffer() 
{
    int bytes = 0;
    if (ioctl(m_sock, FIONREAD, &bytes))
        return 0;
    else
        return bytes;
}

int  TcpSocket::Htonl(int val)
{
    return htonl(val);
}

int  TcpSocket::Htons(short val)
{
    return htons(val);
}

int  TcpSocket::Ntohl(int val)
{
    return ntohl(val);
}

int  TcpSocket::Ntohs(short val)
{
    return ntohs(val);
}
