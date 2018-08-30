#include <nopoll.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
/* According to POSIX.1-2001 */
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "p2p_def.h"
#include "ac_msg_handler.h"
#include "ac_msg.h"
#include "access.h"
#include "plog.h"
#include "helpfunction.h"
#include "device_operation.h"

AccessServerHandler::AccessServerHandler():  m_msg_cb(NULL), m_network_cb(NULL), m_heartBeartInterval(30), 
    m_workStatus(AS_NONE), m_conn(NULL), m_lastMsg(0),
    m_server_timestamp(0)
{
    memset(m_token, '\0', sizeof(m_token));
    memset(m_aes_key, '\0', sizeof(m_aes_key));
    memset(m_aes_key_id, '\0', sizeof(m_aes_key_id));
    socketpair(AF_UNIX, SOCK_DGRAM, 0, m_event_fd); 
}

AccessServerHandler::~AccessServerHandler()
{
}

int  AccessServerHandler::SendMsg(const char* msg, int msg_size, ProtoEncodeType encode_type)
{
    if(!nopoll_conn_is_ready(m_conn))
        return 0;
    if(encode_type == PE_BINARY || encode_type == PE_PROTOBUF) 
        nopoll_conn_send_binary(m_conn, msg, msg_size);
    else 
        nopoll_conn_send_text(m_conn, msg, msg_size);
    return msg_size;
}

int  AccessServerHandler::HandleMsg(const char* msg, int msgSize)
{
    JsonParser jp(msg);

    m_last_active_time = p2p_get_uptime();
    plog_str(msg);

    int type = -1;

    jp.GetIntValue(type, "type");

    if(type == AC_MSG_SHAKE)
    {
        //login response, we will send ack
        int code = -1;
        jp.GetIntValue(code, "code");
        if(code == 200)
        {
            plog("login success!\n");
            m_heartBeartInterval = jp.GetIntValue("heartbeat");
            plog("get heart beat interval:%d\n", m_heartBeartInterval);
            if(m_heartBeartInterval > 90 || m_heartBeartInterval <= 0)
            {
                plog("%s\n", "get heartbeat interval error, use default 30s");
                m_heartBeartInterval = 30;
            }
            m_workStatus = AS_WORKING;
            int ts = -1;
            if(jp.GetIntValue(ts, "timestamp") > 0)
                m_server_timestamp = ts - p2p_get_uptime();
            on_network_change(ACCESS_NETWORK_LOGIN_DONE);
            //reset heartbeat
            m_last_send_heartbeat_time = 0;
        }
        else
        {
            plog("login failed code:%d\n", code);
            m_workStatus = AS_LOGIN_FAILD;
            on_network_change(ACCESS_NETWORK_LOGIN_FAILED);
        }
    }
    else if( type == AC_MSG_HEART_BEAT)
    {
        m_workStatus = AS_WORKING;
        plog("%s\n", "recv heart beat from access");
    }
    else if( type == AC_MSG_DATA || type == AC_MSG_REDIRECT)
    {
        if(!m_msg_cb)
            plog("access msg handler is NULL!\n");
        char* resp = m_msg_cb(msg, msgSize);
        if(resp)
        {
            Send2AC(resp, strlen(resp));
            plog_str(resp);
            free(resp);
        }
    }
    return 0;
}

