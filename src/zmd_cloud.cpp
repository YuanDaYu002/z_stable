#include <pthread.h>  
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>

#include "p2p_access_client.h"
#include "zjson.h"
#include "plog.h"
#include "device_operation.h"
#include "zmd_cloud.h"
#include "tcp_socket.h"
#include "rtp_spliter.h"
#include "zmd_msg.h"
#include "web.h"

#ifndef _BUILD_FOR_NVR_
extern bool p2p_is_device_online();

const int CLD_TCP_RCV_BUFSIZ = 2048;

struct cld_base_t
{
    TcpSocket sock;
    int    last_active_time;
    int    last_keepalive_time;
    int	   now_time;
    void*  media_id;
    int    frame_bytes_offset;
    char   recv_buffer[CLD_TCP_RCV_BUFSIZ];	
    int    read_bytes_offset;
    int    sending_alarm;
    int    sending_normal;
    RTPSpliter rtpSpliter;
    int    frame_flag;
    int    last_send_i_frame_time;
    int	   cvr_alarm_interval;
    int    cvr_on;
    long long last_iframe_pts;
    int    alarm_flag;  // 0 无告警，1 主告警， 2 次告警
    int    stream_type;  // 云存储录像码流类型
    int    send_iframe_interval;    // 无告警时，i帧发送间隔
    int	   write_exit_flag;	// 云存储写操作导致连接断开
    //long long last_vframe_pts; // 上一次传输的视频帧
    //long long last_aframe_pts; // 上一次传输的音频帧
};

int cld_connect_server(const char* identity, const char* ip, int port);
int cld_handle_read(cld_base_t *ctx);
int cld_handle_write(cld_base_t *ctx);
int cld_send_keepalive(cld_base_t *ctx);
int cld_loop(cld_base_t *ctx);

static s_transit_server_info g_server;

void print_server_disconnected(int result, int error_no)
{
    if(result == 0)
        plog("disconnect with server!!! socket closed by server\n");
    else
        plog("%s %s\n", "disconnect with server!!!", strerror(error_no));

}
/* 中转服务器信息保存 */
void refresh_transit_info(char* id, char* ip, int port)
{
    if(id != NULL)
        strncpy(g_server.identity, id, 64);
    g_server.server_port = port;
    if(ip != NULL)
        strncpy(g_server.server_ip, ip, 32);	
}

void cld_set_trans_info(s_transit_server_info * info)
{
    if(g_server.change_flag)
    {
        refresh_transit_info(info->identity, info->server_ip, info->server_port);
        g_server.change_flag = 0;
    }
}

/* 获取云录像中转服务器信息  iCloud get transit login info*/
int get_transit_info(cloud_stream_type video_type)
{
    memset(&g_server, 0x0, sizeof(s_transit_server_info));
    g_server.change_flag = 1;
    p2p_access_client_get_transinfo(0, 1, 3);
    int count = 0;
    while(g_server.change_flag)
    {
        usleep(500*1000);
        count++;
        if(count > 10)
            break;
    }	

    if(g_server.server_port <=0 ||  g_server.server_port > 65535)
        return -1;

    return 0;
}

