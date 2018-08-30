
#ifndef __SYSTEM_PARAMETER_STRUCT_H__
#define __SYSTEM_PARAMETER_STRUCT_H__
#include "common.h"

typedef struct 
{

	//*8-15 bits保留
	//#define		REL_SPECIAL_COM		0x00000000
	//*16-23 bits 代表设备类型00:IPC CMOS VGA 01:IPC CMOS 720P 02:IPC CMOS 1080P 03 IPC CCD 04:DVR 05:NVR
	//#define		REL_VERSION_COM		0x00000000
	//*24-31表示芯片类型
	//#define		CHIP_TYPE_HI3511	0x50000000	//3507/3511芯片
	//#define		CHIP_TYPE_HI3515	0x52000000
	//#define		CHIP_TYPE_HI3520	0x54000000
	//#define		DEV_TYPE_INFO		CHIP_TYPE_HI3511+REL_VERSION_COM+REL_SPECIAL_COM+MAX_REC_CHANNEL

	int				DeviceType;				//设备类型DEV_TYPE_INFO
	char			DeviceName[32];			//设备名称
	char			SerialNumber[32];		//MAC地址
	char			HardWareVersion[32];	//硬件版本
	char			SoftWareVersion[32];		//软件版本
	char			VideoNum;				//视频通道数
	char			AudioNum;				//音频通道数
	char			AlarmInNum;			//报警输入
	char			AlarmOutNum;			//报警输出
	char			SupportAudioTalk;		//是否支持对讲1:支持0:不支持
	char			SupportStore;			//是否支持本地储存1:支持0:不支持
	char			SupportWifi;				//是否支持WIFI 1:支持0:不支持
	char			resver;					//保留
	
}TYPE_DEVICE_INFO;

typedef struct
{
    unsigned short      m_u16StartTime;         // 时间任务片段的开始时间 60xh+m
    unsigned short      m_u16EndTime;           // 时间任务片段的结束时间 60xh+m
    unsigned char           m_u8Valid;              // 时间段是否有效
    unsigned char           m_u8Reserved;
    unsigned short      m_validChannel;         // 对应的定时器通道有效位
}TIMETBLSECTION;

typedef struct
{
    TIMETBLSECTION      m_TBLSection[4]; // 时间段
    unsigned char       m_u8WeekDay;     // 星期几
    unsigned char       m_u8Reserved[3];
}ALARMINDATESET;
typedef struct 
{
    unsigned char               m_uCenterIP[4];     // 中心服务器IP
    unsigned char               m_Switch;           // 是否连接平台
    unsigned char               deviceid[15];       // 平台注册ID
    char                        passwd[16];                 // 平台用户密码
    unsigned int                m_heartbeat;                // 心跳间隔时间(s)
    unsigned short          m_uPhoneListenPt;   // 手机监听端口(1024-65535)
    unsigned short          m_uVideoListenPt;   // 视频监听端口     (1024-65535)
    unsigned short          m_uHttpListenPt;    // http 监听端口(1024-65535  80)
    unsigned short          m_uEnrolPort;       // 注册服务器端口(1024-65535)
    
}TYPE_CENTER_NETWORK;

typedef struct 
{
    unsigned char               m_uLocalIp[4];      // 本机ip 地址
    unsigned char               m_uMask[4];         // 子网掩码
    unsigned char               m_uGateWay[4];      // 网关
    unsigned char               m_uMac[6];          // MAC地址
    unsigned char               m_dhcp;             // DHCP开关1:开0:关
    unsigned char               m_upnp;             // upnp开关1:开0:关
    unsigned short          m_v_port;           // 视频映射端口(1024-65535)
    unsigned short          m_http_port;        // HTTP映射端口(1024-65535)
    unsigned short          m_plat_port;        // 平台映射端口(1024-65535)
    unsigned short          m_phone_port;       // 手机映射端口(1024-65535)
}TYPE_NETWORK_DEVICE;


typedef struct
{
    unsigned char               m_umDNSIp[4];       // 主 DNS
    unsigned char               m_usDNSIp[4];       // 备用 DNS
}TYPE_NETWORK_DNS;

typedef struct 
{   
    unsigned char               m_ntp_switch;               // NTP对时开关
    char                        m_Reserved[3];              
}TYPE_NETWORK_NTP;

typedef struct
{
    unsigned int            m_uPppoeIP[4];  // 暂未使用
    char                    m_s8UserName[32];
    char                    m_s32Passwd[16];
    unsigned char           m_u8PppoeSelected;  //是否启用PPPOE 1:on 0:off
    unsigned char           m_u8Reserved[3];

}TYPE_PPPOEPARA;

