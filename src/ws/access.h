#ifndef ACCESS_H
#define ACCESS_H

//#include <list>
#include "ac_msg_handler.h"
#include "ac_msg.h"
#include "nopoll.h"
typedef enum 
{
    ACCESS_NETWORK_LOGIN_DONE = 1,
    ACCESS_NETWORK_LOGIN_FAILED = 2,
    ACCESS_NETWORK_CONNECT_FAILED = 3,
    ACCESS_NETWORK_HEARTBEAT_ERROR = 4,
    ACCESS_NETWORK_RELOGIN = 5,
    ACCESS_NETWORK_STOP = 6
} access_network_status_t;

typedef enum
{
    AS_NONE = 0,
    AS_CONNECT_FAILD,
    AS_DOING_LOGIN,
    AS_LOGIN_DONE,
    AS_LOGIN_FAILD,
    AS_WORKING,
    AS_NEED_STOP,
    AS_STOPED,
    AS_DISCONNECTED

} AccessWorkStatus;

typedef char* (*AccessMsgHandler)(const char* , int );
typedef int (*AccessNetworkCallback)(access_network_status_t);

#define ACCESS_MSG_MAX_SIZE  4096
class AccessServerHandler
{
 public:
   AccessServerHandler();
   ~AccessServerHandler();
   int  SetCallback(AccessMsgHandler msg_cb, AccessNetworkCallback network_cb);
   bool StartHandler(const char* servIp, int servPort, 
           const char* appVersion, const char* devId,
           const char* token, const char* aes_key,
           const char* aes_key_id);
   void StopHandler();
   bool IsAlive();
   bool IsStop();
   int  GetServerTimeStamp();
   int  GetTransInfo(int chlnum, int media_type, int usage);

 private:
   int  Send2AC(const char*, int);
   bool SendHeartBeat();
   bool LoginAccessServer();
   int  HandleMsg(const char* msg, int msgSize);
   int  SendMsg(const char* msg, int msg_size, ProtoEncodeType encode_type);
   bool ConnectServer(const char* ip, int port);
   int  DoLogin();
   void loop();

   static void* worker(void*);
   void worker_imp();
   void worker_imp2();
   void on_message ( noPollMsg * msg );
   void on_network_change(access_network_status_t s);
   int HandleRead(int ms);
   int aes_msg(const char** msg, int *msg_size, int mode);
   int EncryptMsg(const char** msg, int *msg_size);
   int DecryptMsg(const char** msg, int *msg_size);
   void CloseConnection();

 //member var
 private:
   AccessMsgHandler m_msg_cb;
   AccessNetworkCallback m_network_cb;

   char   m_servIp[32];
   int    m_servPort;
   char   m_token[64];
   char   m_appVersion[32];
   char   m_devId[32];
   char   m_aes_key[128];
   char   m_aes_key_id[128];
   int    m_heartBeartInterval;
   int    m_last_send_heartbeat_time;
   int    m_last_active_time;
   AccessWorkStatus m_workStatus;
   noPollCtx *   m_ctx;
   noPollConn *  m_conn;
   noPollMsg *   m_lastMsg;
   int   m_event_fd[2];
   int   m_server_timestamp;
};


#endif /* end of include guard: ACCESS_H */
