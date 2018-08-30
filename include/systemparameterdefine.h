
#ifndef __SYSTEM_PARAMETER_STRUCT_H__
#define __SYSTEM_PARAMETER_STRUCT_H__
#include "common.h"

typedef struct 
{

	//*8-15 bits����
	//#define		REL_SPECIAL_COM		0x00000000
	//*16-23 bits �����豸����00:IPC CMOS VGA 01:IPC CMOS 720P 02:IPC CMOS 1080P 03 IPC CCD 04:DVR 05:NVR
	//#define		REL_VERSION_COM		0x00000000
	//*24-31��ʾоƬ����
	//#define		CHIP_TYPE_HI3511	0x50000000	//3507/3511оƬ
	//#define		CHIP_TYPE_HI3515	0x52000000
	//#define		CHIP_TYPE_HI3520	0x54000000
	//#define		DEV_TYPE_INFO		CHIP_TYPE_HI3511+REL_VERSION_COM+REL_SPECIAL_COM+MAX_REC_CHANNEL

	int				DeviceType;				//�豸����DEV_TYPE_INFO
	char			DeviceName[32];			//�豸����
	char			SerialNumber[32];		//MAC��ַ
	char			HardWareVersion[32];	//Ӳ���汾
	char			SoftWareVersion[32];		//����汾
	char			VideoNum;				//��Ƶͨ����
	char			AudioNum;				//��Ƶͨ����
	char			AlarmInNum;			//��������
	char			AlarmOutNum;			//�������
	char			SupportAudioTalk;		//�Ƿ�֧�ֶԽ�1:֧��0:��֧��
	char			SupportStore;			//�Ƿ�֧�ֱ��ش���1:֧��0:��֧��
	char			SupportWifi;				//�Ƿ�֧��WIFI 1:֧��0:��֧��
	char			resver;					//����
	
}TYPE_DEVICE_INFO;

typedef struct
{
    unsigned short      m_u16StartTime;         // ʱ������Ƭ�εĿ�ʼʱ�� 60xh+m
    unsigned short      m_u16EndTime;           // ʱ������Ƭ�εĽ���ʱ�� 60xh+m
    unsigned char           m_u8Valid;              // ʱ����Ƿ���Ч
    unsigned char           m_u8Reserved;
    unsigned short      m_validChannel;         // ��Ӧ�Ķ�ʱ��ͨ����Чλ
}TIMETBLSECTION;

typedef struct
{
    TIMETBLSECTION      m_TBLSection[4]; // ʱ���
    unsigned char       m_u8WeekDay;     // ���ڼ�
    unsigned char       m_u8Reserved[3];
}ALARMINDATESET;
typedef struct 
{
    unsigned char               m_uCenterIP[4];     // ���ķ�����IP
    unsigned char               m_Switch;           // �Ƿ�����ƽ̨
    unsigned char               deviceid[15];       // ƽ̨ע��ID
    char                        passwd[16];                 // ƽ̨�û�����
    unsigned int                m_heartbeat;                // �������ʱ��(s)
    unsigned short          m_uPhoneListenPt;   // �ֻ������˿�(1024-65535)
    unsigned short          m_uVideoListenPt;   // ��Ƶ�����˿�     (1024-65535)
    unsigned short          m_uHttpListenPt;    // http �����˿�(1024-65535  80)
    unsigned short          m_uEnrolPort;       // ע��������˿�(1024-65535)
    
}TYPE_CENTER_NETWORK;

typedef struct 
{
    unsigned char               m_uLocalIp[4];      // ����ip ��ַ
    unsigned char               m_uMask[4];         // ��������
    unsigned char               m_uGateWay[4];      // ����
    unsigned char               m_uMac[6];          // MAC��ַ
    unsigned char               m_dhcp;             // DHCP����1:��0:��
    unsigned char               m_upnp;             // upnp����1:��0:��
    unsigned short          m_v_port;           // ��Ƶӳ��˿�(1024-65535)
    unsigned short          m_http_port;        // HTTPӳ��˿�(1024-65535)
    unsigned short          m_plat_port;        // ƽ̨ӳ��˿�(1024-65535)
    unsigned short          m_phone_port;       // �ֻ�ӳ��˿�(1024-65535)
}TYPE_NETWORK_DEVICE;


typedef struct
{
    unsigned char               m_umDNSIp[4];       // �� DNS
    unsigned char               m_usDNSIp[4];       // ���� DNS
}TYPE_NETWORK_DNS;

typedef struct 
{   
    unsigned char               m_ntp_switch;               // NTP��ʱ����
    char                        m_Reserved[3];              
}TYPE_NETWORK_NTP;

