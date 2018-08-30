#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <netinet/tcp.h>
#ifdef ANDROID_API_LEVEL_LOW
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef P2P_MODE
#include "udt.h"
#include "p2p_socket.h"
#endif

#include "tcp_socket.h"
#include "zmd_msg.h"
#include "media_client.h"
#include "plog.h"
#include "p2p_streamer.h"
#include "p2p_def.h"

#include "rtmp_send.h"

typedef struct _p2p_info_s
{
  char local_ip[32];
  int  local_port;
  int  channel;
  int  streamtype;
} p2p_info_s;

typedef struct _tcp_handle_param
{
  char identity[128];
  char server_ip[32];
  char device_id[32];
  int  server_port;
  int  streamtype;
  int  chlnum; 

}tcp_handle_param_t;

extern p2p_init_info_t g_p2p_envir;


#ifdef P2P_MODE

static int get_p2p_info(const char* p2p_fpath, p2p_info_s* info) {
  char buf[1024];

  FILE* file = p2p_fopen(p2p_fpath, "rb");

  if (file) {
    fread(buf, 1, sizeof(buf), file);

    sscanf(buf, "%[^:]:%d:%d:%d",
           info->local_ip, &info->local_port,
           &info->channel, &info->streamtype);
    fclose(file);
    return 0;
  }
  return -1;
}
int p2p_listen(const char* local_ip, int local_port) {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(local_ip);
  addr.sin_port = htons(local_port);

  plog("begin create socket\n");
  UDTSOCKET serv = UDT::socket(AF_INET, SOCK_STREAM, 0);

  int snd_buf = P2P_SNDBUF;
  int rcv_buf = P2P_RCVBUF;
  UDT::setsockopt(serv, 0, UDT_SNDBUF, &snd_buf, sizeof(int));
  UDT::setsockopt(serv, 0, UDT_RCVBUF, &rcv_buf, sizeof(int));
  snd_buf = 8192;
  rcv_buf = 8192;
  UDT::setsockopt(serv, 0, UDP_SNDBUF, &snd_buf, sizeof(int));
  UDT::setsockopt(serv, 0, UDP_RCVBUF, &rcv_buf, sizeof(int));

  bool reuse = true;
  UDT::setsockopt(serv, 0, UDT_REUSEADDR, &reuse, sizeof(bool));

  //set mss
  int mss = P2P_MTU;
  UDT::setsockopt(serv, 0, UDT_MSS, &mss, sizeof(int));

  if (UDT::ERROR == UDT::bind(serv, (struct sockaddr*)&addr, sizeof(addr))) {
    plog("bind: %s\n", UDT::getlasterror().getErrorMessage());
    return false;
  }

  UDT::listen(serv, 10);
  plog("p2plistener working at %s:%d\n", local_ip, local_port);
  return serv;
}

int p2p_accept(int sock) {
  sockaddr_in clientaddr;
  socklen_t addrlen = sizeof(clientaddr);

  UDTSOCKET fhandle;

  //todo check if is the expect peer
  if (UDT::INVALID_SOCK == (fhandle = UDT::accept(sock, (sockaddr*)&clientaddr, (int*)&addrlen))) {
    plog("accept: %s\n",  UDT::getlasterror().getErrorMessage());
    return -1;
  }
  plog("new connection: %s %d \n", inet_ntoa(clientaddr.sin_addr),  ntohs(clientaddr.sin_port));

  return fhandle;
}
void do_p2p(const char* fpath) {
  p2p_info_s info;
  if (0 ==  get_p2p_info(fpath, &info))
    printf("%s:%d:%d:%d\n",
           info.local_ip, info.local_port,
           info.channel, info.streamtype);

  UDT::startup();

  int listen_sock = p2p_listen(info.local_ip, info.local_port);

  int client_sock;
  if (p2p_select_read(listen_sock, 5000) && (client_sock = p2p_accept(listen_sock)) > 0) {
    UDT::close(listen_sock);

    P2PSocket sock;

    sock.SetHandle(client_sock);

    MediaClient* client = new MediaClient(sock);
    if (info.streamtype == MEDIA_TYPE_PLAYBACK) {
    } else {
      if (client->RequestMedia(info.channel, info.streamtype))
        client->SetClientStatus(CLIENT_STATUS_VIDEO);
      else
        client->SetClientStatus(CLIENT_STATUS_STOP);
    }
    client->WaitWorkStop();
    delete client;
  } else {
    plog("%s\n", "accept client error!!!");
  }

  plog("streamer exit!!!\n");
}

#endif // P2P_MODE