void cloud_handler_thread(void* param)
{
	static cld_base_t ctx;
    ctx.sending_alarm = ctx.sending_normal = 0;
    ctx.frame_bytes_offset = ctx.read_bytes_offset = 0;
    ctx.last_keepalive_time = ctx.last_send_i_frame_time = 0;
    ctx.media_id = NULL;
    ctx.last_active_time = ctx.now_time = p2p_get_uptime();
    ctx.frame_flag = 0;
    ctx.last_iframe_pts = 0;
	ctx.write_exit_flag = 1;
	//ctx.last_vframe_pts = ctx.last_aframe_pts = 0;

    while(1)
    {
        /* 普通云录像开关判断 General icloud switch  告警云录像开关判断 Alarm icloud switch */
        if(p2p_get_sync_cvr_on() == 2)
        {
            /*获取中转信息 get transit information */
            if(get_transit_info(cloud_alarm_video) < 0)
            {
                sleep(10);
                continue;
            }
        }
        else if(p2p_get_sync_cvr_on() == 1)
        {
            /*获取中转信息 get transit information */
            if(get_transit_info(cloud_video) < 0)
            {
                sleep(10);
                continue;
            }
        }
        else
        {
            sleep(10); /* 10 Sec Check again 10 秒检查一次*/
            continue;
        }

        if(!p2p_is_device_online())
        {
            plog("device is offline!\n");
            sleep(10);
            continue;
        }
        if(!p2p_is_device_on())
        {
            plog("device is off!\n");
            sleep(10);
            continue;
        }
		#if 0
        cld_base_t * ctx = cld_connect_server(g_server.identity, 
                g_server.server_ip, g_server.server_port);

        if(ctx == NULL)
        {
            sleep(p2p_get_rand(10, 60));
            continue;
        }

        ctx->cvr_alarm_interval = p2p_get_cvr_interval();
        ctx->send_iframe_interval = p2p_base()->cloud_iframe_interval;
        ctx->stream_type = p2p_base()->cloud_stream_type;
        plog_int(ctx->cvr_alarm_interval);
        cld_loop(ctx);
        delete ctx;
		#else
		int ser_scok = cld_connect_server(g_server.identity, 
                g_server.server_ip, g_server.server_port);
		if(ser_scok < 0)
		{
			sleep(p2p_get_rand(10, 60));
            continue;
		}

		ctx.sock.SetHandle(ser_scok);
		ctx.cvr_alarm_interval = p2p_get_cvr_interval();
		ctx.send_iframe_interval = p2p_base()->cloud_iframe_interval;
		ctx.stream_type = p2p_base()->cloud_stream_type;
		plog_int(ctx.cvr_alarm_interval);
		cld_loop(&ctx);
		#endif
        sleep(p2p_get_rand(5,10));
    }	
}


/*
 *@ 云存储常驻线程 General iCloud work pthread 
 */
int cloud_init()
{
    int capacity = p2p_get_device_extend_capacity();
    if((capacity & 0x20) || (capacity & 0x30))
    {
        plog("zmd cloud init !\n");
        p2p_long_task(cloud_handler_thread);
    }
    return 0;
}

int cld_connect_server(const char* identity, const char* ip, int port)
{
    //cld_base_t *base = NULL;
    TcpSocket tcp_sock;
    tf_login_syn_s msg = { P2P_HEAD_INITIALIZER, {0}, {0} };

    if(tcp_sock.ConnectPeer( ip, port))
    {
        plog("connect with %s:%d timeout\n", ip, port);
        return NULL;
    }
    plog("connected with tcp server\n");

    msg.head.cmd_type = htons(CMD_TYPE_IPC_TRANS);
    msg.head.cmd = htonl(IPC_TRANS_LOGIN_SYN);
    msg.head.length = htonl(sizeof(tf_login_syn_s) -  P2P_HEAD_LEN);

    strcpy(msg.register_code, identity);
    strcpy(msg.username_or_sn, p2p_get_deviceid());
    plog("send identity:(%s, %s)\n", msg.register_code, msg.username_or_sn);

    //send identity
    tcp_sock.WriteN((char*)&msg, sizeof(tf_login_syn_s));
    if(!tcp_sock.IfReadable(10000))
    {
        plog("%s\n",  "wait for server reply failed!!!");
        goto ERROR;
    }
    char buf[512];

    plog("recv response from tcp server\n");
    if(tcp_sock.Read(buf, P2P_HEAD_LEN+1) != P2P_HEAD_LEN+1)
    {
        plog("%s\n", "recv server reply failed!!!");
        goto ERROR;
    }
    if(buf[P2P_HEAD_LEN] != 0)
    {
        plog("%s:%d\n", "login to cvr server failed : ", buf[P2P_HEAD_LEN]);
        goto ERROR;
    }

	#if 0
    base = new cld_base_t;

    base->sock.SetHandle(tcp_sock.GetHandle());
    base->sending_alarm = base->sending_normal = 0;
    base->frame_bytes_offset = base->read_bytes_offset = 0;
    base->last_keepalive_time = base->last_send_i_frame_time = 0;
    base->media_id = NULL;
    base->last_active_time = base->now_time = p2p_get_uptime();
    base->frame_flag = 0;
    base->last_iframe_pts = 0;

    plog("%s\n", "login to cvr server success!!");
    return base;
	#else
	plog("%s\n", "login to cvr server success!!");
	return tcp_sock.GetHandle();
	#endif

ERROR:
    tcp_sock.Close();
    return -1;
}