typedef struct
{
    unsigned int            m_uPppoeIP[4];  // ��δʹ��
    char                    m_s8UserName[32];
    char                    m_s32Passwd[16];
    unsigned char           m_u8PppoeSelected;  //�Ƿ�����PPPOE 1:on 0:off
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
    char                    m_s8Name[32];       // ����
    char                    m_s8UserName[16];
    char                    m_s32Passwd[16];        
    unsigned char           m_u8Selected;       //  �Ƿ�����1:����0:������
    unsigned char           m_server;           // 1:��ʾ3322.org, 2:��ʾdynDDNS.org
    unsigned char           m_u8Reserved[6];
    
}TYPE_DYNAMICDOMAIN;

typedef struct
{
    char                    m_server[32];       // ��������ַ
    char                    m_account[32];      // �ʻ�
    char                    m_password[16];     // ���� 
    int                 m_port;             //�˿�
}TYPE_FTP;

typedef struct
{
    char                    m_title[64];            //����
    char                    m_server[32];       // ��������ַ
    char                    m_addr[32];         // ���������ַ
    char                    m_account[32];      // ���������ַ
    char                    m_password[16];     // ����
    int                     m_mode;             // ��֤ģʽ
    int                     m_u8Sslswitch;      //�Ƿ�����SSL 1:����0: ������
    int                     m_u16SslPort;       //�˿�
}TYPE_EMAIAL;

typedef struct
{
    unsigned char           m_u8Selected;       //WIFI�Ƿ�����1:on 0:off
    unsigned char           m_dhcp;         //dhcp�Ƿ�����1:on 0:off
    unsigned char           m_uLocalIp[4];      // ip ��ַ
    unsigned char           m_uMask[4];         // ��������
    unsigned char           m_uGateWay[4];      // ����
    unsigned char           m_uMac[6];          // MAC��ַ
    unsigned char           m_umDNSIp[4];       // �� DNS
    unsigned char           m_usDNSIp[4];       // ���� DNS
    
}TYPE_WIFI_ADDR;

typedef struct
{
             char               RouteDeviceName[32];    //�ȵ�����
             char               Passwd[32];             //����
    unsigned char               AuthenticationMode;     //��֤ģʽ
    unsigned char               EncryptionProtocol;     //����Э��
    unsigned char               Index;                  //ͨ��
    unsigned char               SignalLevel;                //�ȵ�ǿ��(�ͻ�����Ҫ��255�õ��ٷֱ�)1-255
    unsigned char               ConnectStatus;      //����״̬0:δ����1:������
    unsigned char               WepKeyMode;     //0:ASCII 1:Hex
    char                        lang:3;// 0:english 1:chinese 2:xxx 3:xxx,............. 
    char                        Smartlink:1;//   1 :smartlink 0:normal
    char                        res:4;
    char                        m_Reserved;

}TYPE_WIFI_LOGIN;

typedef struct
{
    TYPE_WIFI_ADDR      WifiAddrMode;       //�豸��ȡ��ַ��ʽ
    TYPE_WIFI_LOGIN     LoginWifiDev;       //��½WIFI�ṹ
    
}TYPE_WIFI_DEVICE;

typedef struct 
{
    TYPE_CENTER_NETWORK         m_CenterNet;    // λ0 ƽ̨�������˿�����
    TYPE_NETWORK_DNS            m_DNS;          // λ1 DNS ����������
    TYPE_NETWORK_DEVICE         m_Eth0Config;   // λ2 ������һ���ڼ�ӳ��˿�����
    TYPE_NETWORK_DEVICE         m_Eth1Config;   // λ3 �����ڶ������ڵ�����
    TYPE_PPPOEPARA              m_PppoeSet;     // λ4 PPPOE
    TYPE_NATSETPARA             m_NatConfig;    // λ5Ԥ���ṹ��
    TYPE_DYNAMICDOMAIN          m_DomainConfig; // λ6   DDNS����
    TYPE_FTP                    m_ftp;          // λ7 FTP�ϴ�
    TYPE_EMAIAL                 m_email;        // λ8 �������
    TYPE_NETWORK_NTP            m_NTP;          // NTP��ʱ
    TYPE_WIFI_DEVICE            m_WifiConfig;   //wifi
    unsigned int                m_changeinfo;   // ����ָʾ ��Ӧbit0-bit10 1:�и���0:û�и���

}NETWORK_PARA;