//trans mode
int get_trans_info(const char* fpath, tcp_handle_param_t* para) {
  char buf[1024] = { 0 };

  FILE* file = p2p_fopen(fpath, "rb");

  if (file) {
    fread(buf, 1, sizeof(buf), file);

    sscanf(buf, "%[^,],%[^,],%d,%d,%d,%[^,]",
           para->identity,
           para->server_ip,
           &para->server_port,
           &para->chlnum,
           &para->streamtype,
           para->device_id);
    fclose(file);
    return 0;
  }
  return -1;
}
int  connect_trans_server(tcp_handle_param_t* para) {
  TcpSocket sock;
  plog("begin connecting to tcp server...\n");
  if (sock.ConnectPeer(para->server_ip, para->server_port)) {
    plog("connect error!\n");
    return -1;
  }
  plog("connected with tcp server\n");
  tf_login_syn_s msg = { P2P_HEAD_INITIALIZER, { 0 }, { 0 } };
  msg.head.cmd_type = htons(CMD_TYPE_IPC_TRANS);
  if (para->streamtype == MEDIA_TYPE_PLAYBACK)
    msg.head.cmd = htonl(IPC_PLAYBACK_LOGIN_SYN);
  else
    msg.head.cmd = htonl(IPC_TRANS_LOGIN_SYN);
  msg.head.length = htonl(sizeof(tf_login_syn_s) -  P2P_HEAD_LEN);
  if(strlen(g_p2p_envir.aes_key))
      msg.head.flag = 11;

  strcpy(msg.register_code, para->identity);
  strcpy(msg.username_or_sn, para->device_id);
  plog("send identity:(%s, %s)\n", msg.register_code, msg.username_or_sn);

  //send identity
  sock.WriteN(&msg, sizeof(tf_login_syn_s));
  if (!sock.IfReadable(5000)) {
    plog("%s\n",  "wait for server reply failed!!!");
    sock.Close();
    return -1;
  }
  char buf[512];

  plog("recv response from tcp server\n");
  if (sock.ReadN(buf, P2P_HEAD_LEN + 1) != P2P_HEAD_LEN + 1) {
    plog("%s\n", "recv server reply failed!!!");
    sock.Close();
    return -1;
  }
  if (buf[P2P_HEAD_LEN] == 0) {
    plog("%s\n", "login to transfer server success!!");
    return sock.GetHandle();
  } else {
    plog("%s:%d\n", "login to transfer server failed", buf[P2P_HEAD_LEN]);
    sock.Close();
    return -1;
  }

  sock.Close();
  return -1;

}
void ts_increase()
{
	
}
void do_rtmp(const char* fpath) {
  tcp_handle_param_t para;
  unsigned char input[256]={0};	
  memset(&para, 0, sizeof(para));
  
  if (get_trans_info(fpath, &para)) {
    plog("get_trans_info error!\n");
    return;
  }

  plog("%s:%s:%d:%d:%d:%s\n",
       para.identity,
       para.server_ip,
       para.server_port,
       para.chlnum,
       para.streamtype,
       para.device_id);
	  if(g_p2p_envir.on_set_stream_gop)
	  {
		  g_p2p_envir.on_set_stream_gop(1, 1);
	  }

	//return ;
	//rtmp_t *ctx =  rtmp_connect((const char*) "rtmp://192.168.1.66:1935/live/000");	
	//rtmp_t *ctx =  rtmp_connect((const char*) "rtmp://50.233.84.16/dst/RTMPTEST0000001_0_0"); 

	//rtmp_t *ctx =  rtmp_connect((const char*) "rtmp://21-rtmp.myzmodo.com/live/RTMPTEST0000001_0_0?2968022634"); 
	//rtmp://11-rtsmserver.myzmodo.com:443/live/RTMPTEST0000001_0_0?2554042525
	rtmp_t *ctx =  rtmp_connect((const char*) (para.identity)); 
	
	

	
	if(ctx==NULL)
	{
		printf("rtmp_connect %s error\n", para.identity);
		if(g_p2p_envir.on_set_stream_gop)
		{
			g_p2p_envir.on_set_stream_gop(0, 5);
		}
		return ;
	}
	int streamtype=0;
	int chn;
	sscanf(para.identity,"%[^_]_%d_%d",input,&chn,&streamtype);
	if(streamtype<0||streamtype>=3)
		streamtype = 0;
	void *m_mediaid = p2p_get_media_id(0,streamtype);
	p2p_set_i_frame(m_mediaid);
	
	unsigned char* data = 0 ;
	FrameInfo info={0} ;
	int getRet = 0 ;
	long long audiobasets =0;
	long long videobasets=0;
	int			getfail =0;
	long long  vlast=0;
	do
	{
	   getRet = p2p_get_one_frame(m_mediaid, &data , &info );
	   if(getRet!=0)
	   {
			if(getRet==-2)
			{
				continue;
			}
			else 
			{
				if(getfail++>100)
					break;
				usleep(100000);
				
			}
			continue;
	   }
	   getfail =0;
	   if( !getRet && data && info.FrmLength > 0 )
	   {
	   		
		   if( info.FrmLength >= MAX_FRAME_SIZ)
		   {
			   plog("frame to big!!! (%lu)\n", info.FrmLength);
			   break;
		   }
		   
		   if(info.Flag ==1 )//I frame
		   {
				unsigned char* dec_tmp = NULL;
				dec_tmp = (unsigned char*)malloc(info.FrmLength - sizeof(VideoFrameHeader) + 1);
				if(dec_tmp == NULL)
				{
					plog("malloc tmp mem for I frame dec failed!\n");
					break;
				}

				memcpy(dec_tmp, data+sizeof(VideoFrameHeader), info.FrmLength-sizeof(VideoFrameHeader));
		   		memcpy(input,dec_tmp,256);
				aes_decrypt(g_p2p_envir.aes_key,input,dec_tmp);
				if(FALSE==rtmp_stream_send(ctx,dec_tmp,info.FrmLength-sizeof(VideoFrameHeader),RTMP_IFRAME))
				{
					if(dec_tmp != NULL)
						free(dec_tmp);
					break;
				}
				if(videobasets==0)videobasets=info.Pts-20000;
				ctx->videots=( (unsigned int)( info.Pts-videobasets)/1000);

				//printf("I ts---------->%d,%d\n",ctx->videots,ctx->audiots);
				
				//printf("I ts ---->[count:%d,%d][t:%d,%d][%lld]\n",ctx->vcount,ctx->acount,ctx->videots,ctx->audiots,ctx->videots-vlast);
				vlast = ctx->videots;
				if(dec_tmp != NULL)
					free(dec_tmp);
		   }
		   else if(info.Flag ==2 )
		   {
		   		if(FALSE==rtmp_stream_send(ctx,data+sizeof(VideoFrameHeader),info.FrmLength-sizeof(VideoFrameHeader),RTMP_PFRAME))
				{
					break;
				}
				if(videobasets==0)videobasets=info.Pts;
				ctx->videots=( (unsigned int) ( info.Pts-videobasets)/1000 );
				//printf("P ts---------->%d,%d\n",ctx->videots,ctx->audiots);
				
				//printf("P ts ---->[count:%d,%d][t:%d,%d][%lld]\n",ctx->vcount,ctx->acount,ctx->videots,ctx->audiots,ctx->videots-vlast);
				vlast = ctx->videots;
		   }
		   else if(info.Flag ==3 )
		   {
		   	 	//continue;
		   		if(FALSE==rtmp_stream_send(ctx,data+sizeof(AudioFrameHeader),info.FrmLength-sizeof(AudioFrameHeader),RTMP_AFRAME))
				{
					break;
				}
				if(audiobasets==0)audiobasets=info.Pts;
				ctx->audiots=( (unsigned int)( info.Pts-audiobasets)/1000)+100;		
				
		   }

		   //g_p2p_envir.aes_key	

		   
			
	   }
	   
	}while(1); 
	plog("*********************rtmp eixt!!!\n");
	p2p_free_media_id(m_mediaid);
	rtmp_close(ctx);
	if(g_p2p_envir.on_set_stream_gop)
	{
		g_p2p_envir.on_set_stream_gop(0, 5);
	}


}

