#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

#include "zmd_msg.h"
#include "zmd_socket.h"

class PrivateMsgHandler;
int handle_common_msg(trans_msg_s* head, void* data, int len, PrivateMsgHandler* sender);

class PrivateMsgHandler
{
 public:
   int SendReply(void* data, int len){return m_sock.WriteN(data, len);}

   int HandleMsg(trans_msg_s* head, void* data, int len);

   PrivateMsgHandler(ZmdSocket& sock):m_sock(sock)
   {
		m_is_set_ircut = false;
		m_is_hold_mic = false;
		m_last_talk_heartbeat = 0L;
   }
   virtual ~PrivateMsgHandler();
 private:
   PrivateMsgHandler();
   
   ZmdSocket& m_sock;
 public:
   bool   m_is_set_ircut;
   bool   m_is_hold_mic;
   time_t m_last_talk_heartbeat;
};


#endif /* end of include guard: MSG_HANDLER_H */
