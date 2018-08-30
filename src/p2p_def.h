#ifndef _P2P_DEF_H_
#define _P2P_DEF_H_

#define USER_TYPE  2
#define P2P_SET_STATUS(s) do{g_session.status = s; p2p_mt_set_status(#s);}while(0)
#define P2P_CHECK_STATUS(s) (g_session.status == s)

#define IFRAME_FLAG 0x63643030
#define PFRAME_FLAG 0x63643130
#define AFRAME_FLAG 0x62773130

typedef enum {
    P2P_ONLINE=1,    //registed success    
    P2P_SENDING_HEARTBEAT,
    P2P_HEARTBEAT_ERROR,
    P2P_LOGINING,
    P2P_NEED_LOGIN,   //need regist
    P2P_NO_DEVICE_ID,
    P2P_NETWORK_NOT_READY
} p2p_status_t;

typedef struct _p2p_base_t {
    p2p_status_t status;
	char device_id[32];
    //char access_ip[32];
	//int  access_port;
	/*从服务器获取的utc时间戳，减去了系统的uptime， 可利用uptime实时算出utc时间*/
	int  server_timestamp_base_uptime;	

    int  token_valid;   //token 是否有效
    char token[128];
    char access_aes_key[128];
    char aes_key_id[128];    
    int  cover_refresh_time;    //封面刷新时间，单位秒
    int  dm_record_time;        //门磁告警录像时长, 单位秒
    int  cvr_timeout_intv;      //云存储间隔
    int  cloud_stream_type;     //云存储码流类型
    int  cloud_iframe_interval;    //云存储发送I帧间隔
    int  rtsp_frame_rate;       // rtsp 码流的帧率
    int  rtsp_bit_rate;         // rtsp 码流的码率
} p2p_base_t;

/* define in p2p_main.cpp */
p2p_base_t* p2p_base();

#endif //_P2P_DEF_H_