typedef struct
{
    unsigned char           m_u8NatIpAddr[4];
    unsigned char           m_u8NatIpValid;
    unsigned char           m_u8Reserved[3];
}TYPE_NATSETPARA;

typedef struct
{
    char                    m_s8Name[32];       // 域名
    char                    m_s8UserName[16];
    char                    m_s32Passwd[16];        
    unsigned char           m_u8Selected;       //  是否启用1:启用0:不启用
    unsigned char           m_server;           // 1:表示3322.org, 2:表示dynDDNS.org
    unsigned char           m_u8Reserved[6];
    
}TYPE_DYNAMICDOMAIN;

typedef struct
{
    char                    m_server[32];       // 服务器地址
    char                    m_account[32];      // 帐户
    char                    m_password[16];     // 密码 
    int                 m_port;             //端口
}TYPE_FTP;

typedef struct
{
    char                    m_title[64];            //标题
    char                    m_server[32];       // 服务器地址
    char                    m_addr[32];         // 接收邮箱地址
    char                    m_account[32];      // 发送邮箱地址
    char                    m_password[16];     // 密码
    int                     m_mode;             // 认证模式
    int                     m_u8Sslswitch;      //是否启用SSL 1:启用0: 不启用
    int                     m_u16SslPort;       //端口
}TYPE_EMAIAL;

typedef struct
{
    unsigned char           m_u8Selected;       //WIFI是否启用1:on 0:off
    unsigned char           m_dhcp;         //dhcp是否启用1:on 0:off
    unsigned char           m_uLocalIp[4];      // ip 地址
    unsigned char           m_uMask[4];         // 子网掩码
    unsigned char           m_uGateWay[4];      // 网关
    unsigned char           m_uMac[6];          // MAC地址
    unsigned char           m_umDNSIp[4];       // 主 DNS
    unsigned char           m_usDNSIp[4];       // 备用 DNS
    
}TYPE_WIFI_ADDR;

typedef struct
{
             char               RouteDeviceName[32];    //热点名称
             char               Passwd[32];             //密码
    unsigned char               AuthenticationMode;     //认证模式
    unsigned char               EncryptionProtocol;     //加密协议
    unsigned char               Index;                  //通道
    unsigned char               SignalLevel;                //热点强度(客户端需要除255得到百分比)1-255
    unsigned char               ConnectStatus;      //连接状态0:未连接1:已连接
    unsigned char               WepKeyMode;     //0:ASCII 1:Hex
    char                        lang:3;// 0:english 1:chinese 2:xxx 3:xxx,............. 
    char                        Smartlink:1;//   1 :smartlink 0:normal
    char                        res:4;
    char                        m_Reserved;

}TYPE_WIFI_LOGIN;

typedef struct
{
    TYPE_WIFI_ADDR      WifiAddrMode;       //设备获取地址方式
    TYPE_WIFI_LOGIN     LoginWifiDev;       //登陆WIFI结构
    
}TYPE_WIFI_DEVICE;

typedef struct 
{
    TYPE_CENTER_NETWORK         m_CenterNet;    // 位0 平台及监听端口设置
    TYPE_NETWORK_DNS            m_DNS;          // 位1 DNS 服务器设置
    TYPE_NETWORK_DEVICE         m_Eth0Config;   // 位2 本机第一网口及映射端口设置
    TYPE_NETWORK_DEVICE         m_Eth1Config;   // 位3 本机第二个网口的设置
    TYPE_PPPOEPARA              m_PppoeSet;     // 位4 PPPOE
    TYPE_NATSETPARA             m_NatConfig;    // 位5预留结构体
    TYPE_DYNAMICDOMAIN          m_DomainConfig; // 位6   DDNS服务
    TYPE_FTP                    m_ftp;          // 位7 FTP上传
    TYPE_EMAIAL                 m_email;        // 位8 邮箱服务
    TYPE_NETWORK_NTP            m_NTP;          // NTP对时
    TYPE_WIFI_DEVICE            m_WifiConfig;   //wifi
    unsigned int                m_changeinfo;   // 更新指示 对应bit0-bit10 1:有更新0:没有更新

}NETWORK_PARA;

