/*
 * filename: 	zmd_msg.h
 * description: 	ָ���ͷ�ļ�
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

#define SYNC_KEY				"synckey"       		/* �豸����ͬ�����кţ���ʼΪ0, ��󲻻ᳬ��32��byte */
#define SYNC_IO_NUM				"device_ionum"
#define SYNC_IO_ALAM			"device_ioalarm"		/* ������,31λ�������ӵ͵���λ��ʾÿ��io ͨ���Ŀ���״̬  Bit0: 0----offf , 1---on */
#define SYNC_MD_ALAM			"device_mdalarm"		/* ������,31λ�������ӵ͵���λ��ʾÿ��io ͨ���Ŀ���״̬  Bit0: 0----offf , 1---on */
#define SYNC_VIDEO_LOST_ALAM	"device_videolostalarm" /* 0----off ,1---on */
#define SYNC_CHANNEL			"device_channel"
#define SYNC_PILOT				"device_pilot"			/* �豸ָʾ�ƿ���:0----�رգ�1---���� */
#define SYNC_RECORD				"device_record"			/* ������,31λ�������ӵ͵���λ��ʾÿ����Ƶͨ����¼�񿪹�״̬. Bit0: 0----offf , 1---on */
#define SYNC_CAPACITY			"device_capacity"		
#define SYNC_VERSION			"device_version"
#define SYNC_MODEL				"device_model"
#define SYNC_TIMEZONE			"time_zone"				/* �豸ʱ�� */
#define SYNC_TIMEOFFSET			"time_offset"			/* �豸ʱ����ƫ�� */
#define SYNC_EXTEND_CAPACITY	"device_extend_capacity"
#define SYNC_MUTE				"mute"					/* �豸�Ƿ�����1��ʾ��������2��ʾ�ر����� */
#define SYNC_ALARM_INTERVAL		"alarm_interval"		/* �豸����ʱ������Ĭ��ֵ900������λ�� */
#define SYNC_DEVECE_ON			"device_on"				/* 1: �豸�� 0: �豸�� .Ĭ��ֵΪ1 */
#define SYNC_DEVICE_SCHEDULE	"device_schedule"		/* ����device���صĲ���, ����һ����λ��,��Ӧ��bit Ϊ1 ��ʾ����0��ʾ��. Ĭ��ֵΪ0 Bit0: by location ;Bit1: by time */
#define SYNC_NOTIFICATION		"notification"			/* �豸�澯���ܿ��� */
#define SYNC_NOTIFY_WHTEN		"notify_when"			/* ��λ���ֶ�, ��Ӧ��bit Ϊ1 ��ʾ����0��ʾ��. Ĭ��ֵΪ0  Bit0: ��⵽�ƶ����ʱ�����澯; Bit1: ��⵽����ʱ�����澯 */
#define SYNC_NOTIFY_SCHEDULE	"notify_schedule"		/* notify�����Ĳ���, ����һ����λ��,��Ӧ��bit Ϊ1 ��ʾ����0��ʾ��. Ĭ��ֵΪ0  Bit0: by location ;Bit1: by time*/
#define SYNC_SCHEDULE_LIST		"schedule_list"			/* �豸schedule�б� */
#define SYNC_CVR_ON				"cvr_on"				/* �ƴ洢�Ƿ����� 0��ʾ�رգ�1��ʾ������ͨ�ƴ洢��2��ʾ���������ƴ洢 */
#define SYNC_SENSITIVITY		"sensitivity"			/* ������ */
#define SYNC_LIGHT_SWITCH		"light_switch"			/* Light ���� */
#define SYNC_WHITE_SWITCH		"white_switch"			/* �׳㿪�� */
#define SYNC_BREATH_SWITCH		"breathe_switch"		/* ��ɫ�л����������ܣ� */
#define SYNC_RGB				"rgb"
#define SYNC_MICROWAVE_SWITCH   "microwave_switch"		/* ΢����⿪�أ�0���ر�1������ Ĭ��ֵ��0 */
#define SYNC_SUB_DEVICES		"relation"				/* �󶨵ı����豸 */
#define SYNC_IMAGEFLIP			"imageflip"				/* ͼ�����0��������1�����·�ת��2�����Ҿ���3:180����ת����ת�Ӿ���*/
#define SYNC_NIGHTSWITCH		"nightvision"			/* ҹ�ӿ��� 1���Զ���2���������⣬3���رպ��� */
#define SYNC_LOCAL_PWD			"local_pwd"				/* admin �û������룬MD5����*/
#define SYNC_PRESET				"preset_list"			/* Ԥ�õ��б� */
#define SYNC_USE_VOICE_MSG		"use_voice_message"		/* �Ƿ�ʹ������ */
#define SYNC_CHIME_SCHEDULE		"chime_schedule"		/* ����ʱ��ο��ƿ��� */
#define SYNC_CHIME				"chime"					/* ���忪�� */
#define SYNC_SUNRISE			"sunrise"				/* �ŵ��ճ� */
#define SYNC_SUNSET				"sunset"				/* �ŵ����� */
#define SYNC_LIGHT_SCHEDULE		"light_schedule"		/* �ŵƿ���ʱ��� */
#define SYNC_CVR_INTERVAL		"cvr_timeout_intv"		/* �����ƴ洢�������, ��λ��*/
#define SYNC_SOUND_DETECTION    "sound_detection"        /* ������⿪�� */
#define SYNC_SOUND_SENSITIVITY  "sound_sensitivity"     /* ����������ж� */
#define SYNC_USER_MODE          "user_mode"             /* �û�ģʽ */
#define SYNC_NIGHT_ADC          "night_adc"             /* IRCUT �л���Χ */
/* �¿� */
#define SYNC_WORK_MODE          "work_mode"             /* ����ģʽ��0Ϊ���䣬1Ϊ���ȣ�2Ϊ��ʪ��3Ϊͨ��*/
#define SYNC_AIR_SWITCH         "air_switch"            /* �յ����أ�0Ϊ�أ�1Ϊ�� */
#define SYNC_COOL_TEMP          "cool_temp"             /* ����ģʽ�µ��¶����� */
#define SYNC_HEAT_TEMP          "heat_temp"             /* ����ģʽ�µ��¶����� */
#define SYNC_DRY_TEMP           "dry_temp"              /* ��ʪģʽ�µ��¶����� */
#define SYNC_VEN_TEMP           "ven_temp"              /* ͨ��ģʽ�µ��¶����� */
#define SYNC_PUSH_CONDITION          "push_condition_list"              /* �������Ͳ��� */
#define SYNC_VOLUME             "device_volume"         /* ����������� */
#define SYNC_STATISTICS_INTERVAL      "statistics_interval" /* ����ͳ��ʱ����*/
#define SYNC_ALARM_STREAM_BITRATE    "alarm_stream_bitrate" /* �澯������������ */
#define SYNC_SEC_MW_BEFORE_MD   "seconds_of_microwave_before_md"  /*�ƶ���ⴥ��ʱͳ��ǰ�������ڷ���΢������*/
#define SYNC_BACKLIGHT          "device_backlight"      /* ����ֵ */
#define SYNC_LIGHT_BRIGHTNESS   "light_brightness"      /* �׳������ */
#define SYNC_FRAME_RATE         "frame_rate"                      /* ֡�ʣ���λ ֡/s */
#define SYNC_COVER_REFRESH_TIME "cover_refresh_time"    /* ����ˢ��ʱ�� */
#define SYNC_DM_RECORD_TIME     "dm_record_time"        /* �ŴŸ澯¼��ʱ�� */
#define SYNC_ALARM_STRATEGY     "alarm_strategy"        /* �澯���� */
#define SYNC_CLOUD_IFRAME_INTERVAL  "could_i_frame_interval"   /* �ƴ洢����I֡��� */
#define SYNC_CLOUD_STREAM_TYPE      "could_record_resolution"  /* �ƴ洢ʹ�õ��������� */
#define SYNC_RTSP_FRAME_RATE        "rtsp_frame_rate"   /* rtsp ����֡�� */
#define SYNC_RTSP_BIT_RATE          "rtsp_bit_rate"     /* rtsp �������� */