void do_trans(const char* fpath) {
  tcp_handle_param_t para;

  memset(&para, 0, sizeof(para));
  
  if (get_trans_info(fpath, &para)) {
    plog("get_trans_info error!\n");
    return;
  }
  /* 回放时不限制重复连接请求 */
  if(para.streamtype == MEDIA_TYPE_PLAYBACK)
  	unlink(fpath);
  plog("%s:%s:%d:%d:%d:%s\n",
       para.identity,
       para.server_ip,
       para.server_port,
       para.chlnum,
       para.streamtype,
       para.device_id);

  int fd = 0;
  int try_count = 3;

  while((fd = connect_trans_server(&para)) < 0 && try_count-- > 0)
  {
	plog("connect trans server failed, try again!\n");
  	sleep(1);
  }
  if (fd > 0) {
    TcpSocket sock;
    sock.SetHandle(fd);

    MediaClient* client = new MediaClient(sock, CLIENT_WORK_TRAN);
    if (para.streamtype == MEDIA_TYPE_PLAYBACK) {
    } else {
#if 1
        /* set TCP_CORK to improve network usage, but will lower real-time*/
        int flag = 1;
        if(setsockopt(fd, IPPROTO_TCP, TCP_CORK, (char*)&flag, sizeof(flag)))
            plog("set TCP_CORK error!\n");
#endif
        int tmp_streamtype = para.streamtype;
        if(para.streamtype == MEDIA_TYPE_RTSP)
        {
            tmp_streamtype = 2;
            if(g_p2p_envir.on_set_stream_temporary)
                g_p2p_envir.on_set_stream_temporary(1, 2, p2p_base()->rtsp_frame_rate, p2p_base()->rtsp_bit_rate);
        }
      if (client->RequestMedia(para.chlnum, tmp_streamtype))
        client->SetClientStatus(CLIENT_STATUS_VIDEO);
      else
        client->SetClientStatus(CLIENT_STATUS_STOP);
    }
    client->WaitWorkStop();
    delete client;
  } else {
    plog("%s\n", "connect server failed!!!");
  }
  plog("streamer exit!!!\n");
  p2p_on_video_release(para.streamtype, para.chlnum);
  if(para.streamtype == MEDIA_TYPE_RTSP && g_p2p_envir.on_set_stream_temporary)
      g_p2p_envir.on_set_stream_temporary(0, 2, 0, 0);
}
struct stream_info_t {
  char mode[32];
  char fpath[128];
};
void stream_worker(void* para) {
  plogfn();

  stream_info_t* info = (stream_info_t*)para;
  
  if (strcmp("trans", info->mode) == 0)
      do_trans(info->fpath);
  if (strcmp("rtmp", info->mode) == 0) 
  	  do_rtmp(info->fpath);
#ifdef P2P_MODE
  else if (strcmp("p2p", info->mode) == 0)
	  do_p2p(info->fpath);
#endif // P2P_MODE

  unlink(info->fpath);
  delete info;
}
int start_streamer(const char* mode, const char* fpath) {
  stream_info_t* info = new stream_info_t;

  strcpy(info->mode, mode);
  strcpy(info->fpath, fpath);

  if(!p2p_long_task_p(stream_worker, info))
  {
  	unlink(fpath);
	delete info;
  }
  return 0;
}

