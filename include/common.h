
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

/*ֻ����makefile����make �������ʱ�ƶ����ͼ��ɴ��ض��궨��by harvey*/

//�汾��Ϣ
//*8-15 bits����
//*16-23 bits �����豸����00:IPC CMOS VGA 01:IPC CMOS 720P 02:IPC CMOS 1080P 03 IPC CCD 04:DVR 05:NVR

#define 	MAX_ALARM_ZONE				16

typedef  int  BOOL;

typedef struct 
{
	unsigned char year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	unsigned char week;	
	unsigned char reserved;
} datetime_setting;

typedef struct
{
	unsigned int 			m_u8Exist;  // 0: ������ �� 1 ���ڵ���û�м����ϣ�2�����ڲ��������ļ�ϵͳ
	unsigned long			m_u32Capacity;  // ��kΪ��λ
	unsigned long			m_u32FreeSpace; // ��k Ϊ��λ
	unsigned char			m_cDevName[16];
}BlockDevInfo_S;


typedef enum
{
	SYSSET_NULL = 0,
	SYSNET_SET,
	SYSSECURITY_SET,
	SYSRECTASK_SET,
	SYSMACHINE_SET,
	SYSCAMERA_SET,
	SYSANALOG_SET,
	SYSALARM_SET,
	SYSSENSOR_SET,
	SYSPOWER_SET,
	SYSCOMMON_SET,
	SYSBLIND_SET,
	SYSMOTION_SET,
	SYSREC00TASK_SET,
	SYSREC01TASK_SET,
	SYSREC02TASK_SET,
	SYSREC03TASK_SET,
	SYSREC04TASK_SET,
	SYSREC05TASK_SET,
	SYSREC06TASK_SET,
	SYSREC07TASK_SET,
	SYSRECSCHEDULE_SET,

	SYSPCDIR_SET,
	SYSALARMIN_SET,
	SYSMAINETANCE_SET,   // ϵͳά��
	SYSDISPLAY_SET,
	SYSUSERPREMIT_SET,
	SYSEXCEPT_SET,
	SYSOSDINSERT_SET,
	SYSRUNINFO_SET,
	SYSPTZ_SET,
	SYSALARMZONE_SET,
	SYSDEFSCHEDULE_SET,
	SYSPTZLINK_SET,
	PANEL_SET,
	MEGAEYE_SET,
	MEGAEYE_ECHOSET,
	SYS3G_SET,
	SYS3G_DIAL_SET,
	AUTOSWITCH_SET,
	PELCOCMD_SET,
	PICTIMER_SET,
	VOPIC_SET,
	NETDECODER_SET,
	ALARMPORT_SET,
	VIDEOLOSS_SET,
	SENSOR_SET,
	EXTEND_SET,
	DOORBELL_SCHEDULE_SET
}SYSPARATYPE;

typedef enum WIFI_Connect_Status
{
	WPA_DISCONNECTED = 0,
	WPA_INTERFACE_DISABLED,
	WPA_INACTIVE,
	WPA_SCANNING,
	WPA_AUTHENTICATING,
	WPA_ASSOCIATING,
	WPA_ASSOCIATED,
	WPA_4WAY_HANDSHAKE,
	WPA_GROUP_HANDSHAKE,
	WPA_COMPLETED,
	WIFI_PASSWD_ERROR,
	MONITOR_WIFI,
	WPA_IDLE,
	WIFI_UNCONNECTED,
	WIFI_CONNECTING,
	WIFI_CONNECTED,
	WIFI_ERROR,
	WIFI_MONITOR, 
	SYS_RESET

}WIFI_Connect_Status;

#endif 


