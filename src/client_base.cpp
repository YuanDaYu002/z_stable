#include <string.h>
#include <errno.h>
#include "plog.h"
#include "client_base.h"

//extern for wifi
int socket_write_none = 0;

ClientBase::ClientBase(ZmdSocket& sock, CLIENT_WORK_MODE mode)
    : m_sock(sock), m_mode(mode), m_msg_handler(PrivateMsgHandler(sock)) 
{
    m_status = CLIENT_STATUS_COMMON;
};
ClientBase::~ClientBase() 
{
	socket_write_none = 0;
}

int  ClientBase::SendReply(void* data, int len) 
{
    m_send_buffer.Write(data, len);
    return len;
}
int ClientBase::HandleCmdImp(trans_msg_s* head, void* msg_body, int msg_size) 
{
    switch (ntohl(head->cmd)) 
    {
        case PC_IPC_MEDIA_TYPE_SYN:
        case PC_IPC_VOICE_STATUS_SYN:
        case PC_IPC_STOP_MEDIA_SYN:
            HandleVideo(head, msg_body, msg_size);
            break;
        case PC_IPC_PLAYBACK_SYN:
        case PC_IPC_PLAYBACK_STOP_SYN:
        case PC_IPC_PLAYBACK_LIST_SYN:
        case PC_IPC_PLAYBACK_DATE_SYN:
            HandlePlayback(head, msg_body, msg_size);
            break;
        default:
            m_msg_handler.HandleMsg(head, msg_body, msg_size);
            break;
    }
    return 0;
}
void ClientBase::work_imp() 
{
    m_last_active_time = time(NULL);
    m_last_heartbeat_time = 0;
    time_t current_time = m_last_active_time;
    m_sock.SetBlocking(false);

    /* tcp real time streaming mode send audio by default */
    if (m_mode == CLIENT_WORK_TRAN && m_status == CLIENT_STATUS_VIDEO)
        m_video.RequestAuido(1);

    bool read, write;
    while (m_status != CLIENT_STATUS_STOP) 
    {
        read=write=false;
        if (0 > m_sock.Select(read, write, 20))
            break;

        time(&current_time);

        if (read) 
        {
            if (HandleRead() <= 0)
                break;
            if (HandleCmd() < 0)
                break;
            m_last_active_time = current_time;
        } else if(OnIdle(current_time)) 
        {
            break;
        }

        if (write && m_status != CLIENT_STATUS_STOP) 
        {
            if (0 == HandleWrite()) 
            {
                if (m_status == CLIENT_STATUS_COMMON)
                    usleep(50 * 1000);
                else
                    usleep(1000);
            }
        }
		if(write)
			socket_write_none = 0;
		else
			socket_write_none = 1;
    }

    OnClose();
}
int	ClientBase::HandleRead() 
{
    int ret = 0;
    char buf[1500]; //MTU size

    if((ret = m_sock.Read(buf, sizeof(buf))) > 0)
        return m_recv_buffer.Write(buf, ret);
    else if (ret == 0 || (ret == -1 && errno != EAGAIN && errno != EINTR)) 
    {
        if(ret != 0)
            strerror_r(errno, buf, sizeof(buf));
        else
            strcpy(buf, "socket closed by peer");
        plog("read error: [ret=%d],[%s]\n", ret, buf);
        m_status = CLIENT_STATUS_STOP;
        return -1;
    }
    /* this should not happen */
    plog("unexpected error accrued !!!\n");
    m_status = CLIENT_STATUS_STOP;
    return -1;
}
int	ClientBase::HandleWrite() 
{
    int len = 0;
    int ret = 0;
    char* data = NULL;

    if (m_send_buffer.GetWriteOffset() > 0) 
    {
        ret = m_sock.Write(m_send_buffer.GetBuffer(), m_send_buffer.GetWriteOffset());
        if (ret <= 0 && errno != EAGAIN && errno != EWOULDBLOCK) 
        {
            plog("write error(%s)!\n", strerror(errno));
            return -1;
        }
        if (ret > 0) 
        {
            m_send_buffer.Read(NULL, ret);
        }
        if (m_send_buffer.GetWriteOffset() > 0) return 1;
    }

    if (m_status == CLIENT_STATUS_VIDEO) 
    {
        ret = m_video.GetOneFrame((void**)&data, &len);
    } else if (m_status == CLIENT_STATUS_PLAYBACK) 
    {
        ret = m_playback.GetData((void**)&data, &len);
        //ret ==0 mean record file has been all send
        //we need send a end signal
        if (ret == 0) 
        {
            trans_msg_s head = P2P_HEAD_INITIALIZER;

            head.cmd = htonl(IPC_PC_PLAYBACK_STOP_SYN);
            head.cmd_type = htons(CMD_TYPE_IPC_CLIENT);

            m_send_buffer.Write(&head, P2P_HEAD_LEN);

            m_status = CLIENT_STATUS_COMMON;
            plog("playback send end\n");
            return 0;
        }
    }

    if (ret > 0) 
    {
        m_send_buffer.Write(data, len);
        return 1;
    } else if (ret < 0) 
    {
        m_status = CLIENT_STATUS_STOP;
        return -1;
    }
    return 0;
}
int	ClientBase::HandleCmd() 
{
    trans_msg_s* head;
    char buf[2048];
    char* msg_head = NULL;
    char* msg_body = NULL;
    int  ret = 0;

    do {
        int readable_size = m_recv_buffer.GetWriteOffset();

        if (readable_size < P2P_HEAD_LEN) break;

        msg_head = (char*)m_recv_buffer.GetBuffer();
        head = (trans_msg_s*)msg_head;

        if (head->magic != ZMD_MAGIC) 
        {
            plog("bad magic number:0x%x\n", head->magic);
            ret = -1;
            break;
        }

        int len = ntohl(head->length);

        if (len > sizeof(buf) || len < 0) 
        {
            plog("package to big : %d\n", len);
            ret = -1;
            break;
        }
        //recv not enough
        if (len > readable_size - P2P_HEAD_LEN) break;
        //copy msg body to buf
        msg_body = msg_head + P2P_HEAD_LEN;
        memcpy(buf, msg_body, len);
        HandleCmdImp(head, buf, len);
        m_recv_buffer.Read(NULL, len + P2P_HEAD_LEN);
    } while (m_recv_buffer.GetWriteOffset() >= P2P_HEAD_LEN);
    return ret;
}
int	ClientBase::HandleVideo(trans_msg_s* head, void* body, int body_len) 
{
    typedef struct _video_req_t 
    {
        char streamtype;
        char chlnum;
        char operation;
        char reserved;
    } video_req_t;

    char* buf = (char*)body;
    unsigned int cmd = ntohl(head->cmd);

    if (cmd == PC_IPC_MEDIA_TYPE_SYN) 
    {
        video_req_t* req = (video_req_t*)buf;
        if (req->streamtype < 0 || req->streamtype > 2)
            return 0;
        if (p2p_is_channel_valid(req->chlnum, req->streamtype)) 
        {
            if (req->operation == 1) 
            {
                if (m_video.RequestMedia(req->chlnum, req->streamtype))
                    m_status = CLIENT_STATUS_VIDEO;
            } else 
            {
                m_video.ReleaseMedia();
                m_status = CLIENT_STATUS_COMMON;
            }

            buf[P2P_HEAD_LEN] = '\0';
        } else 
        {
            buf[P2P_HEAD_LEN] = '\1';
        }
        head->cmd = htonl(PC_IPC_MEDIA_TYPE_ACK);
        head->length = htonl(1);
        memcpy(buf, head, P2P_HEAD_LEN);
        buf[P2P_HEAD_LEN] = '\0';
        m_send_buffer.Write(buf, P2P_HEAD_LEN + 1);
    } else if (cmd ==  PC_IPC_VOICE_STATUS_SYN) 
    {
        plog("recv voice syn\n");
        m_video.RequestAuido(buf[0]);
        head->cmd = htonl(PC_IPC_VOICE_STATUS_ACK);
        head->length = htonl(1);
        memcpy(buf, head, P2P_HEAD_LEN);
        buf[P2P_HEAD_LEN] = 0;
        m_send_buffer.Write(buf, P2P_HEAD_LEN + 1);
    } else if (cmd ==  PC_IPC_STOP_MEDIA_SYN) 
    {
        m_video.ReleaseMedia();
        m_status = CLIENT_STATUS_COMMON;
    }
    return 0;
}
int	ClientBase::HandlePlayback(trans_msg_s* head, void* body, int body_len) 
{
    char* buf = (char*)body;
    int cmd = ntohl(head->cmd);

    if (cmd == PC_IPC_PLAYBACK_SYN) 
    {
        if (body_len < sizeof(client_playback_syn)) 
        {
            plog("body size less than expect!\n");
            return -1;
        }
        if ( m_status != CLIENT_STATUS_COMMON)
        {
            plog("device busy!\n");
            return -1;
        }
        client_playback_syn* req = (client_playback_syn*)buf;

        int ret = !m_playback.RequestPlayback(ntohl(req->date), req->chl);
        head->cmd = htonl(PC_IPC_PLAYBACK_ACK);
        head->length = htonl(4);
        memcpy(buf, head, P2P_HEAD_LEN);
        buf[P2P_HEAD_LEN] = ret;
        m_send_buffer.Write(buf, P2P_HEAD_LEN + 4);
        if (ret == 0) 
        {
            m_status = CLIENT_STATUS_PLAYBACK;
            plog("start playback\n");
        }
    } else if (cmd == PC_IPC_PLAYBACK_STOP_SYN) 
    {
        m_status = CLIENT_STATUS_COMMON;
        head->cmd = htonl(PC_IPC_PLAYBACK_STOP_ACK);
        head->length = htonl(4);
        memcpy(buf, head, P2P_HEAD_LEN);
        buf[P2P_HEAD_LEN] = 0;
        m_send_buffer.Write(buf, P2P_HEAD_LEN + 4);
        m_playback.ReleasePlayback();

    } else if (cmd == PC_IPC_PLAYBACK_LIST_SYN) 
    {
        uint32_t date = ntohl(*(uint32_t*)buf);
        const int num_per_package = 50;
        const int node_size = 12;
        const int msg_body_size = 8;
        int chl = buf[4];

        plog("PC_IPC_PLAYBACK_LIST_SYN[%d,%d]\n", chl, date);

        PlaybackClient::p2p_playback_list_t list = m_playback.GetPlaybackList(chl, date);
        const int lsize = list.size();
        req_playback_list_ack ack = {P2P_HEAD_INITIALIZER, 0};

        ack.head.cmd = htonl(PC_IPC_PLAYBACK_LIST_ACK);
        ack.head.length = htonl(lsize>num_per_package?(num_per_package*node_size + msg_body_size):(lsize*node_size + msg_body_size));
        ack.number = htonl(lsize);
        ack.result = 0;

        const int slic_cnt = lsize/num_per_package + 1;
        ack.head.seqnum = slic_cnt;
        ack.head.seqnum |= 0<<16; 
        ack.head.seqnum = htonl(ack.head.seqnum);

        m_send_buffer.Write(&ack, sizeof(ack));
        for(int i=0; i<lsize; i++)
        {	//每num_per_package个为一包，每包带包头
            if(i != 0 && i%num_per_package == 0)
            {
                int left_num = lsize-i;
                ack.head.length = htonl(left_num>num_per_package?(num_per_package*node_size):(left_num*node_size));
                ack.head.seqnum = slic_cnt;
                ack.head.seqnum |= (i/num_per_package)<<16; 
                ack.head.seqnum = htonl(ack.head.seqnum);
                m_send_buffer.Write(&ack.head, sizeof(ack.head));
            }
            p2p_record_info_t info = list[i];
            info.start_time = htonl(info.start_time);
            info.end_time = htonl(info.end_time);
            info.type = htons(info.type);
            info.level= htons(info.level);

            m_send_buffer.Write(&info, node_size);
        }
    } else if (cmd == PC_IPC_PLAYBACK_DATE_SYN) 
    {
        if(body_len < sizeof(req_playback_date_list_syn_s))
        {
            plog("body_len < %d\n", sizeof(req_playback_date_list_syn_s));
            return -1;
        }
        req_playback_date_list_syn_s *req = (req_playback_date_list_syn_s*)body;
        PlaybackClient::p2p_playback_date_list_t l =
            m_playback.GetPlaybackDateList(req->channel,
                    (time_t)ntohl(req->start_date),
                    (time_t)ntohl(req->end_date));
        req_playback_date_list_ack_s ack = {P2P_HEAD_INITIALIZER, 0, 0, 0};

        ack.head.cmd = htonl(PC_IPC_PLAYBACK_DATE_ACK);
        ack.head.length = htonl(l.size()*4+8);
        ack.result = 0;
        ack.number = htonl(l.size());

        m_send_buffer.Write(&ack, sizeof(ack));
        m_send_buffer.Write(&l[0], l.size()*4);
        plog("req playback date end...........\n");
    }
    return 0;
}
void ClientBase::OnClose() 
{
    m_sock.Close();
}
int  ClientBase::OnIdle(long cur_sec) 
{
    if (m_msg_handler.m_is_hold_mic && 
            labs(cur_sec - m_msg_handler.m_last_talk_heartbeat) > 7) 
    {
        plog("talk heart beat timeout!\n");
        m_msg_handler.m_is_hold_mic = false;
        p2p_free_mic(this, (char*)this);
    }
    if (m_mode == CLIENT_WORK_P2P && labs(cur_sec - m_last_active_time) > 5) 
    {
        plog("heart beat timeout!\n");
        return -1;
    } 
    //中转模式需要主动发心跳
    if (m_mode == CLIENT_WORK_TRAN ) 
    {
        if(labs(cur_sec - m_last_heartbeat_time) > 5) 
        {
            trans_msg_s msg = P2P_HEAD_INITIALIZER;

            msg.cmd_type = htons(CMD_TYPE_IPC_TRANS);
            msg.cmd = htonl(IPC_TRANS_HEART_SYN);
            SendReply(&msg, sizeof(msg));
            time(&m_last_heartbeat_time);
            plog("tcp heart beat send!\n");
        }
        if(cur_sec - m_last_active_time > 15) 
        {
            plog("tcp heart beat timeout!\n");
            return -1;
        }
    }
    return 0;
}