typedef struct
{
	//  1: ����0:������
	ALARMINDATESET		m_TimeTblSet[8];    /* ÿ��һ���������ڼ��㲼����ʱ���*/
	unsigned char 		m_u8ZoneEnable;  // �Ƿ�����
	unsigned char			m_u8OSDEnable;   // �Ƿ�����Ļ��ʾ
	unsigned char			m_u8RpCenter;    // �Ƿ��ϱ�����
	unsigned char			m_u8EmailEnable; //	�Ƿ���e-mail
	unsigned int			m_u32UionChannel;  //��̨����0-31����0-31ͨ��
	unsigned int			m_u32RecEnable;	   // ����ʹ��ͨ��¼�� ÿһλ��ʾһ��ͨ����ÿһλʹ��һ��ͨ����Ϊ1��λʹ��¼��
	unsigned int			m_u8ShotEnable;   // ����ʹ��ͨ��ץ�ģ� ÿһλ��ʾһ��ͨ����ÿһλʹ��һ��ͨ����Ϊ1��λʹ��¼��
	unsigned int			m_OutPutPort;	//�����ÿλ����һ�������
	unsigned int			m_VoiceAlarm;//����
	unsigned int			m_DetectTime;//�೤ʱ����һ�δ˲���8��IO��Ҫ����һ������󱣴��Ϊ׼��λ:��
	unsigned int			m_OutputTime;//���������ʱ
	unsigned int			m_uFTP;//�Ƿ�FTP�ϴ�
	unsigned int			m_Mode;//0:����1:����
	
}AlarmZoneSet;

typedef struct
{
	AlarmZoneSet			m_AlarmZone[MAX_ALARM_ZONE];	//16����������
	unsigned int			m_changeinfo1;		// 0:û�и���1:�и���

}ALARMZONEGROUPSET;


/* p2p sync */
typedef struct 
{
	int 				schedule_id;
	int 				flag; 			// 0: ��schedule Ϊ�����豸���� ; 1: ���Ƹ澯����
	int 				repeat_day; 	//��λ���ֶ�, bit0 ��ʾ�����죬�Դ�����
	int 				off_at;		//�رյ���ʼʱ��
	int			 		on_at;		//�رյĽ���ʱ��˵�������off_at ��on_at ��Ϊ0, ���ʾȫ��ر�
}schedule_time;

/* sync paramter struct */
/*chang_flag bit λ */
/*bit 0: device_schedule*/
/*bit 1: notify_schedule*/
typedef struct _web_sync_param_t 
{
	char 	sync_key[12];
	struct
	{			
		unsigned char light_switch; // Light ���� 1:  �� 2���� 3���Զ� Ĭ��ֵΪ1
		unsigned char white_switch; // �׳㿪�� 1���� 2���� Ĭ�ϣ�1
		unsigned char breath_switch; // ��ɫ�л����������ܣ���1���� 2����
		unsigned char color_red;
		unsigned char color_green;
		unsigned char color_blue;
		unsigned char sunrise_hour;
		unsigned char sunrise_min;
		unsigned char sunset_hour;
		unsigned char sunset_min;
	}lamp;

	char    reserved[3];
    unsigned char    frame_rate;   //֡�����ã���λ��֡/s  ����10����10֡/s
    unsigned char    sound_sensitivity;   //����������жȣ�0Ϊ�ͣ�1Ϊ�У�2Ϊ��
    unsigned char    user_mode;           //�û�ģʽ
	unsigned char 	 voice_message_index; // �û�����ʹ���������� 0-4, 0xff ��ʾû��ѡ��
	unsigned char 	 microwave_switch;    // ΢����⿪�أ�0���ر�1������ Ĭ��ֵ��0
	unsigned char    nightvision_switch;  // ҹ�ӿ��� 1���Զ���2���������⣬3���رպ���
	unsigned char    imageflip_switch;	  // ͼ����� 0��������1�����·�ת��2�����Ҿ���3:180����ת����ת�Ӿ���
	
	/* �ʱ�����Ƴ���Ϊ30bytes�� ��Ҫʱ�����ó�32bytes��ʹ�� */
	char	time_zone[64];
	int     time_offset;
    unsigned char     device_backlight;   // ����ֵ
    unsigned char     light_brightness;   // �׳������
	char	reserved2[22];	
	//int	 	cvr_timeout_intv;	  // �����ƴ洢�������, ��λ��
	int 	alarm_interval;
	int 	mute;
	int 	device_on;
	/**************���ֿ���*************/
			
	unsigned int use_voice_message:1; // �Ƿ���������
	unsigned int chime:1;			  // �Ƿ�����
	unsigned int light_schedule:1;    // 0�رգ�1����, Ĭ��0
    unsigned int sound_detetion:1;    // ������������, 0�رգ� 1����
	unsigned int switchs_reserved:28;
	
	/***************************/
	int 	device_schedule;
    unsigned char    device_volume;          // �������
    unsigned char    statistics_interval;    // ��������ͳ��ʱ����
    unsigned char    alarm_stream_bitrate;    // ������Ƶ�������ʼ���
    unsigned char    seconds_of_microwave_before_md; // �ƶ���ⴥ��ʱͳ��ǰ�������ڷ���΢������
	short   ircut_admax;
	short   ircut_admin;
	int 	cvr_on; 
	int 	sensitivity;
	schedule_time  time_list[10];
}web_sync_param_t; // 360bytes

#endif 