enum cmd
{
    /* ��ͻ���ͨ��Э�� */
    PC_TRANS_LOGIN_SYN = 11000,
    PC_TRANS_LOGIN_ACK = 11100,
    PC_TRANS_HEART_SYN = 11001,
    PC_TRANS_HEART_ACK = 11101,

    /* ���豸ͨ��Э�� */
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


    /* �ͻ������豸ͨ��Э�� */
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

    /* ��SIP������ͨ��Э�� */
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

    DEVICE_TYPE_BIG_THERMOSTAST = 19,	// ���¿�
    DEVICE_TYPE_SMALL_THERMOSTAST = 20, // С�¿�
    DEVICE_TYPE_VENT_HOLE = 21, // ͨ���

    DEVICE_TYPE_FISHEYE_LAMP = 27, // ���۵���
    
};

#define ZMD_MAGIC 0x9F55FFFF
struct trans_msg_s{
    unsigned int	magic;		 /* ����ͷ */
    //short			version;	 /* �汾�� */
    unsigned char   channel_id;
    unsigned char   flag;
    short			cmd_type;	 /* ָ������ */
    unsigned int	cmd;		 /* ָ����*/
    unsigned int	seqnum;		 /* ��ˮ�� */
    unsigned int	length;		 /* ���峤�� */
    //char			*buf;		 /* �������� */
};
struct trans_video_s{
    unsigned int	magic;		 /* ����ͷ */
    char			chlnum; 	 /* channel id */
    char            frame_flag;  /* frame flag*/
    short			cmd_type;	 /* ָ������ */
    unsigned int	cmd;		 /* ָ����*/
    unsigned int	seqnum;		 /* ��ˮ�� */
    unsigned int	length;		 /* ���峤�� */
    //char			*buf;		 /* �������� */
};
#define P2P_HEAD_LEN sizeof(trans_msg_s)
#define P2P_HEAD_INITIALIZER {ZMD_MAGIC, 0, 0, 0, 0, 0, 0}


/* �û����豸��¼ */
struct tf_login_syn_s{
    trans_msg_s head;
    char register_code[32];
    char username_or_sn[32];
};

/* �û����豸��¼ack */
struct tf_login_ack_s{
    unsigned char result;
};

/* SIP����TCP���� */
struct tf_tcpconn_syn_s{
    char channel_id[32];
};


/* SIP����TCP���ӻظ� */
struct tf_tcpconn_ack_s{
    unsigned char result;
    char resave[3];
    char register_code[32];
};


/* �ж���ý�� */
struct client_stopmedia_syn_s{
    char link_id[32];
};
struct client_stopmedia_ack_s{
    unsigned char result;
};

/* �������� */
struct client_mediatype_syn_s{
    unsigned char media_type;
    char resave[3];
    char link_id[32];
};
struct client_mediatype_ack_s{
    unsigned char result;
};

/* �������� */
struct client_voicestatus_syn_s{
    unsigned char voice_status;
    char resave[3];
    char link_id[32];
};
struct client_voicestatus_ack_s{
    unsigned char result;
};

/* �Խ����� */
struct client_talkstatus_syn_s{
    unsigned char talk_status;
    char resave[3];
    char link_id[32];
};
struct client_talkstatus_ack_s{
    unsigned char result;
};

/* �ƶ���⿪�� */
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



