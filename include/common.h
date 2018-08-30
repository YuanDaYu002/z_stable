
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

/*只需在makefile或者make 命令编译时制定机型即可打开特定宏定义by harvey*/

//版本信息
//*8-15 bits保留
//*16-23 bits 代表设备类型00:IPC CMOS VGA 01:IPC CMOS 720P 02:IPC CMOS 1080P 03 IPC CCD 04:DVR 05:NVR

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
	unsigned int 			m_u8Exist;  // 0: 不存在 ， 1 存在但是没有加载上，2，存在并加载上文件系统
	unsigned long			m_u32Capacity;  // 以k为单位
	unsigned long			m_u32FreeSpace; // 以k 为单位
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
	SYSMAINETANCE_SET,   // 系统维护
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