int cld_loop(cld_base_t *ctx)
{
    int last_refresh_cvr_on_time = 0;
	static void* media_id = NULL;
	static int last_stream_type = -1;

	/*获取读指针,整个云存储阶段只获取一次,如遇到码流切换则重新获取*/ 
	if(media_id == NULL || last_stream_type != ctx->stream_type)
	{
		if(media_id != NULL)
		{
			p2p_free_media_id(media_id);
			plog("get new media_id for cld_loop");
		}
		else
			plog("init media_id for cld_loop");
		media_id = p2p_get_media_id(0, ctx->stream_type);
		if(!media_id)
			return -1;
		last_stream_type = ctx->stream_type;
		p2p_set_i_frame(media_id);
	}

    ctx->media_id = media_id;
    //if(!ctx->media_id)
       // return -1 ;
    //p2p_set_i_frame(ctx->media_id);	

    ctx->cvr_on = p2p_get_sync_cvr_on();	
    last_refresh_cvr_on_time = ctx->last_active_time = p2p_get_uptime();
    ctx->sock.SetBlocking(false);
    while(1)
    {
        bool readable, writable;

        ctx->now_time = p2p_get_uptime();

        if(ctx->now_time % 10 == 0 && !p2p_is_device_online())
        {
            plog("device is offline now!\n");
            break;
        }
        if(ctx->now_time % 10 == 0 && !p2p_is_device_on())
        {
            plog("device is off now!\n");
            break;
        }
        if(ctx->now_time - last_refresh_cvr_on_time > 30)
        {
            ctx->cvr_on = p2p_get_sync_cvr_on();
            last_refresh_cvr_on_time = ctx->now_time;
        }
        ctx->sock.Select(readable, writable, 10);

        if(ctx->now_time - ctx->last_active_time > 60)
        {               
            plog("tcp server not return keep alive ack!!!\n");
            break;
        }

        if ( readable )
        {
            if(cld_handle_read(ctx))
                break;
        }
        if ( writable )
        {
            int ret = cld_handle_write(ctx);
            if(ret == 0)
            {
                usleep(1000*5);
            }
            else if(ret < 0)
            {
                plog("handle write failed!!!\n");
                break;
            }
        }			
    }

	/*云存储退出的时候 不释放 是为了buffer未覆盖时 重传掉线时间段的视频*/
    //p2p_free_media_id(ctx->media_id);
    ctx->sock.Close();

	ctx->write_exit_flag = 1;
    plog("tcp cloud loop exit...\n");	
    return 0;
}
int cld_handle_read(cld_base_t *ctx)
{	
    int msg_len;
    int ret;
    trans_msg_s *head;

    ctx->last_active_time = ctx->now_time;

    ret=ctx->sock.Read(ctx->recv_buffer+ctx->read_bytes_offset, 
            CLD_TCP_RCV_BUFSIZ-ctx->read_bytes_offset);

    if(ret > 0)
    {
        ctx->read_bytes_offset+= ret;
    }
    else if(ret <= 0 && errno != EAGAIN && errno != EINTR)
    {
        print_server_disconnected(ret, errno);
        return -1;
    }
    //处理收到的全部消息
    do
    {
        head = (trans_msg_s*)ctx->recv_buffer;

        if( head->magic != ZMD_MAGIC)
        {		
            plog("bad magic number:0x%x\n", head->magic);
            return -1;
        }

        msg_len = ntohl(head->length) + P2P_HEAD_LEN;

        if(msg_len + ctx->read_bytes_offset >= CLD_TCP_RCV_BUFSIZ)
        {
            plog("error: body length %d more than %d\n", msg_len, CLD_TCP_RCV_BUFSIZ);
            return -1;
        }

        if(msg_len > ctx->read_bytes_offset)
            continue;

        if(IPC_TRANS_HEART_ACK == ntohl(head->cmd))
        {
            plog("recv heart beat ack from server\n");
        }
        else
        {
            plog("recv unknow msg type:%d\n", ntohl(head->cmd));
        }
        ctx->read_bytes_offset -= msg_len;
        if( ctx->read_bytes_offset > 0 )
        {
            memmove(ctx->recv_buffer, ctx->recv_buffer + msg_len, ctx->read_bytes_offset);
        }

    }while(ctx->read_bytes_offset > 0);

    return 0;	
}