void AccessServerHandler::on_message ( noPollMsg * new_msg )
{
    //plog("is fragment :%d\n", nopoll_msg_is_fragment (msg));
    //plogd("is final :%d\n", nopoll_msg_is_final(msg));
    // reply to the message
    noPollMsg *aux = m_lastMsg;
    m_lastMsg = nopoll_msg_join (m_lastMsg, new_msg);
    nopoll_msg_unref (aux);

    if(nopoll_msg_is_final(new_msg))
    {
        const char* msg = (char*)nopoll_msg_get_payload(m_lastMsg);
        int msg_size = nopoll_msg_get_payload_size(m_lastMsg);
        DecryptMsg(&msg, &msg_size);
        HandleMsg(msg, msg_size);
        free((void*)msg);

        nopoll_msg_unref(m_lastMsg);
        m_lastMsg = NULL;
    }
    nopoll_msg_unref(new_msg);
}
int AccessServerHandler::DoLogin()
{	
    m_workStatus = AS_DOING_LOGIN;

    if(!p2p_wifi_connected())
    {
        plog("wifi disconnected!\n");
        m_workStatus = AS_DISCONNECTED;
        return -1;
    }

    if(ConnectServer(m_servIp, m_servPort))
    {
        LoginAccessServer();
    }
    else
    {
        return -1;
    }
    for(int i=0;i<10;i++)
    {			
        if(nopoll_conn_pending_write_bytes(m_conn))
            nopoll_conn_flush_writes(m_conn, 1000*1000, 0);

        noPollMsg* msg = nopoll_conn_get_msg(m_conn);

        if(msg) on_message(msg);

        if(m_workStatus == AS_WORKING)
        {
            return 0;
        }
        else if(m_workStatus == AS_LOGIN_FAILD)
        {
            return -1;
        }
        else if(m_workStatus != AS_DOING_LOGIN)
        {
            return -1;
        }
        else
        {
            plog("wait for login ack...\n");
        }
        usleep(500*1000);
    }
    plog("wait for login ack timeout!\n");
    return -1;
}
void AccessServerHandler::loop()
{			
    m_last_send_heartbeat_time = 0;
    m_last_active_time = p2p_get_uptime();

    while(m_workStatus == AS_WORKING)
    {
        if(!p2p_wifi_connected())
        {
            plog("wifi disconnected!\n");
            m_workStatus = AS_DISCONNECTED;
            break;
        }
        if(nopoll_conn_is_ok(m_conn))
        {			 
            if(nopoll_conn_pending_write_bytes(m_conn))
                nopoll_conn_flush_writes(m_conn, 1000*1000, 0);

            HandleRead(5000);
        }
        else
        {
            plog("connection lost!\n");
            break;
        }

        int now = p2p_get_uptime();

        // check after 15 seconds 
        if(now - m_last_send_heartbeat_time > 15 &&
                m_last_send_heartbeat_time > m_last_active_time)
        {
            plog("recv keep alive timeout!\n");
            on_network_change(ACCESS_NETWORK_HEARTBEAT_ERROR);
            m_workStatus = AS_DISCONNECTED;
            break;
        }

        if(now - m_last_send_heartbeat_time >= m_heartBeartInterval)
        {
            SendHeartBeat();
            m_last_send_heartbeat_time = now;
        }
    }
}
void AccessServerHandler::worker_imp2()
{
    plogfn();

    m_ctx = nopoll_ctx_new ();
    //  nopoll_log_enable(m_ctx, 1);

    if(0 != DoLogin())
    {
        plog("login failed!\n");
    }
    else
    {
        loop();
    }

    CloseConnection();

    nopoll_ctx_unref(m_ctx);
    m_ctx = NULL;	
    m_workStatus = AS_STOPED;
    plog("AccessServerHandler stoped\n");	
    on_network_change(ACCESS_NETWORK_RELOGIN);
}

