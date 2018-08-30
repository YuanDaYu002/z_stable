#ifndef MEDIACLIENT_H
#define MEDIACLIENT_H

#include <arpa/inet.h>

#include "msg_handler.h"
#include "video_client.h"
#include "playback_client.h"
#include "device_operation.h"
#include "zmd_socket.h"
#include "simple_buffer.h"

typedef enum 
{
    CLIENT_STATUS_STOP,
    CLIENT_STATUS_COMMON,
    CLIENT_STATUS_VIDEO,
    CLIENT_STATUS_PLAYBACK
} MEDIA_CLIENT_STATUS;

typedef enum
{
	CLIENT_WORK_P2P = 0,
	CLIENT_WORK_TRAN
}CLIENT_WORK_MODE;
struct RcvBuffer
{
    RcvBuffer()
    {
        size = sizeof(buf);
        rptr = wptr = buf;
        end  = buf + size;
    }
    ~RcvBuffer(){}
    char   buf[4096];
    char*  rptr;//read ptr
    char*  wptr;//write ptr
    char*  end;
    int    size;
};
//for p2p and upnp use
class ClientBase
{
 public:
   ClientBase(ZmdSocket& sock, CLIENT_WORK_MODE mode) ;
   virtual ~ClientBase();

   int   SendReply(void* data, int len) ;
   int	 HandleCmdImp(trans_msg_s* head, void* msg_body, int msg_size);
   void  work_imp();
   int   HandleRead();
   int   HandleWrite();
   int   HandleCmd();
   int   HandleVideo(trans_msg_s* head, void* body, int body_len);
   int   HandlePlayback(trans_msg_s* head, void* body, int body_len) ;
   int   OnIdle(long cur_sec);
   void  OnClose();

   MEDIA_CLIENT_STATUS  m_status;
   CLIENT_WORK_MODE 	m_mode;
   VideoClient          m_video;
   PlaybackClient       m_playback;
   ZmdSocket&	 		m_sock;
   PrivateMsgHandler	m_msg_handler;
   CSimpleBuffer        m_send_buffer;
   CSimpleBuffer        m_recv_buffer;
   time_t				m_last_active_time;
   time_t				m_last_heartbeat_time;
 private:
 	ClientBase();
	ClientBase(const ClientBase&);
	ClientBase& operator = (const ClientBase&);
};

#endif /* end of include guard:  */