void p2p_start_trans_streamer(const char* id, const char* trans_ip, int trans_port,
                                         int channel, int streamtype, const char* devid) {
  char fpath[128];

  if (access(STUN_DIR, 0) != 0) {
    mkdir(STUN_DIR,  0777);
  }

  plogfn();
  //使用文件名来过滤重复的中转请求
  sprintf(fpath, STUN_DIR"/trans_info_%s_%d_%d_%d",
          trans_ip, trans_port, channel, streamtype);

  plog("path:%s\n", fpath);
  if (access(fpath, F_OK) == 0) {
    plog("stream %s already exist!\n", fpath);
    return;
  }
  plogfn();

  FILE* file = p2p_fopen(fpath, "wb");
  if (file) {
    fprintf(file, "%s,%s,%d,%d,%d,%s",
            id,
            trans_ip,
            trans_port,
            channel,
            streamtype,
            devid);
    fclose(file);
  }
  //streamtype=MEDIA_TYPE_RTMP;
  if(streamtype==MEDIA_TYPE_RTMP)
  {
	start_streamer("rtmp", fpath);
  }
  else
  {
	start_streamer("trans", fpath);
  }
}
void test_streamer_proc(void *para)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr, c_addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(8011);

    bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(lfd, 5);

    plog("listen ....\n");
    int req_type = MEDIA_TYPE_PLAYBACK;

    while(1) {
        socklen_t len = sizeof(c_addr);
        int fd = accept(lfd, (struct sockaddr*)&c_addr, &len);

        plog("........\n");
        if (fd > 0) {
            TcpSocket sock;
            sock.SetHandle(fd);

            MediaClient* client = new MediaClient(sock, CLIENT_WORK_TRAN);
            if (req_type == MEDIA_TYPE_PLAYBACK) {
            } else {
                if (client->RequestMedia(0, 1))
                    client->SetClientStatus(CLIENT_STATUS_VIDEO);
                else
                    client->SetClientStatus(CLIENT_STATUS_STOP);
            }
            client->WaitWorkStop();
            delete client;
        } else {
            plog("%s\n", "connect server failed!!!");
        }
        plog("exit ...\n");
    }
}
void test_streamer()
{
    p2p_long_task(test_streamer_proc);
}