bool AccessServerHandler::StartHandler( const char* serv_ip, int serv_port,
        const char* appVersion, const char* devId,
        const char* tokenid, const char* aes_key,
        const char* aes_key_id)
{
    assert(serv_ip != NULL);

    plog("serv_ip:%s\n", serv_ip);
    plog("serv_port:%d\n", serv_port);

    strcpy(m_servIp, serv_ip);
    m_servPort = serv_port;

    strcpy(m_appVersion, appVersion);
    strcpy(m_devId, devId);
    strcpy(m_token, tokenid);
    strcpy(m_aes_key, aes_key);
    strcpy(m_aes_key_id, aes_key_id);

    m_workStatus = AS_NONE;
    pthread_t thr;

    pthread_create(&thr, NULL, worker, this);

    return true;
}
void AccessServerHandler::StopHandler()
{
    if(m_workStatus != AS_STOPED)
        m_workStatus = AS_NEED_STOP;
    
    char* cmd = NULL;
    /* interrupt HandleRead */
    write(m_event_fd[1], &cmd, sizeof(char*));
}
void* AccessServerHandler::worker(void* para)
{
    printf("%s %d %s tid:[%d] pid:[%d] ppid:[%d]\n", __FILE__,__LINE__,__FUNCTION__,(int)pthread_self(),(int)getpid(),(int)getppid());
    pthread_detach(pthread_self());
    AccessServerHandler *pthis = (AccessServerHandler*)para;
    pthis->worker_imp2();

    return NULL;
}
bool AccessServerHandler::LoginAccessServer()
{
    JsonBuilder enc;

    enc.AddInt( "type", AC_MSG_SHAKE);
    enc.AddInt( "length", sizeof(ac_login_body));
    enc.AddString( "cmd_version", "1.0.0.0");
    enc.AddInt( "data_type", 1);
    enc.AddString( "token_id", m_token);
    enc.AddInt("client_type", 0);
    enc.AddString("client_version", m_appVersion );
    enc.AddString("client_id", m_devId);

    plog("msg:%s\n", enc.GetJson());

    const char *msg = enc.GetJson();
    int   msg_size = enc.GetSize();
    if(strlen(m_aes_key))
    {
        EncryptMsg(&msg, &msg_size);

        int send_bufsiz = 4 + strlen(m_aes_key_id) + msg_size; 

        plog_int(send_bufsiz);

        char* send_buf = (char*)malloc(send_bufsiz);

        *(short*)send_buf = htons(strlen(m_aes_key_id)+4);
        *(short*)(send_buf+2) = htons(strlen(m_aes_key_id));
        strcpy(send_buf+4, m_aes_key_id);
        memcpy(send_buf+4+strlen(m_aes_key_id), msg, msg_size);
        free((void*)msg);
        SendMsg(send_buf, send_bufsiz, PROTOTYPE);
        free(send_buf);
    }
    else 
    {
        SendMsg(msg, msg_size, PROTOTYPE);
    }
    return true;
}

bool AccessServerHandler::SendHeartBeat()
{
    JsonBuilder enc;

    enc.AddInt( "type", AC_MSG_HEART_BEAT);
    enc.AddInt( "length" ,0);

    Send2AC(enc.GetJson(), enc.GetSize());
    plog("%s\n", "heart beat send!");
    return true;
}

bool AccessServerHandler::IsAlive()
{
    return m_workStatus == AS_WORKING;
}

bool AccessServerHandler::IsStop()
{
    return m_workStatus == AS_STOPED;
}

bool AccessServerHandler::ConnectServer(const char* ip, int port)
{
    char str_port[32];

    sprintf( str_port, "%d", m_servPort);
    noPollConn * conn = nopoll_conn_new (m_ctx, ip, str_port, NULL, "/ws", NULL, NULL);
    if (! nopoll_conn_is_ok (conn)) 
    {
        // some error handling here
        plog("connect to %s:%s error!\n", ip, str_port);
        nopoll_conn_close(conn);
        return false;
    }
    if(! nopoll_conn_wait_until_connection_ready (conn, 7)) 
    {
        // some error handling
        plog("nopoll connect error\n");
        nopoll_conn_close(conn);
        return false;
    }
    nopoll_conn_close(m_conn);
    m_conn = conn;

    plog("connected with access\n");
    return true;
}
int  AccessServerHandler::aes_msg(const char** msg, int *msg_size, int mode)
{
    if(strlen(m_aes_key))
    {
        int out_bufsiz = *msg_size + 16 - *msg_size%16;

        char* out_buf = (char*)calloc(1, out_bufsiz+1);

        p2p_aes_buffer(m_aes_key, *msg, *msg_size,
                out_buf, &out_bufsiz, mode);
        // plog_int(out_bufsiz);
        *msg = out_buf;
        *msg_size = out_bufsiz;
    }
    /* for compat*/
    else
    {
        char* tmp = (char*)malloc(*msg_size+1);
        memset(tmp, '\0', *msg_size+1);
        memcpy(tmp, *msg, *msg_size);
        *msg = tmp;
    }
    return 0;

}
int  AccessServerHandler::EncryptMsg(const char** msg, int *msg_size)
{
    return aes_msg(msg, msg_size, 1);
}
int AccessServerHandler::DecryptMsg(const char** msg, int *msg_size)
{
    return aes_msg(msg, msg_size, 0);
}
int  AccessServerHandler::Send2AC(const char* msg, int msg_size)
{
    if(m_workStatus != AS_WORKING)
        return -1;
    if(strlen(m_aes_key))
    {
        EncryptMsg(&msg, &msg_size);
        int send_bufsiz = 2 + msg_size; 

        // plog_int(send_bufsiz);

        char* send_buf = (char*)malloc(send_bufsiz);

        *(short*)send_buf = htons(2);
        memcpy(send_buf+2, msg, msg_size);
        free((void*)msg);
        SendMsg(send_buf, send_bufsiz, PROTOTYPE);
        free(send_buf);
    }
    else
    {
        SendMsg(msg, msg_size, PROTOTYPE);
    }

    return 0;
}

