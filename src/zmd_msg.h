/*
 * filename: 	zmd_msg.h
 * description: 	指令的头文件
 * creator:		huangchunhua
 * create date:	2013-09-22
 * correcter: 	
 * correct date: 
 * version: 	0.1
 */ 

#ifndef _ZMD_MSG_H_
#define _ZMD_MSG_H_


#ifdef __cplusplus
extern "C"{
#endif

#define SYNC_KEY				"synckey"       		/* 设备配置同步序列号，初始为0, 最大不会超过32个byte */
#define SYNC_IO_NUM				"device_ionum"
#define SYNC_IO_ALAM			"device_ioalarm"		/* 开关量,31位整数，从低到高位表示每个io 通道的开关状态  Bit0: 0----offf , 1---on */
#define SYNC_MD_ALAM			"device_mdalarm"		/* 开关量,31位整数，从低到高位表示每个io 通道的开关状态  Bit0: 0----offf , 1---on */
#define SYNC_VIDEO_LOST_ALAM	"device_videolostalarm" /* 0----off ,1---on */
#define SYNC_CHANNEL			"device_channel"
#define SYNC_PILOT				"device_pilot"			/* 设备指示灯开关:0----关闭，1---开启 */
#define SYNC_RECORD				"device_record"			/* 开关量,31位整数，从低到高位表示每个视频通道的录像开关状态. Bit0: 0----offf , 1---on */
#define SYNC_CAPACITY			"device_capacity"		
#define SYNC_VERSION			"device_version"
#define SYNC_MODEL				"device_model"
#define SYNC_TIMEZONE			"time_zone"				/* 设备时区 */
#define SYNC_TIMEOFFSET			"time_offset"			/* 设备时区便偏移 */
#define SYNC_EXTEND_CAPACITY	"device_extend_capacity"
#define SYNC_MUTE				"mute"					/* 设备是否静音，1表示打开声音，2表示关闭声音 */
#define SYNC_ALARM_INTERVAL		"alarm_interval"		/* 设备报警时间间隔（默认值900），单位秒 */
#define SYNC_DEVECE_ON			"device_on"				/* 1: 设备开 0: 设备关 .默认值为1 */
#define SYNC_DEVICE_SCHEDULE	"device_schedule"		/* 控制device开关的策略, 这是一个按位或,对应的bit 为1 表示开，0表示关. 默认值为0 Bit0: by location ;Bit1: by time */
#define SYNC_NOTIFICATION		"notification"			/* 设备告警的总开关 */
#define SYNC_NOTIFY_WHTEN		"notify_when"			/* 按位或字段, 对应的bit 为1 表示开，0表示关. 默认值为0  Bit0: 检测到移动侦测时产生告警; Bit1: 检测到声音时产生告警 */
#define SYNC_NOTIFY_SCHEDULE	"notify_schedule"		/* notify开启的策略, 这是一个按位或,对应的bit 为1 表示开，0表示关. 默认值为0  Bit0: by location ;Bit1: by time*/
#define SYNC_SCHEDULE_LIST		"schedule_list"			/* 设备schedule列表 */
#define SYNC_CVR_ON				"cvr_on"				/* 云存储是否开启， 0表示关闭，1表示开启普通云存储，2表示开启智能云存储 */
#define SYNC_SENSITIVITY		"sensitivity"			/* 灵敏度 */
#define SYNC_LIGHT_SWITCH		"light_switch"			/* Light 开关 */
#define SYNC_WHITE_SWITCH		"white_switch"			/* 白炽开关 */
#define SYNC_BREATH_SWITCH		"breathe_switch"		/* 颜色切换（呼吸功能） */
#define SYNC_RGB				"rgb"
#define SYNC_MICROWAVE_SWITCH   "microwave_switch"		/* 微波侦测开关，0：关闭1：开启 默认值：0 */
#define SYNC_SUB_DEVICES		"relation"				/* 绑定的被动设备 */
#define SYNC_IMAGEFLIP			"imageflip"				/* 图像控制0：正常，1：上下翻转，2：左右镜像，3:180度旋转（翻转加镜像）*/
#define SYNC_NIGHTSWITCH		"nightvision"			/* 夜视开关 1：自动，2：开启红外，3：关闭红外 */
#define SYNC_LOCAL_PWD			"local_pwd"				/* admin 用户的密码，MD5加密*/
#define SYNC_PRESET				"preset_list"			/* 预置点列表 */
#define SYNC_USE_VOICE_MSG		"use_voice_message"		/* 是否使用留言 */
#define SYNC_CHIME_SCHEDULE		"chime_schedule"		/* 响铃时间段控制开关 */
#define SYNC_CHIME				"chime"					/* 响铃开关 */
#define SYNC_SUNRISE			"sunrise"				/* 门灯日出 */
#define SYNC_SUNSET				"sunset"				/* 门灯日落 */
#define SYNC_LIGHT_SCHEDULE		"light_schedule"		/* 门灯控制时间段 */
#define SYNC_CVR_INTERVAL		"cvr_timeout_intv"		/* 智能云存储报警间隔, 单位秒*/
#define SYNC_SOUND_DETECTION    "sound_detection"        /* 声音侦测开关 */
#define SYNC_SOUND_SENSITIVITY  "sound_sensitivity"     /* 声音侦测敏感度 */
#define SYNC_USER_MODE          "user_mode"             /* 用户模式 */
#define SYNC_NIGHT_ADC          "night_adc"             /* IRCUT 切换范围 */
/* 温控 */
#define SYNC_WORK_MODE          "work_mode"             /* 工作模式，0为制冷，1为制热，2为除湿，3为通风*/
#define SYNC_AIR_SWITCH         "air_switch"            /* 空调开关，0为关，1为开 */
#define SYNC_COOL_TEMP          "cool_temp"             /* 制冷模式下的温度设置 */
#define SYNC_HEAT_TEMP          "heat_temp"             /* 制热模式下的温度设置 */
#define SYNC_DRY_TEMP           "dry_temp"              /* 除湿模式下的温度设置 */
#define SYNC_VEN_TEMP           "ven_temp"              /* 通风模式下的温度设置 */
#define SYNC_PUSH_CONDITION          "push_condition_list"              /* 报警推送策略 */
#define SYNC_VOLUME             "device_volume"         /* 调节输出音量 */
#define SYNC_STATISTICS_INTERVAL      "statistics_interval" /* 报警统计时间间隔*/
#define SYNC_ALARM_STREAM_BITRATE    "alarm_stream_bitrate" /* 告警推送码流级别 */
#define SYNC_SEC_MW_BEFORE_MD   "seconds_of_microwave_before_md"  /*移动侦测触发时统计前多少秒内发生微波报警*/
#define SYNC_BACKLIGHT          "device_backlight"      /* 背光值 */
#define SYNC_LIGHT_BRIGHTNESS   "light_brightness"      /* 白炽灯亮度 */
#define SYNC_FRAME_RATE         "frame_rate"                      /* 帧率，单位 帧/s */
#define SYNC_COVER_REFRESH_TIME "cover_refresh_time"    /* 封面刷新时间 */
#define SYNC_DM_RECORD_TIME     "dm_record_time"        /* 门磁告警录像时长 */
#define SYNC_ALARM_STRATEGY     "alarm_strategy"        /* 告警策略 */
#define SYNC_CLOUD_IFRAME_INTERVAL  "could_i_frame_interval"   /* 云存储发送I帧间隔 */
#define SYNC_CLOUD_STREAM_TYPE      "could_record_resolution"  /* 云存储使用的码流类型 */
#define SYNC_RTSP_FRAME_RATE        "rtsp_frame_rate"   /* rtsp 码流帧率 */
#define SYNC_RTSP_BIT_RATE          "rtsp_bit_rate"     /* rtsp 码流码率 */

enum cmd
{
    /* 与客户端通信协议 */
    PC_TRANS_LOGIN_SYN = 11000,
    PC_TRANS_LOGIN_ACK = 11100,
    PC_TRANS_HEART_SYN = 11001,
    PC_TRANS_HEART_ACK = 11101,

    /* 与设备通信协议 */
    IPC_TRANS_LOGIN_SYN = 21000,
    IPC_TRANS_LOGIN_ACK = 21100,
    IPC_TRANS_HEART_SYN = 21001,
    IPC_TRANS_HEART_ACK = 21101,
    IPC_TRANS_STOP_SYN = 21002,
    IPC_TRANS_STOP_ACK = 21102,	

    IPC_DB_PLAYBACK_LOGIN_SYN = 21003,
    IPC_DB_PLAYBACK_LOGIN_ACK = 21103,
    IPC_PLAYBACK_START_SYN = 21004,
    IPC_PLAYBACK_START_ACK = 21104,
    IPC_PLAYBACK_LOGIN_SYN = 21005,
    IPC_PLAYBACK_LOGIN_ACK = 21105,


    /* 客户端与设备通信协议 */
    PC_IPC_TRANSFER_DATA = 31000,
    PC_IPC_MEDIA_TYPE_SYN = 31001,
    PC_IPC_MEDIA_TYPE_ACK = 31101,
    PC_IPC_STOP_MEDIA_SYN = 31002,
    PC_IPC_STOP_MEDIA_ACK = 31102,
    PC_IPC_VOICE_STATUS_SYN = 31003,
    PC_IPC_VOICE_STATUS_ACK = 31103,
    PC_IPC_MIC_STATUS_SYN = 31004,
    PC_IPC_MIC_STATUS_ACK = 31104,
    PC_IPC_REPORT_STATUS_SYN = 31005,
    PC_IPC_REPORT_STATUS_ACK = 31105,
    PC_IPC_HEART_SYN = 31006,
    PC_IPC_HEART_ACK = 31106,
    PC_DB_PLAYBACK_SYN = 31007,
    PC_DB_PLAYBACK_ACK = 31107,
    PC_DB_PLAYBACK_STOP_SYN = 31008,
    PC_DB_PLAYBACK_STOP_ACK = 31108,
    PC_IPC_UPNP_LOGIN_SYN = 31009,
    PC_IPC_UPNP_LOGIN_ACK = 31109,
    PC_IPC_UPNP_CHANGE_PWD_SYN = 31010,
    PC_IPC_UPNP_CHANGE_PWD_ACK = 31110,
    PC_IPC_PTZ_SYN = 31011,
    PC_IPC_PTZ_ACK = 31111,
    PC_IPC_GET_PRESETPOINT_SYN = 31012,
    PC_IPC_GET_PRESETPOINT_ACK = 31112,
    PC_IPC_REPLY_DOORBELL_SYN = 31013,
    PC_IPC_REPLY_DOORBELL_ACK = 31113,
    PC_IPC_FLIP_SYN = 31015,
    PC_IPC_FLIP_ACK = 31115,
    PC_IPC_SDCARD_LIST_SYN = 31016,
    PC_IPC_SDCARD_LIST_ACK = 31116,
    PC_IPC_PLAY_SDCARD_SYN = 31017,
    PC_IPC_PLAY_SDCARD_ACK = 31117,
    PC_IPC_IR_SET_SYN = 31018,
    PC_IPC_IR_SET_ACK = 31118,
    PC_IPC_IR_GET_SYN = 31019,
    PC_IPC_IR_GET_ACK = 31119,  
    PC_IPC_PLAYBACK_LIST_SYN = 31020,
    PC_IPC_PLAYBACK_LIST_ACK = 31120,
    PC_IPC_PLAYBACK_SYN = 31021,
    PC_IPC_PLAYBACK_ACK = 31121,
    PC_IPC_PLAYBACK_STOP_SYN = 31022,
    PC_IPC_PLAYBACK_STOP_ACK = 31122,
    IPC_PC_PLAYBACK_STOP_SYN = 31023,
    IPC_PC_PLAYBACK_STOP_ACK = 31123,

    IPC_PC_TALK_HEART_SYN = 31024,
    IPC_PC_TALK_HEART_ACK = 31124,

    PC_IPC_PLAYBACK_DATE_SYN = 31025,
    PC_IPC_PLAYBACK_DATE_ACK = 31125,

    /* 与SIP服务器通信协议 */
    SIP_TRANS_LOGIN_SYN = 41000,
    SIP_TRANS_LOGIN_ACK = 41100,
    SIP_TRANS_TCPCONN_SYN = 41001,
    SIP_TRANS_TCPCONN_ACK = 41101,
    SIP_TRANS_HEART_SYN = 41002,
    SIP_TRANS_HEART_ACK = 41102

};
enum _cmd_type
{
    CMD_TYPE_IPC_CLIENT=0,
    CMD_TYPE_IPC_TRANS,
};

enum extend_ability_name_e
{
    EA_DEVICE_SCHEDULE = 0,
    EA_SMART_CLOUD_RECORD = 4,
    EA_NORAML_CLOUD_RECORD = 5,
    EA_TIMEZONE_SETUP = 6,
    EA_MUTE = 16,
    EA_ALARM_INTERVAL = 18,
    EA_NIGHT_SWITCH = 19,
    EA_IMAGE_FLIP = 20,
    EA_REMOTE_PLAYBACK = 21,
    EA_MICROWAVE_DETECTION = 22,
    EA_SUB_DEVICE = 23,
    EA_H265 = 24,
    EA_PT_PRESET = 25
};
enum new_ability_name_e
{
    NA_VOLUME_CTL = 7,
    NA_BACKLIGHT_CTL = 9,
};
enum p2p_device_type_e
{
    DEVICE_TYPE_IPC = 0,
    DEVICE_TYPE_NVR = 1,
    DEVICE_TYPE_DVR = 2,
    DEVICE_TYPE_BELL = 3,
    DEVICE_TYPE_DOOR_LAMP = 6,
    DEVICE_TYPE_PIVOT = 7,
    DEVICE_TYPE_SIMPLE_BELL = 8,

    DEVICE_TYPE_GARDEN_LAMP = 14,

    DEVICE_TYPE_BIG_THERMOSTAST = 19,	// 大温控
    DEVICE_TYPE_SMALL_THERMOSTAST = 20, // 小温控
    DEVICE_TYPE_VENT_HOLE = 21, // 通风孔

    DEVICE_TYPE_FISHEYE_LAMP = 27, // 鱼眼灯泡
    
};

#define ZMD_MAGIC 0x9F55FFFF
struct trans_msg_s{
    unsigned int	magic;		 /* 特殊头 */
    //short			version;	 /* 版本号 */
    unsigned char   channel_id;
    unsigned char   flag;
    short			cmd_type;	 /* 指令类型 */
    unsigned int	cmd;		 /* 指令字*/
    unsigned int	seqnum;		 /* 流水号 */
    unsigned int	length;		 /* 包体长度 */
    //char			*buf;		 /* 包体内容 */
};
struct trans_video_s{
    unsigned int	magic;		 /* 特殊头 */
    char			chlnum; 	 /* channel id */
    char            frame_flag;  /* frame flag*/
    short			cmd_type;	 /* 指令类型 */
    unsigned int	cmd;		 /* 指令字*/
    unsigned int	seqnum;		 /* 流水号 */
    unsigned int	length;		 /* 包体长度 */
    //char			*buf;		 /* 包体内容 */
};
#define P2P_HEAD_LEN sizeof(trans_msg_s)
#define P2P_HEAD_INITIALIZER {ZMD_MAGIC, 0, 0, 0, 0, 0, 0}


/* 用户或设备登录 */
struct tf_login_syn_s{
    trans_msg_s head;
    char register_code[32];
    char username_or_sn[32];
};

/* 用户或设备登录ack */
struct tf_login_ack_s{
    unsigned char result;
};

/* SIP请求TCP连接 */
struct tf_tcpconn_syn_s{
    char channel_id[32];
};


/* SIP请求TCP连接回复 */
struct tf_tcpconn_ack_s{
    unsigned char result;
    char resave[3];
    char register_code[32];
};


/* 中断流媒体 */
struct client_stopmedia_syn_s{
    char link_id[32];
};
struct client_stopmedia_ack_s{
    unsigned char result;
};

/* 更换码流 */
struct client_mediatype_syn_s{
    unsigned char media_type;
    char resave[3];
    char link_id[32];
};
struct client_mediatype_ack_s{
    unsigned char result;
};

/* 声音开关 */
struct client_voicestatus_syn_s{
    unsigned char voice_status;
    char resave[3];
    char link_id[32];
};
struct client_voicestatus_ack_s{
    unsigned char result;
};

/* 对讲开关 */
struct client_talkstatus_syn_s{
    unsigned char talk_status;
    char resave[3];
    char link_id[32];
};
struct client_talkstatus_ack_s{
    unsigned char result;
};

/* 移动侦测开关 */
struct client_reportstatus_syn_s{
    unsigned char report_status;
    char resave[3];
    char link_id[32];
};
struct client_reportstatus_ack_s{
    unsigned char result;
};

struct client_playback_syn{
    unsigned int date;
    char chl;
    char reserved[3];
};

struct req_playback_list_ack {
    trans_msg_s head;
    char result;
    char reserved[3];
    int  number;
};

struct req_playback_date_list_syn_s {
    int start_date;
    int end_date;
    char channel;
    char reserved[3];
};

struct req_playback_date_list_ack_s {
    trans_msg_s head;
    char result;
    char reserved[3];
    int  number;
    char date_list[0];
};

struct p2p_record_info_t
{
    unsigned int start_time;
    unsigned int end_time;
    short    	 type;	
    short        level;
    char      fpath[128];//fpath is not need by protoco
};


#ifdef __cplusplus
}
#endif

#endif