typedef struct
{
	//  1: 启用0:不启用
	ALARMINDATESET		m_TimeTblSet[8];    /* 每天一个处理按星期计算布撤防时间段*/
	unsigned char 		m_u8ZoneEnable;  // 是否启用
	unsigned char			m_u8OSDEnable;   // 是否有屏幕显示
	unsigned char			m_u8RpCenter;    // 是否上报中心
	unsigned char			m_u8EmailEnable; //	是否发送e-mail
	unsigned int			m_u32UionChannel;  //云台关联0-31代表0-31通道
	unsigned int			m_u32RecEnable;	   // 报警使能通道录像， 每一位表示一个通道，每一位使能一个通道，为1的位使能录像
	unsigned int			m_u8ShotEnable;   // 报警使能通道抓拍， 每一位表示一个通道，每一位使能一个通道，为1的位使能录像
	unsigned int			m_OutPutPort;	//输出口每位代表一个输出口
	unsigned int			m_VoiceAlarm;//蜂鸣
	unsigned int			m_DetectTime;//多长时间检测一次此参数8个IO口要保持一致以最后保存的为准单位:秒
	unsigned int			m_OutputTime;//报警输出延时
	unsigned int			m_uFTP;//是否FTP上传
	unsigned int			m_Mode;//0:常闭1:常开
	
}AlarmZoneSet;

typedef struct
{
	AlarmZoneSet			m_AlarmZone[MAX_ALARM_ZONE];	//16个报警输入
	unsigned int			m_changeinfo1;		// 0:没有更新1:有更新

}ALARMZONEGROUPSET;


/* p2p sync */
typedef struct 
{
	int 				schedule_id;
	int 				flag; 			// 0: 此schedule 为控制设备开关 ; 1: 控制告警推送
	int 				repeat_day; 	//按位或字段, bit0 表示星期天，以此类推
	int 				off_at;		//关闭的起始时刻
	int			 		on_at;		//关闭的结束时刻说明：如果off_at 与on_at 都为0, 则表示全天关闭
}schedule_time;

/* sync paramter struct */
/*chang_flag bit 位 */
/*bit 0: device_schedule*/
/*bit 1: notify_schedule*/
typedef struct _web_sync_param_t 
{
	char 	sync_key[12];
	struct
	{			
		unsigned char light_switch; // Light 开关 1:  开 2：关 3：自动 默认值为1
		unsigned char white_switch; // 白炽开关 1：开 2：关 默认：1
		unsigned char breath_switch; // 颜色切换（呼吸功能），1：开 2：关
		unsigned char color_red;
		unsigned char color_green;
		unsigned char color_blue;
		unsigned char sunrise_hour;
		unsigned char sunrise_min;
		unsigned char sunset_hour;
		unsigned char sunset_min;
	}lamp;

	char    reserved[3];
    unsigned char    frame_rate;   //帧率设置，单位：帧/s  比如10代表10帧/s
    unsigned char    sound_sensitivity;   //声音侦测敏感度，0为低，1为中，2为高
    unsigned char    user_mode;           //用户模式
	unsigned char 	 voice_message_index; // 用户正在使用哪条留言 0-4, 0xff 表示没有选择
	unsigned char 	 microwave_switch;    // 微波侦测开关，0：关闭1：开启 默认值：0
	unsigned char    nightvision_switch;  // 夜视开关 1：自动，2：开启红外，3：关闭红外
	unsigned char    imageflip_switch;	  // 图像控制 0：正常，1：上下翻转，2：左右镜像，3:180度旋转（翻转加镜像）
	
	/* 最长时区名称长度为30bytes， 必要时可以拿出32bytes来使用 */
	char	time_zone[64];
	int     time_offset;
    unsigned char     device_backlight;   // 背光值
    unsigned char     light_brightness;   // 白炽灯亮度
	char	reserved2[22];	
	//int	 	cvr_timeout_intv;	  // 智能云存储报警间隔, 单位秒
	int 	alarm_interval;
	int 	mute;
	int 	device_on;
	/**************各种开关*************/
			
	unsigned int use_voice_message:1; // 是否启用留言
	unsigned int chime:1;			  // 是否响铃
	unsigned int light_schedule:1;    // 0关闭，1开启, 默认0
    unsigned int sound_detetion:1;    // 声音报警开关, 0关闭， 1开启
	unsigned int switchs_reserved:28;
	
	/***************************/
	int 	device_schedule;
    unsigned char    device_volume;          // 输出音量
    unsigned char    statistics_interval;    // 报警分析统计时间间隔
    unsigned char    alarm_stream_bitrate;    // 报警视频流的码率级别
    unsigned char    seconds_of_microwave_before_md; // 移动侦测触发时统计前多少秒内发生微波报警
	short   ircut_admax;
	short   ircut_admin;
	int 	cvr_on; 
	int 	sensitivity;
	schedule_time  time_list[10];
}web_sync_param_t; // 360bytes

#endif 