int AccessServerHandler::HandleRead(int ms)
{
    fd_set readfds;

    FD_ZERO(&readfds);

    FD_SET(nopoll_conn_socket(m_conn), &readfds);
    FD_SET(m_event_fd[0], &readfds);

    int maxfd = nopoll_conn_socket(m_conn);
    if(maxfd < m_event_fd[0]) 
        maxfd = m_event_fd[0];

    timeval to;
    to.tv_sec = ms / 1000;
    to.tv_usec = (ms % 1000) * 1000;

    int n = select(maxfd+1, &readfds, NULL, NULL, &to);

    if(n>0)
    {
        if(FD_ISSET(m_event_fd[0], &readfds))
        {
            char *cmd = NULL;
            read(m_event_fd[0], &cmd, sizeof(char*));
            if(cmd)
            {
                plog_str(cmd);
                int ret = Send2AC(cmd, strlen(cmd));
				plog("Send2AC ret %d\n",ret);
                free(cmd);
            }
            else
            {
                /* recv NULL ptr mean need stop this select*/
                plog("interrepted!\n");
                return 0;
            }
        }
        if(FD_ISSET(nopoll_conn_socket(m_conn), &readfds))
        {
            noPollMsg* msg = nopoll_conn_get_msg(m_conn);

            if(msg) on_message(msg);
        }
    }
    return 0;
}

int AccessServerHandler::GetTransInfo(int chlnum, int media_type, int usage)
{
    JsonBuilder enc;

    if(!IsAlive()) return -1;

    enc.AddInt( "type", AC_MSG_DATA);
    enc.AddInt( "length", 0);
    if( usage == 1)
        enc.AddInt( "message_id", 1022111);
    else if( usage == 2 )
        enc.AddInt( "message_id", 2022111);
    else if( usage == 3 )
        enc.AddInt( "message_id", 3022111);
    else if( usage == 4 )
        enc.AddInt( "message_id", 4022111);

    enc.AddInt( "cmd", DEV_SRV_GET_TRANSINFO_SYN);
    enc.AddString("to_id", "");
    enc.AddString("from_id", m_devId);
    enc.AddString("dev_id", m_devId);
    enc.AddInt("channel_id", chlnum);
    enc.AddInt("usage", usage);

    char* cmd = strdup(enc.GetJson());

    write(m_event_fd[1], &cmd, sizeof(char*));
    return 0;
}

int  AccessServerHandler::GetServerTimeStamp()
{
    return m_server_timestamp+p2p_get_uptime();
}

int  AccessServerHandler::SetCallback(AccessMsgHandler msg_cb, AccessNetworkCallback network_cb)
{
    m_msg_cb = msg_cb;
    m_network_cb = network_cb;
    return 0;
}

void AccessServerHandler::on_network_change(access_network_status_t s)
{
    if(m_network_cb) m_network_cb(s);
}
void AccessServerHandler::CloseConnection()
{
    if(m_conn)
        nopoll_conn_close(m_conn);
    m_conn = NULL;
}