int cld_handle_write(cld_base_t *ctx)
{
    static int last_md_time = 0;
    static int begin_send_alarm_time = 0;
    int getRet = 0 ;	   
    unsigned char* data = 0 ;
    FrameInfo info = {0} ;


    //是否上次的数据还没有发送完
    if(ctx->frame_bytes_offset != 0 || (ctx->write_exit_flag && ctx->frame_bytes_offset != 0))
    {
		if( ctx->write_exit_flag )
		{
			ctx->write_exit_flag = 0;
			ctx->frame_bytes_offset = 0;
			plog("retran last frame!\n");
		}
		else
			plog("trans the remains of last frame!\n ");
	
        getRet = ctx->sock.Write(ctx->rtpSpliter.GetSendBuffer()+ctx->frame_bytes_offset,
                ctx->rtpSpliter.GetSendBufferSize()-ctx->frame_bytes_offset);
        //plog("frame_bytes_offset:%d\n", ctx->frame_bytes_offset);
        if(getRet < 0)
        {
        	if(ctx->frame_bytes_offset == 0) ctx->frame_bytes_offset = 1;//cur frame need retrans
        	plog("send icloud data to tcp server error:%s!\n", strerror(errno));
            return -1;
		}
		else if(getRet > 0)
        {
            ctx->frame_bytes_offset+=getRet;
            if(ctx->frame_bytes_offset == ctx->rtpSpliter.GetSendBufferSize())
                ctx->frame_bytes_offset = 0;
            return 1;
        }
        else
            return 0;
    }

    if(labs(ctx->now_time - ctx->last_keepalive_time) > 10)
    {
        if(0>cld_send_keepalive(ctx))
        {
            plog("disconnect with tcp server!!!\n");
            return -1;
        }			
        ctx->last_keepalive_time = ctx->now_time;
    }

    getRet = p2p_get_one_frame(ctx->media_id, &data, &info);

    //drop frame
    if(getRet == -2)
        return 1;
    if( getRet != -1 && data && info.FrmLength > 0 )
    {
        if( info.FrmLength >= MAX_FRAME_SIZ)
        {
            plog("frame to big!!! (%lu)\n", info.FrmLength);
            return -1;
        }

        int flag = 0;

        VideoFrameHeader* fh = (VideoFrameHeader*)data;

        if(fh->m_nVHeaderFlag != 0x63643030 && 
                fh->m_nVHeaderFlag != 0x63643130 &&
                fh->m_nVHeaderFlag != 0x62773130)
        {
            plog("invalid header flag:[0x%x]\n", fh->m_nVHeaderFlag);
            p2p_set_i_frame(ctx->media_id);
            return 1;
        }
        if( info.Flag != 3 )
        {			
            if(fh->m_FrameType == 0)
                flag = 0;
            else if(fh->m_FrameType == 1)
                flag = 3;
            else if(fh->m_FrameType == 2)
                flag = (ctx->frame_flag%2)+1;
            else if(fh->m_FrameType == 4)
                flag = (ctx->frame_flag++%2)+1;	

            /* 检测告警云存储开关 */
            if( ctx->cvr_on == 2 )
            {
                ctx->sending_normal = 0;
                /* 移动侦测判断 check the motion status*/
                if( fh->m_b3CloudRecord == 0 )
                {
                	/* 预防有些设备没有用报警优化的新逻辑， 这里强制发送cvr_alarm_interval 秒的视频 */
                    if(ctx->now_time - begin_send_alarm_time > ctx->cvr_alarm_interval+2)
                    {
                        /* 非移动侦测只发I帧10s 上传一次 */
                        if(ctx->sending_alarm == 1)
                        {
                            ctx->sending_alarm = 0; 
                            plog("ending send alarm video...\n");
                        }
                    }
                    else
                        fh->m_b3CloudRecord	= ctx->alarm_flag; // cvr_alarm_interval之内的帧都置为告警标记
                }
                else
                {
                    ctx->alarm_flag = fh->m_b3CloudRecord;
                    last_md_time = ctx->now_time;
                    /* 移动侦测发生 先发I帧 防止花屏*/
                    if(ctx->sending_alarm == 0)
                    {
                        plog("begin send alarm video...\n");	
                        begin_send_alarm_time = ctx->now_time;

                        ctx->sending_alarm = 1;
                        if(info.Flag != 1)
                        {
                            p2p_set_i_frame_by_curpos(ctx->media_id);
                            return 1;
                        }	 					
                    }
                }		
            }
            else if(ctx->cvr_on == 1) /* 普通云存储 */
            {
                ctx->sending_alarm = 1;
                if(ctx->sending_normal == 0)
                {
                    ctx->sending_normal = 1;
                    if(info.Flag != 1)
                    {
                        p2p_set_i_frame_by_curpos(ctx->media_id);
                        return 1;
                    }				
                }
            }
            else if(ctx->cvr_on == 0)
            {
            	p2p_set_current_i_frame(ctx->media_id);
                plog("close the Cloud\n");
                return -1;
            }			
        }
        else 
            flag = 4;

        bool need_send = (ctx->sending_alarm || ctx->sending_normal);

        if(!need_send && info.Flag == 1 && ctx->now_time - ctx->last_send_i_frame_time >ctx->send_iframe_interval-2)
        {
            need_send = true;
            plog("sending i frame ...\n");
            ctx->last_send_i_frame_time = ctx->now_time;
            ctx->last_iframe_pts = info.Pts;
        }

        if(ctx->sending_alarm && info.Flag == 1 && ctx->last_iframe_pts == info.Pts)
        {
        	plog("ignore iframe that already send!\n");
        	need_send = 0;
        }

#if 0
		if(need_send)
		{
			if((info.Flag == 3 && info.Pts <= ctx->last_aframe_pts) || (info.Flag != 3 && info.Pts <= ctx->last_vframe_pts))
			{
				plog("ignore the frame that last md already send!\n");
				need_send = 0;
			}
		}

		if(need_send)
		{	
			if(info.Flag != 3)/* v frame */
				ctx->last_vframe_pts = info.Pts;
			else/* a frame */
				ctx->last_aframe_pts = info.Pts;
		}
#endif
		
        if(need_send)
        {
            ctx->rtpSpliter.SplitFrame(CMD_TYPE_IPC_CLIENT, (char*)data, info.FrmLength, flag, 0);
			if( ctx->write_exit_flag ) ctx->write_exit_flag = 0;

            getRet = ctx->sock.Write(ctx->rtpSpliter.GetSendBuffer(), 
                    ctx->rtpSpliter.GetSendBufferSize());
            if(getRet > 0)
            {
                if(getRet != ctx->rtpSpliter.GetSendBufferSize())
                    ctx->frame_bytes_offset = getRet;
                return 1;
            }
            else if(getRet < 0)
            {
            	if(ctx->frame_bytes_offset == 0) ctx->frame_bytes_offset = 1;//cur frame need retrans
                plog("send icloud data to tcp server error:%s!\n", strerror(errno));
                return -1;
            } 
            return 1;
        }
    }
    else
        return 0;
    return 1;

}

int cld_send_keepalive(cld_base_t *ctx)
{
    trans_msg_s msg = P2P_HEAD_INITIALIZER;

    msg.cmd_type = htons(CMD_TYPE_IPC_TRANS);
    msg.cmd = htonl(IPC_TRANS_HEART_SYN);

    plog("%s\n", "sending keepalive to tcp server...");
    return ctx->sock.WriteN(&msg, sizeof(msg));
}
#else
int cloud_init(){};
#endif
